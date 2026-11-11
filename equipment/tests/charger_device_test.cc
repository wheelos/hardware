// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/devices/charger/charger_device.h"

#include <future>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "tests/fake_stream.h"

namespace apollo::drivers::equipment::testing {
namespace {
// Synthetic test protocol, never registered by the production component.
class ChargingFixtureBackend final : public ChargingBackend {
 public:
  explicit ChargingFixtureBackend(ChannelBinding binding)
      : binding_(std::move(binding)) {}
  msg::EquipmentError Initialize(const DeviceConfig& config) override {
    return binding_.channel->OpenCan({0x300}, {0x400}, false, false, &session_);
  }
  bool SupportsStop() const override { return true; }
  msg::EquipmentError Activate() override {
    return binding_.channel->Health().error();
  }
  msg::EquipmentError StartCharging(SteadyTime* sent_at) override {
    return Send(1, sent_at);
  }
  msg::EquipmentError StopCharging(SteadyTime* sent_at) override {
    return Send(2, sent_at);
  }
  msg::EquipmentError Poll(ChargingFeedback* feedback, bool* updated) override {
    *updated = false;
    CanFrame frame;
    const auto error = session_->Poll(&frame, updated);
    if (!IsOk(error) || !*updated) {
      return error;
    }
    if (frame.length != 1 ||
        !msg::ChargingState::Phase_IsValid(frame.data[0])) {
      return Error(msg::TRANSPORT_ERROR, "invalid charging fixture frame",
                   true);
    }
    feedback->value.set_phase(
        static_cast<msg::ChargingState::Phase>(frame.data[0]));
    feedback->error = Error(msg::OK);
    feedback->received_at = frame.received_at;
    feedback->observed_at_unix_ns = frame.observed_at_unix_ns;
    *updated = true;
    return Error(msg::OK);
  }
  msg::EquipmentError Deactivate() override { return Error(msg::OK); }

 private:
  msg::EquipmentError Send(uint8_t command, SteadyTime* sent_at) {
    CanFrame frame;
    frame.id = 0x300;
    frame.length = 1;
    frame.data[0] = command;
    return session_->Send(frame, sent_at);
  }
  ChannelBinding binding_;
  std::shared_ptr<CanSession> session_;
};
class TimestampChargingBackend final : public ChargingBackend {
 public:
  msg::EquipmentError Initialize(const DeviceConfig&) override {
    feedback.error = Error(msg::OK);
    feedback.value.set_phase(msg::ChargingState::IDLE);
    feedback.received_at = std::chrono::steady_clock::now();
    return Error(msg::OK);
  }
  bool SupportsStop() const override { return true; }
  msg::EquipmentError Activate() override { return Error(msg::OK); }
  msg::EquipmentError StartCharging(SteadyTime* sent_at) override {
    return StopCharging(sent_at);
  }
  msg::EquipmentError StopCharging(SteadyTime* sent_at) override {
    *sent_at = dispatched = std::chrono::steady_clock::now();
    return Error(msg::OK);
  }
  msg::EquipmentError Poll(ChargingFeedback* value, bool* updated) override {
    *value = feedback;
    *updated = true;
    return Error(msg::OK);
  }
  msg::EquipmentError Deactivate() override { return Error(msg::OK); }
  ChargingFeedback feedback;
  SteadyTime dispatched;
};
}  // namespace
class ChargingDispatchTest : public ::testing::TestWithParam<std::string> {};
TEST_P(ChargingDispatchTest, CompletionAndReleaseRequirePostDispatchFeedback) {
  auto backend = std::make_unique<TimestampChargingBackend>();
  auto* source = backend.get();
  ChargerDevice device(std::move(backend));
  ASSERT_TRUE(IsOk(device.Initialize(Config("charger").devices(0))));
  ASSERT_TRUE(IsOk(device.Activate()));
  DeviceUpdate initial;
  ASSERT_TRUE(IsOk(device.Poll(&initial)));
  OperationContext context{
      "session", std::chrono::steady_clock::now() + std::chrono::seconds(3),
      std::chrono::steady_clock::now()};
  ASSERT_TRUE(
      IsOk(device.Begin(context, Request("instance", msg::StartChargingGoal(),
                                         "start", "tool", "start")
                                     .command())));
  const auto& action = GetParam();
  if (action == "stop") {
    source->feedback.value.set_phase(msg::ChargingState::CHARGING);
    source->feedback.received_at = std::chrono::steady_clock::now();
    DeviceUpdate started;
    ASSERT_TRUE(IsOk(device.Poll(&started)));
    ASSERT_EQ(started.operations.size(), 1);
    msg::StopChargingGoal goal;
    goal.set_session_id("session");
    context.operation_id = "stop";
    context.evidence_after = std::chrono::steady_clock::now();
    ASSERT_TRUE(IsOk(device.Begin(
        context, Request("instance", goal, "stop", "tool", "stop").command())));
  } else if (action == "cancel") {
    ASSERT_TRUE(IsOk(device.Cancel("session")));
  }
  source->feedback.value.set_phase(action == "start"
                                       ? msg::ChargingState::CHARGING
                                       : msg::ChargingState::IDLE);
  for (const auto offset :
       {std::chrono::nanoseconds(-1), std::chrono::nanoseconds(0)}) {
    source->feedback.received_at = source->dispatched + offset;
    DeviceUpdate stale;
    ASSERT_TRUE(IsOk(device.Poll(&stale)));
    EXPECT_TRUE(stale.operations.empty());
    EXPECT_TRUE(device.HasControlLease());
    EXPECT_FALSE(device.SafeToRelease());
  }
  source->feedback.received_at = std::chrono::steady_clock::now();
  DeviceUpdate fresh;
  ASSERT_TRUE(IsOk(device.Poll(&fresh)));
  ASSERT_EQ(fresh.operations.size(), 1);
  EXPECT_EQ(fresh.operations[0].state,
            action == "cancel" ? msg::CANCELED : msg::SUCCEEDED);
  EXPECT_EQ(device.HasControlLease(), action == "start");
  EXPECT_EQ(device.SafeToRelease(), action != "start");
}
INSTANTIATE_TEST_SUITE_P(StartStopAndCancel, ChargingDispatchTest,
                         ::testing::Values("start", "stop", "cancel"));
class ChargerTest : public ::testing::Test {
 protected:
  void Start(uint32_t capacity = 32, uint32_t max_age = 1000) {
    stream = std::make_shared<FakeStream>();
    DeviceFactory factory;
    ASSERT_TRUE(factory.Register(
        "charger", "charging_fixture", [](const ChannelBindings& bindings) {
          return std::make_unique<ChargerDevice>(
              std::make_unique<ChargingFixtureBackend>(bindings.at("main")));
        }));
    auto config = Config("charger");
    config.set_max_operations(capacity);
    config.mutable_devices(0)->set_model("charging_fixture");
    config.mutable_devices(0)->set_state_max_age_ms(max_age);
    config.mutable_devices(0)->clear_model_config();
    runtime = std::make_unique<EquipmentRuntime>(
        std::move(factory), [this](const StreamConfig&) { return stream; });
    Feedback(msg::ChargingState::IDLE);
    ASSERT_TRUE(IsOk(runtime->Initialize(config)));
    ASSERT_TRUE(IsOk(runtime->Start()));
    ASSERT_TRUE(WaitUntil(
        [&] { return IsOk(runtime->GetState(StateRequest()).error()); }));
    instance = runtime->ListDevices({}).runtime_instance_id();
  }
  void Feedback(msg::ChargingState::Phase phase) {
    stream->Feedback(0x400, static_cast<uint8_t>(phase));
  }
  msg::OperationSnapshot Operation(const std::string& id) {
    msg::GetOperationRequest query;
    query.set_runtime_instance_id(instance);
    query.set_operation_id(id);
    return runtime->GetOperation(query).operation();
  }
  msg::ExecuteResponse BeginCharge() {
    return runtime->Execute(
        Request(instance, msg::StartChargingGoal(), "start", "tool", "start"));
  }
  msg::ExecuteResponse StopCharge(const std::string& session) {
    msg::StopChargingGoal goal;
    goal.set_session_id(session);
    return runtime->Execute(Request(instance, goal, "stop", "tool", "stop"));
  }
  msg::ChargingState State() {
    msg::ChargingState state;
    runtime->GetState(StateRequest())
        .state()
        .capabilities(0)
        .value()
        .UnpackTo(&state);
    return state;
  }
  std::shared_ptr<FakeStream> stream;
  std::unique_ptr<EquipmentRuntime> runtime;
  std::string instance;
};
TEST_F(ChargerTest, StartSuccessKeepsSessionAndReservedStopWorksAtCapacity) {
  Start(1);
  const auto start = BeginCharge();
  ASSERT_EQ(start.error().code(), msg::OK);
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
  EXPECT_EQ(Operation(start.operation().operation_id()).state(),
            msg::EXECUTING);
  Feedback(msg::ChargingState::CHARGING);
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(start.operation().operation_id()).state() ==
           msg::SUCCEEDED;
  }));
  msg::ChargingResult result;
  ASSERT_TRUE(
      Operation(start.operation().operation_id()).result().UnpackTo(&result));
  ASSERT_FALSE(result.session_id().empty());
  EXPECT_EQ(result.phase(), msg::ChargingState::CHARGING);
  EXPECT_TRUE(runtime->GetState(StateRequest()).state().blocked());
  msg::CancelOperationRequest cancel;
  cancel.set_runtime_instance_id(instance);
  cancel.set_operation_id(start.operation().operation_id());
  EXPECT_EQ(runtime->CancelOperation(cancel).operation().state(),
            msg::SUCCEEDED);
  EXPECT_EQ(stream->WriteCount(), 1);
  EXPECT_EQ(runtime
                ->Execute(Request(instance, msg::StartChargingGoal(), "second",
                                  "tool", "start"))
                .error()
                .code(),
            msg::BUSY);
  EXPECT_EQ(StopCharge("another-session").error().code(),
            msg::INVALID_ARGUMENT);
  const auto stop = StopCharge(result.session_id());
  ASSERT_EQ(stop.error().code(), msg::OK);
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 2; }));
  EXPECT_EQ(Operation(stop.operation().operation_id()).state(), msg::EXECUTING);
  Feedback(msg::ChargingState::IDLE);
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(stop.operation().operation_id()).state() == msg::SUCCEEDED;
  }));
  EXPECT_FALSE(runtime->GetState(StateRequest()).state().blocked());
  EXPECT_TRUE(State().session_id().empty());
  EXPECT_TRUE(IsOk(runtime->Shutdown()));
}
TEST_F(ChargerTest, ExplicitStopCanInterruptAnUnfinishedStart) {
  Start();
  const auto start = BeginCharge();
  ASSERT_EQ(start.error().code(), msg::OK);
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
  Feedback(msg::ChargingState::STARTING);
  ASSERT_TRUE(WaitUntil([&] { return !State().session_id().empty(); }));
  const auto stop = StopCharge(State().session_id());
  ASSERT_EQ(stop.error().code(), msg::OK);
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 2; }));
  Feedback(msg::ChargingState::IDLE);
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(stop.operation().operation_id()).state() == msg::SUCCEEDED;
  }));
  EXPECT_EQ(Operation(start.operation().operation_id()).state(), msg::CANCELED);
  EXPECT_TRUE(IsOk(runtime->Shutdown()));
}
TEST_F(ChargerTest, CancelPendingStartWaitsForIdleFeedback) {
  Start();
  const auto start = BeginCharge();
  ASSERT_EQ(start.error().code(), msg::OK);
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
  msg::CancelOperationRequest cancel;
  cancel.set_runtime_instance_id(instance);
  cancel.set_operation_id(start.operation().operation_id());
  ASSERT_EQ(runtime->CancelOperation(cancel).error().code(), msg::OK);
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 2; }));
  EXPECT_EQ(Operation(start.operation().operation_id()).state(),
            msg::CANCELING);
  Feedback(msg::ChargingState::IDLE);
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(start.operation().operation_id()).state() == msg::CANCELED;
  }));
  EXPECT_FALSE(runtime->GetState(StateRequest()).state().blocked());
}
TEST_F(ChargerTest, StaleStateDoesNotPreventExplicitStopOfOwnedSession) {
  Start(32, 100);
  const auto start = BeginCharge();
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
  Feedback(msg::ChargingState::CHARGING);
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(start.operation().operation_id()).state() ==
           msg::SUCCEEDED;
  }));
  const auto session = State().session_id();
  std::this_thread::sleep_for(std::chrono::milliseconds(120));
  EXPECT_EQ(runtime->GetState(StateRequest()).error().code(), msg::STALE_STATE);
  const auto stop = StopCharge(session);
  ASSERT_EQ(stop.error().code(), msg::OK);
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 2; }));
  Feedback(msg::ChargingState::IDLE);
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(stop.operation().operation_id()).state() == msg::SUCCEEDED;
  }));
}
TEST_F(ChargerTest, ShutdownStopsOwnedSessionAfterStartOperationHasFinished) {
  Start();
  const auto start = BeginCharge();
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
  Feedback(msg::ChargingState::CHARGING);
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(start.operation().operation_id()).state() ==
           msg::SUCCEEDED;
  }));
  auto stopped = std::async(std::launch::async, [&] {
    const bool sent = WaitUntil([&] { return stream->WriteCount() == 2; });
    if (sent) {
      Feedback(msg::ChargingState::IDLE);
    }
    return sent;
  });
  EXPECT_TRUE(IsOk(runtime->Shutdown()));
  EXPECT_TRUE(stopped.get());
  EXPECT_EQ(Operation(start.operation().operation_id()).state(),
            msg::SUCCEEDED);
}
}  // namespace apollo::drivers::equipment::testing

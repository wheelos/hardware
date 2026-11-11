// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/equipment_runtime.h"

#include <future>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "tests/fake_stream.h"

namespace apollo::drivers::equipment::testing {
class RuntimeTest : public ::testing::Test {
 protected:
  void Start(const EquipmentModuleConfig& config = Config()) {
    stream = std::make_shared<FakeStream>();
    runtime = std::make_unique<EquipmentRuntime>(
        DeviceFactory(), [this](const StreamConfig&) { return stream; });
    stream->Feedback(0x200, 2,
                     config.channels(0).stream().can().enable_can_fd());
    ASSERT_TRUE(IsOk(runtime->Initialize(config)));
    ASSERT_TRUE(IsOk(runtime->Start()));
    ASSERT_TRUE(WaitUntil(
        [this] { return IsOk(runtime->GetState(StateRequest()).error()); }));
    instance = runtime->ListDevices({}).runtime_instance_id();
  }
  msg::OperationSnapshot Operation(const std::string& id) {
    msg::GetOperationRequest query;
    query.set_runtime_instance_id(instance);
    query.set_operation_id(id);
    return runtime->GetOperation(query).operation();
  }
  void Complete(const std::string& id, uint8_t bits = 1, bool fd = false) {
    stream->Feedback(0x200, bits, fd);
    ASSERT_TRUE(WaitUntil([&] { return IsTerminal(Operation(id).state()); }));
  }
  std::shared_ptr<FakeStream> stream;
  std::unique_ptr<EquipmentRuntime> runtime;
  std::string instance;
};
TEST_F(RuntimeTest, SwitchRequiresFeedbackNotWriteAckOrLoopback) {
  Start();
  const auto response = runtime->Execute(Request(instance, SwitchGoal()));
  ASSERT_EQ(response.error().code(), msg::OK);
  const auto id = response.operation().operation_id();
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
  EXPECT_EQ(stream->LastCommand(), 1);
  EXPECT_EQ(Operation(id).state(), msg::EXECUTING);
  stream->Feedback(0x100, 1);
  stream->Feedback(0x200, 4);
  ASSERT_TRUE(WaitUntil([&] {
    msg::SwitchState value;
    return runtime->GetState(StateRequest())
               .state()
               .capabilities(0)
               .value()
               .UnpackTo(&value) &&
           value.transitioning();
  }));
  EXPECT_EQ(Operation(id).state(), msg::EXECUTING);
  Complete(id);
  const auto completed = Operation(id);
  ASSERT_EQ(completed.state(), msg::SUCCEEDED);
  msg::SwitchState result;
  ASSERT_TRUE(completed.result().UnpackTo(&result));
  EXPECT_TRUE(result.enabled());
  EXPECT_FALSE(result.transitioning());
  EXPECT_TRUE(IsOk(runtime->Shutdown()));
}
TEST_F(RuntimeTest, AlreadySatisfiedDoesNotWriteAndCanBeLookedUpByKey) {
  Start();
  const auto request = Request(instance, SwitchGoal(false));
  const auto response = runtime->Execute(request);
  ASSERT_EQ(response.error().code(), msg::OK);
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(response.operation().operation_id()).state() ==
           msg::SUCCEEDED;
  }));
  EXPECT_TRUE(
      Operation(response.operation().operation_id()).already_satisfied());
  EXPECT_EQ(stream->WriteCount(), 0);
  msg::GetOperationRequest query;
  query.set_runtime_instance_id(instance);
  *query.mutable_request_key() = request.request_key();
  EXPECT_EQ(runtime->GetOperation(query).operation().operation_id(),
            response.operation().operation_id());
}
TEST_F(RuntimeTest, DeduplicatesBeforeBusyAndRejectsChangedContent) {
  Start();
  auto request = Request(instance, SwitchGoal());
  request.mutable_command()->mutable_arguments()->set_value("\xff");
  EXPECT_EQ(runtime->Execute(request).error().code(), msg::INVALID_ARGUMENT);
  request = Request(instance, SwitchGoal());
  const auto first = runtime->Execute(request);
  ASSERT_EQ(first.error().code(), msg::OK);
  EXPECT_EQ(runtime->Execute(request).operation().operation_id(),
            first.operation().operation_id());
  request.mutable_command()->mutable_arguments()->PackFrom(SwitchGoal(false));
  EXPECT_EQ(runtime->Execute(request).error().code(), msg::REQUEST_CONFLICT);
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
  Complete(first.operation().operation_id());
  EXPECT_EQ(stream->WriteCount(), 1);
}
TEST_F(RuntimeTest, ConcurrentRequestsAdmitOnlyOneOrdinaryOperation) {
  Start();
  std::vector<std::future<msg::ExecuteResponse>> requests;
  for (int i = 0; i < 12; ++i) {
    requests.push_back(std::async(std::launch::async, [&, i] {
      return runtime->Execute(
          Request(instance, SwitchGoal(), std::to_string(i)));
    }));
  }
  int admitted = 0;
  std::string id;
  for (auto& pending : requests) {
    const auto response = pending.get();
    if (response.error().code() == msg::OK) {
      ++admitted;
      id = response.operation().operation_id();
    } else {
      EXPECT_EQ(response.error().code(), msg::BUSY);
    }
  }
  ASSERT_EQ(admitted, 1);
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
  Complete(id);
}
TEST_F(RuntimeTest, RejectsTypeMissingValueUnknownActionAndChangedRuntime) {
  Start();
  msg::LiftGoal wrong;
  wrong.set_target(msg::LiftGoal::RAISED);
  EXPECT_EQ(runtime->Execute(Request(instance, wrong)).error().code(),
            msg::INVALID_ARGUMENT);
  EXPECT_EQ(
      runtime->Execute(Request(instance, msg::SwitchGoal())).error().code(),
      msg::INVALID_ARGUMENT);
  EXPECT_EQ(
      runtime->Execute(Request(instance, SwitchGoal(), "x", "tool", "unknown"))
          .error()
          .code(),
      msg::UNSUPPORTED_CAPABILITY);
  EXPECT_EQ(runtime->Execute(Request("previous", SwitchGoal())).error().code(),
            msg::RUNTIME_CHANGED);
  auto request = Request(instance, SwitchGoal());
  request.set_execution_timeout_ms(0);
  EXPECT_EQ(runtime->Execute(request).error().code(), msg::INVALID_ARGUMENT);
  EXPECT_EQ(stream->WriteCount(), 0);
}
TEST_F(RuntimeTest, QueryNeverRefreshesStaleFeedback) {
  auto config = Config();
  config.mutable_devices(0)->set_state_max_age_ms(20);
  Start(config);
  const auto sequence = runtime->GetState(StateRequest())
                            .state()
                            .capabilities(0)
                            .state_sequence();
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  auto state = runtime->GetState(StateRequest());
  EXPECT_EQ(state.error().code(), msg::STALE_STATE);
  EXPECT_EQ(state.state().capabilities(0).state_sequence(), sequence);
  EXPECT_EQ(runtime->Execute(Request(instance, SwitchGoal())).error().code(),
            msg::STALE_STATE);
}
TEST_F(RuntimeTest, CancellationRequiresNewStationaryFeedback) {
  Start();
  const auto response = runtime->Execute(Request(instance, SwitchGoal()));
  ASSERT_EQ(response.error().code(), msg::OK);
  const auto id = response.operation().operation_id();
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
  msg::CancelOperationRequest cancel;
  cancel.set_runtime_instance_id(instance);
  cancel.set_operation_id(id);
  ASSERT_EQ(runtime->CancelOperation(cancel).error().code(), msg::OK);
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 2; }));
  EXPECT_EQ(stream->LastCommand(), 0);
  EXPECT_EQ(Operation(id).state(), msg::CANCELING);
  Complete(id, 2);
  EXPECT_EQ(Operation(id).state(), msg::CANCELED);
}
TEST_F(RuntimeTest, TimeoutDoesNotReleaseResourceOrBecomeLateSuccess) {
  Start();
  auto request = Request(instance, SwitchGoal());
  request.set_execution_timeout_ms(30);
  const auto response = runtime->Execute(request);
  ASSERT_EQ(response.error().code(), msg::OK);
  const auto id = response.operation().operation_id();
  ASSERT_TRUE(
      WaitUntil([&] { return Operation(id).state() == msg::TIMED_OUT; }));
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 2; }));
  EXPECT_TRUE(Operation(id).error().may_have_executed());
  EXPECT_EQ(runtime->Execute(Request(instance, SwitchGoal(), "another"))
                .error()
                .code(),
            msg::BUSY);
  stream->Feedback(0x200, 2);
  ASSERT_TRUE(WaitUntil(
      [&] { return !runtime->GetState(StateRequest()).state().blocked(); }));
  EXPECT_EQ(Operation(id).state(), msg::TIMED_OUT);
}
TEST_F(RuntimeTest, UnsupportedCancelDoesNotPretendToStop) {
  Start(Config("switch", false));
  const auto response = runtime->Execute(Request(instance, SwitchGoal()));
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
  msg::CancelOperationRequest cancel;
  cancel.set_runtime_instance_id(instance);
  cancel.set_operation_id(response.operation().operation_id());
  EXPECT_EQ(runtime->CancelOperation(cancel).error().code(),
            msg::CANCEL_UNSUPPORTED);
  Complete(response.operation().operation_id());
}
TEST_F(RuntimeTest, ShortWriteIsUnknownAndShutdownCannotClaimStopped) {
  Start(Config("switch", false));
  stream->short_write = true;
  const auto response = runtime->Execute(Request(instance, SwitchGoal()));
  const auto id = response.operation().operation_id();
  ASSERT_TRUE(WaitUntil([&] { return Operation(id).state() == msg::UNKNOWN; }));
  EXPECT_TRUE(Operation(id).error().may_have_executed());
  EXPECT_EQ(runtime->Execute(Request(instance, SwitchGoal(), "another"))
                .error()
                .code(),
            msg::DEVICE_OFFLINE);
  EXPECT_TRUE(runtime->GetState(StateRequest()).state().blocked());
  EXPECT_TRUE(runtime->Shutdown().may_have_executed());
}
TEST_F(RuntimeTest, ContradictoryLimitsFailRatherThanSucceed) {
  Start();
  const auto response = runtime->Execute(Request(instance, SwitchGoal()));
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
  const auto id = response.operation().operation_id();
  Complete(id, 3);
  EXPECT_EQ(Operation(id).state(), msg::FAILED);
  EXPECT_EQ(runtime->GetState(StateRequest()).error().code(),
            msg::DEVICE_FAULT);
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 2; }));
  stream->Feedback(0x200, 2);
  ASSERT_TRUE(WaitUntil(
      [&] { return !runtime->GetState(StateRequest()).state().blocked(); }));
}
TEST_F(RuntimeTest, TransportExceptionBecomesUnknownAndOffline) {
  Start(Config("switch", false));
  const auto response = runtime->Execute(Request(instance, SwitchGoal()));
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
  stream->throw_read = true;
  const auto id = response.operation().operation_id();
  ASSERT_TRUE(WaitUntil([&] { return Operation(id).state() == msg::UNKNOWN; }));
  EXPECT_EQ(runtime->GetState(StateRequest()).error().code(),
            msg::DEVICE_OFFLINE);
  EXPECT_TRUE(runtime->Shutdown().may_have_executed());
}
TEST_F(RuntimeTest, RetentionProtectsDeduplicationThenExpiresBoundedly) {
  auto config = Config();
  config.set_max_operations(1);
  config.set_result_retention_ms(30);
  Start(config);
  const auto request = Request(instance, SwitchGoal(false));
  const auto first = runtime->Execute(request);
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(first.operation().operation_id()).state() ==
           msg::SUCCEEDED;
  }));
  EXPECT_EQ(runtime->Execute(Request(instance, SwitchGoal(false), "new"))
                .error()
                .code(),
            msg::RESOURCE_EXHAUSTED);
  EXPECT_EQ(runtime->Execute(request).operation().operation_id(),
            first.operation().operation_id());
  std::this_thread::sleep_for(std::chrono::milliseconds(40));
  EXPECT_EQ(runtime->Execute(Request(instance, SwitchGoal(false), "new"))
                .error()
                .code(),
            msg::OK);
  msg::GetOperationRequest query;
  query.set_runtime_instance_id(instance);
  query.set_operation_id(first.operation().operation_id());
  EXPECT_EQ(runtime->GetOperation(query).error().code(),
            msg::RESULT_UNAVAILABLE);
}
TEST_F(RuntimeTest, ExpiredReleasedOperationDoesNotReceiveFutureDeviceFaults) {
  auto config = Config();
  config.set_result_retention_ms(20);
  Start(config);
  auto request = Request(instance, SwitchGoal());
  request.set_execution_timeout_ms(30);
  const auto first = runtime->Execute(request);
  ASSERT_EQ(first.error().code(), msg::OK);
  const auto id = first.operation().operation_id();
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 2; }));
  EXPECT_EQ(Operation(id).state(), msg::TIMED_OUT);
  stream->Feedback(0x200, 2);
  ASSERT_TRUE(WaitUntil(
      [&] { return !runtime->GetState(StateRequest()).state().blocked(); }));
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  EXPECT_EQ(
      runtime->Execute(Request(instance, SwitchGoal(), "prune", "unknown"))
          .error()
          .code(),
      msg::NOT_FOUND);
  msg::GetOperationRequest query;
  query.set_runtime_instance_id(instance);
  query.set_operation_id(id);
  EXPECT_EQ(runtime->GetOperation(query).error().code(),
            msg::RESULT_UNAVAILABLE);
  stream->Feedback(0x200, 3);
  ASSERT_TRUE(WaitUntil([&] {
    return runtime->GetState(StateRequest()).error().code() ==
           msg::DEVICE_FAULT;
  }));
  EXPECT_EQ(runtime->GetState(StateRequest()).state().link(), msg::CONNECTED);
}
TEST_F(RuntimeTest, LiftAndOpenCloseAreBusinessTypesNotRuntimeBranches) {
  for (const std::string type : {"lift", "open_close"}) {
    Start(Config(type, true, true));
    msg::ExecuteRequest request;
    if (type == "lift") {
      msg::LiftGoal goal;
      goal.set_target(msg::LiftGoal::RAISED);
      request = Request(instance, goal);
    } else {
      msg::OpenCloseGoal goal;
      goal.set_target(msg::OpenCloseGoal::OPEN);
      request = Request(instance, goal);
    }
    const auto response = runtime->Execute(request);
    ASSERT_EQ(response.error().code(), msg::OK);
    ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 1; }));
    EXPECT_EQ(stream->LastWriteSize(), sizeof(canfd_frame));
    Complete(response.operation().operation_id(), 1, true);
    EXPECT_EQ(Operation(response.operation().operation_id()).state(),
              msg::SUCCEEDED);
    EXPECT_TRUE(IsOk(runtime->Shutdown()));
  }
}
TEST(EquipmentConfigurationTest, MissingInitialFeedbackIsUnavailable) {
  auto stream = std::make_shared<FakeStream>();
  EquipmentRuntime runtime(DeviceFactory(),
                           [stream](const StreamConfig&) { return stream; });
  ASSERT_TRUE(IsOk(runtime.Initialize(Config())));
  ASSERT_TRUE(IsOk(runtime.Start()));
  EXPECT_EQ(runtime.GetState(StateRequest()).error().code(),
            msg::STATE_UNAVAILABLE);
  EXPECT_TRUE(IsOk(runtime.Shutdown()));
}
TEST(EquipmentConfigurationTest,
     UninitializedAndInvalidConfigurationsFailExplicitly) {
  EquipmentRuntime runtime;
  EXPECT_EQ(runtime.GetState(StateRequest()).error().code(), msg::NOT_READY);
  EXPECT_EQ(runtime.GetOperation({}).error().code(), msg::NOT_READY);
  EXPECT_EQ(runtime.CancelOperation({}).error().code(), msg::NOT_READY);
  auto config = Config();
  config.set_result_retention_ms(std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(runtime.Initialize(config).code(), msg::INVALID_ARGUMENT);
  config = Config();
  config.mutable_devices(0)->set_model("unknown");
  EXPECT_EQ(runtime.Initialize(config).code(), msg::INVALID_ARGUMENT);
}
TEST(EquipmentConfigurationTest,
     ConflictingOwnershipAndAliasedBitsAreRejected) {
  auto config = Config();
  auto second = config.devices(0);
  second.set_device_id("second");
  *config.add_devices() = second;
  EquipmentRuntime conflict(DeviceFactory(), [](const StreamConfig&) {
    return std::make_shared<FakeStream>();
  });
  EXPECT_EQ(conflict.Initialize(config).code(), msg::INVALID_ARGUMENT);
  config = Config();
  CanSignalConfig mapping;
  ASSERT_TRUE(config.devices(0).model_config().UnpackTo(&mapping));
  mapping.set_moving_bit(mapping.positive_limit_bit());
  config.mutable_devices(0)->mutable_model_config()->PackFrom(mapping);
  EquipmentRuntime invalid(DeviceFactory(), [](const StreamConfig&) {
    return std::make_shared<FakeStream>();
  });
  EXPECT_EQ(invalid.Initialize(config).code(), msg::INVALID_ARGUMENT);
}
}  // namespace apollo::drivers::equipment::testing

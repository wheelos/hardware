// Copyright 2026 WheelOS. All Rights Reserved.
#include <linux/can/error.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "modules/drivers/equipment/devices/switch/switch_device.h"
#include "modules/drivers/equipment/protocols/can_signal/can_signal_backend.h"
#include "modules/drivers/equipment/transport/channel_registry.h"
#include "modules/drivers/hal/stream/can_stream_utils.h"
#include "tests/fake_stream.h"

namespace apollo::drivers::equipment::testing {
TEST(ChannelTest, BusOffSubscriptionUsesAnUnboundSocketAndIsOptIn) {
  const int fd = socket(AF_CAN, SOCK_RAW, CAN_RAW);
  ASSERT_GE(fd, 0);
  can_err_mask_t mask = 0;
  socklen_t size = sizeof(mask);
  const int initial =
      getsockopt(fd, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &mask, &size);
  const auto default_mask = mask;
  const int enabled =
      hal::internal::SetCanSocketErrorFilter(fd, CAN_ERR_BUSOFF);
  const int observed =
      getsockopt(fd, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &mask, &size);
  close(fd);
  ASSERT_EQ(initial, 0);
  EXPECT_EQ(default_mask, 0u);
  ASSERT_EQ(enabled, 0);
  ASSERT_EQ(observed, 0);
  EXPECT_EQ(mask, CAN_ERR_BUSOFF);
}
TEST(ChannelTest, OneReaderBroadcastsSharedReceiveWithoutStealingFrames) {
  auto stream = std::make_shared<FakeStream>();
  auto channel = std::make_shared<Channel>(Config().channels(0), stream);
  std::shared_ptr<CanSession> first, second, conflict;
  ASSERT_TRUE(IsOk(channel->OpenCan({0x100}, {0x200}, false, false, &first)));
  ASSERT_TRUE(IsOk(channel->OpenCan({0x101}, {0x200}, false, false, &second)));
  EXPECT_EQ(channel->OpenCan({0x100}, {0x201}, false, false, &conflict).code(),
            msg::INVALID_ARGUMENT);
  EXPECT_EQ(channel->OpenCan({0x200}, {}, false, false, &conflict).code(),
            msg::INVALID_ARGUMENT);
  EXPECT_EQ(channel->OpenCan({0x102}, {}, false, true, &conflict).code(),
            msg::INVALID_ARGUMENT);
  ASSERT_TRUE(IsOk(channel->Start()));
  stream->Feedback(0x200, 1);
  ASSERT_TRUE(
      WaitUntil([&] { return channel->Health().received_frames() == 1; }));
  CanFrame a, b;
  bool updated = false;
  ASSERT_TRUE(IsOk(first->Poll(&a, &updated)));
  ASSERT_TRUE(updated);
  ASSERT_TRUE(IsOk(second->Poll(&b, &updated)));
  ASSERT_TRUE(updated);
  EXPECT_EQ(a.data[0], 1);
  EXPECT_EQ(a.received_at, b.received_at);
  CanFrame command;
  command.id = 0x101;
  SteadyTime sent_at;
  EXPECT_EQ(first->Send(command, &sent_at).code(), msg::INVALID_ARGUMENT);
  EXPECT_TRUE(IsOk(second->Send(command, &sent_at)));
  EXPECT_EQ(stream->connections, 1u);
  EXPECT_TRUE(IsOk(channel->Shutdown()));
  EXPECT_EQ(stream->disconnections, 1u);
}
TEST(ChannelTest, OverflowIsExplicitAndOnlyAffectsTheSlowSubscriber) {
  auto config = Config().channels(0);
  config.set_receive_queue_capacity(1);
  auto stream = std::make_shared<FakeStream>();
  auto channel = std::make_shared<Channel>(config, stream);
  std::shared_ptr<CanSession> slow, fast;
  ASSERT_TRUE(IsOk(channel->OpenCan({}, {0x200}, false, false, &slow)));
  ASSERT_TRUE(IsOk(channel->OpenCan({}, {0x200}, false, false, &fast)));
  ASSERT_TRUE(IsOk(channel->Start()));
  CanFrame frame;
  bool updated = false;
  stream->Feedback(0x200, 1);
  ASSERT_TRUE(
      WaitUntil([&] { return channel->Health().received_frames() == 1; }));
  ASSERT_TRUE(IsOk(fast->Poll(&frame, &updated)));
  ASSERT_TRUE(updated);
  stream->Feedback(0x200, 2);
  ASSERT_TRUE(
      WaitUntil([&] { return channel->Health().received_frames() == 2; }));
  EXPECT_EQ(slow->Poll(&frame, &updated).code(), msg::RESOURCE_EXHAUSTED);
  EXPECT_TRUE(IsOk(fast->Poll(&frame, &updated)));
  EXPECT_TRUE(updated);
  EXPECT_EQ(frame.data[0], 2);
  EXPECT_EQ(channel->Health().status(), msg::ChannelHealth::UP);
}
TEST(ChannelTest, NamedBindingsValidateBeforeOpeningAnyStream) {
  auto config = Config();
  auto stream = std::make_shared<FakeStream>();
  ChannelRegistry registry([stream](const StreamConfig&) { return stream; });
  ASSERT_TRUE(IsOk(registry.Initialize(config)));
  ChannelBindings bindings;
  auto device = config.devices(0);
  device.mutable_bindings(0)->set_channel_id("missing");
  EXPECT_EQ(registry.Resolve(device, &bindings).code(), msg::INVALID_ARGUMENT);
  device = config.devices(0);
  *device.add_bindings() = device.bindings(0);
  EXPECT_EQ(registry.Resolve(device, &bindings).code(), msg::INVALID_ARGUMENT);
  EXPECT_EQ(stream->connections, 0u);
  auto duplicate = config.channels(0);
  duplicate.set_channel_id("alias");
  *config.add_channels() = duplicate;
  EXPECT_EQ(registry.Initialize(config).code(), msg::INVALID_ARGUMENT);
  config.mutable_channels(1)
      ->mutable_stream()
      ->mutable_can()
      ->set_interface_name("other_bus");
  EXPECT_EQ(registry.Initialize(config).code(), msg::INVALID_ARGUMENT);
}
TEST(ChannelTest, ByteStreamsRequireExclusiveReaderOrExplicitTransactions) {
  auto config = Config().channels(0);
  config.mutable_stream()->mutable_serial()->set_device("/dev/test_only");
  config.mutable_stream()->mutable_serial()->set_baud_rate(9600);
  auto stream = std::make_shared<FakeStream>();
  auto exclusive = std::make_shared<Channel>(config, stream);
  std::shared_ptr<hal::Stream> raw;
  ASSERT_TRUE(IsOk(exclusive->OpenStream(&raw)));
  EXPECT_EQ(exclusive->OpenStream(&raw).code(), msg::INVALID_ARGUMENT);
  config.set_transaction_protocol("fixture");
  auto shared = std::make_shared<Channel>(config, stream);
  std::shared_ptr<TransactionSession> a, b, conflict;
  ASSERT_TRUE(IsOk(shared->OpenTransactions("fixture", "1", &a)));
  ASSERT_TRUE(IsOk(shared->OpenTransactions("fixture", "2", &b)));
  EXPECT_EQ(shared->OpenStream(&raw).code(), msg::INVALID_ARGUMENT);
  EXPECT_EQ(shared->OpenTransactions("fixture", "1", &conflict).code(),
            msg::INVALID_ARGUMENT);
  EXPECT_EQ(shared->OpenTransactions("other", "3", &conflict).code(),
            msg::INVALID_ARGUMENT);
  ASSERT_TRUE(IsOk(shared->Start()));
  std::atomic<int> executing{0};
  std::atomic<bool> overlap{false};
  auto transaction = [&](hal::Stream&) {
    if (++executing != 1) {
      overlap = true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    --executing;
    return Error(msg::OK);
  };
  auto first =
      std::async(std::launch::async, [&] { return a->Run(transaction); });
  auto second =
      std::async(std::launch::async, [&] { return b->Run(transaction); });
  EXPECT_TRUE(IsOk(first.get()));
  EXPECT_TRUE(IsOk(second.get()));
  EXPECT_FALSE(overlap);
  EXPECT_TRUE(IsOk(shared->Shutdown()));
}
TEST(ChannelTest, DevicesShareOneCanChannelWithIndependentOperationQueues) {
  auto config = Config();
  auto other = config.devices(0);
  other.set_device_id("second");
  CanSignalAddress address;
  address.set_command_id(0x101);
  address.set_feedback_id(0x201);
  ASSERT_TRUE(other.mutable_bindings(0)->mutable_address()->PackFrom(address));
  *config.add_devices() = other;
  auto stream = std::make_shared<FakeStream>();
  EquipmentRuntime runtime(DeviceFactory(),
                           [stream](const StreamConfig&) { return stream; });
  stream->Feedback(0x200, 2);
  stream->Feedback(0x201, 2);
  ASSERT_TRUE(IsOk(runtime.Initialize(config)));
  ASSERT_TRUE(IsOk(runtime.Start()));
  auto state = StateRequest();
  state.set_device_id("second");
  ASSERT_TRUE(WaitUntil([&] {
    return IsOk(runtime.GetState(StateRequest()).error()) &&
           IsOk(runtime.GetState(state).error());
  }));
  const auto inventory = runtime.ListDevices({});
  ASSERT_EQ(inventory.channels_size(), 1);
  EXPECT_EQ(inventory.devices(0).channel_ids(0), "main_bus");
  const auto first = runtime.Execute(
      Request(inventory.runtime_instance_id(), SwitchGoal(), "a"));
  const auto second = runtime.Execute(
      Request(inventory.runtime_instance_id(), SwitchGoal(), "b", "second"));
  ASSERT_TRUE(IsOk(first.error()));
  ASSERT_TRUE(IsOk(second.error()));
  ASSERT_TRUE(WaitUntil([&] { return stream->WriteCount() == 2; }));
  stream->Feedback(0x200, 1);
  stream->Feedback(0x201, 1);
  msg::GetOperationRequest query;
  query.set_runtime_instance_id(inventory.runtime_instance_id());
  ASSERT_TRUE(WaitUntil([&] {
    query.set_operation_id(first.operation().operation_id());
    const bool a =
        runtime.GetOperation(query).operation().state() == msg::SUCCEEDED;
    query.set_operation_id(second.operation().operation_id());
    return a &&
           runtime.GetOperation(query).operation().state() == msg::SUCCEEDED;
  }));
  EXPECT_TRUE(IsOk(runtime.Shutdown()));
  EXPECT_EQ(stream->connections, 1u);
  EXPECT_EQ(stream->disconnections, 1u);
}
TEST(ChannelTest, SameIdsOnSeparateBusesAreLegalAndBusOffIsIsolated) {
  auto config = Config();
  auto second = config.channels(0);
  second.set_channel_id("second_bus");
  second.mutable_stream()->mutable_can()->set_interface_name("second_bus");
  *config.add_channels() = second;
  auto device = config.devices(0);
  device.set_device_id("second");
  device.mutable_bindings(0)->set_channel_id("second_bus");
  *config.add_devices() = device;
  auto first_stream = std::make_shared<FakeStream>();
  auto second_stream = std::make_shared<FakeStream>();
  EquipmentRuntime runtime(DeviceFactory(), [&](const StreamConfig& stream) {
    return stream.can().interface_name() == "second_bus" ? second_stream
                                                         : first_stream;
  });
  first_stream->Feedback(0x200, 2);
  second_stream->Feedback(0x200, 2);
  ASSERT_TRUE(IsOk(runtime.Initialize(config)));
  ASSERT_TRUE(IsOk(runtime.Start()));
  auto state = StateRequest();
  state.set_device_id("second");
  ASSERT_TRUE(WaitUntil([&] {
    return IsOk(runtime.GetState(StateRequest()).error()) &&
           IsOk(runtime.GetState(state).error());
  }));
  first_stream->Feedback(CAN_ERR_FLAG | CAN_ERR_BUSOFF, 0);
  ASSERT_TRUE(WaitUntil([&] {
    return runtime.GetState(StateRequest()).error().code() ==
           msg::DEVICE_OFFLINE;
  }));
  EXPECT_TRUE(IsOk(runtime.GetState(state).error()));
  EXPECT_EQ(runtime.ListDevices({}).channels(0).status(),
            msg::ChannelHealth::BUS_OFF);
  EXPECT_TRUE(IsOk(runtime.Shutdown()));
}
namespace {
class GatedReadStream final : public FakeStream {
 public:
  size_t read(uint8_t* data, size_t size) override {
    const auto count = FakeStream::read(data, size);
    if (count != 0 && block_next_read.exchange(false)) {
      reading.set_value();
      EXPECT_EQ(released_.wait_for(std::chrono::seconds(3)),
                std::future_status::ready);
    }
    return count;
  }
  std::atomic<bool> block_next_read{false};
  std::promise<void> reading;
  std::promise<void> release;

 private:
  std::future<void> released_ = release.get_future();
};
class GatedBackend final : public BinaryBackend {
 public:
  explicit GatedBackend(ChannelBinding binding)
      : backend_(std::move(binding)) {}
  msg::EquipmentError Initialize(const DeviceConfig& config) override {
    return backend_.Initialize(config);
  }
  bool SupportsStop() const override { return true; }
  msg::EquipmentError Activate() override { return backend_.Activate(); }
  msg::EquipmentError SetTarget(bool positive, SteadyTime* sent_at) override {
    if (sending) {
      sending();
    }
    return backend_.SetTarget(positive, sent_at);
  }
  msg::EquipmentError Stop(SteadyTime* sent_at) override {
    if (sending) {
      sending();
    }
    return backend_.Stop(sent_at);
  }
  msg::EquipmentError Poll(SignalFeedback* value, bool* updated) override {
    return backend_.Poll(value, updated);
  }
  msg::EquipmentError Deactivate() override { return backend_.Deactivate(); }
  std::function<void()> sending;

 private:
  CanSignalBackend backend_;
};
class TimestampBackend final : public BinaryBackend {
 public:
  msg::EquipmentError Initialize(const DeviceConfig&) override {
    return Error(msg::OK);
  }
  bool SupportsStop() const override { return true; }
  msg::EquipmentError Activate() override { return Error(msg::OK); }
  msg::EquipmentError SetTarget(bool positive, SteadyTime* sent_at) override {
    *sent_at = std::chrono::steady_clock::now();
    if (reply_on_send) {
      feedback.positive = positive;
      feedback.negative = !positive;
      feedback.received_at = std::chrono::steady_clock::now();
    }
    return Error(msg::OK);
  }
  msg::EquipmentError Stop(SteadyTime* sent_at) override {
    *sent_at = std::chrono::steady_clock::now();
    return Error(msg::OK);
  }
  msg::EquipmentError Poll(SignalFeedback* value, bool* updated) override {
    *value = feedback;
    *updated = true;
    return Error(msg::OK);
  }
  msg::EquipmentError Deactivate() override { return Error(msg::OK); }
  SignalFeedback feedback;
  bool reply_on_send = false;
};
class DualBindingBackend final : public BinaryBackend {
 public:
  explicit DualBindingBackend(const ChannelBindings& bindings)
      : control_(bindings.at("control")), diagnostic_(bindings.at("diag")) {}
  msg::EquipmentError Initialize(const DeviceConfig& config) override {
    const auto error = control_.Initialize(config);
    if (!IsOk(error)) {
      return error;
    }
    return diagnostic_.channel->OpenCan({}, {0x555}, false, false, &session_);
  }
  bool SupportsStop() const override { return true; }
  msg::EquipmentError Activate() override { return control_.Activate(); }
  msg::EquipmentError SetTarget(bool positive, SteadyTime* sent_at) override {
    return control_.SetTarget(positive, sent_at);
  }
  msg::EquipmentError Stop(SteadyTime* sent_at) override {
    return control_.Stop(sent_at);
  }
  msg::EquipmentError Poll(SignalFeedback* feedback, bool* updated) override {
    return control_.Poll(feedback, updated);
  }
  msg::EquipmentError Deactivate() override { return control_.Deactivate(); }

 private:
  CanSignalBackend control_;
  ChannelBinding diagnostic_;
  std::shared_ptr<CanSession> session_;
};
}  // namespace
class DispatchOrderingTest : public ::testing::TestWithParam<bool> {};
TEST_P(DispatchOrderingTest, ReadBeforeWriteCannotCompleteCommandOrCancel) {
  const bool cancel = GetParam();
  auto config = Config();
  auto stream = std::make_shared<GatedReadStream>();
  auto channel = std::make_shared<Channel>(config.channels(0), stream);
  ChannelBinding binding{channel, config.devices(0).bindings(0).address()};
  auto backend = std::make_unique<GatedBackend>(binding);
  auto* source = backend.get();
  SwitchDevice device(std::move(backend));
  ASSERT_TRUE(IsOk(device.Initialize(config.devices(0))));
  ASSERT_TRUE(IsOk(channel->Start()));
  ASSERT_TRUE(IsOk(device.Activate()));
  stream->Feedback(0x200, 2);
  ASSERT_TRUE(WaitUntil([&] {
    DeviceUpdate update;
    return IsOk(device.Poll(&update)) && !update.samples.empty();
  }));
  OperationContext context{
      "operation", std::chrono::steady_clock::now() + std::chrono::seconds(3),
      std::chrono::steady_clock::now()};
  const auto command = Request("instance", SwitchGoal()).command();
  if (cancel) {
    ASSERT_TRUE(IsOk(device.Begin(context, command)));
  }
  stream->block_next_read = true;
  stream->Feedback(0x200, 1);
  const auto reading =
      stream->reading.get_future().wait_for(std::chrono::seconds(3));
  std::promise<void> sending;
  auto dispatched = sending.get_future();
  source->sending = [&] { sending.set_value(); };
  auto result = std::async(std::launch::async, [&] {
    return cancel ? device.Cancel("operation") : device.Begin(context, command);
  });
  const auto dispatch = dispatched.wait_for(std::chrono::seconds(3));
  stream->release.set_value();
  ASSERT_EQ(reading, std::future_status::ready);
  ASSERT_EQ(dispatch, std::future_status::ready);
  ASSERT_TRUE(IsOk(result.get()));
  ASSERT_TRUE(
      WaitUntil([&] { return channel->Health().received_frames() == 2; }));
  DeviceUpdate stale;
  ASSERT_TRUE(IsOk(device.Poll(&stale)));
  ASSERT_EQ(stale.samples.size(), 1);
  EXPECT_TRUE(stale.operations.empty());
  EXPECT_FALSE(device.SafeToRelease());
  stream->Feedback(0x200, 1);
  ASSERT_TRUE(
      WaitUntil([&] { return channel->Health().received_frames() == 3; }));
  DeviceUpdate fresh;
  ASSERT_TRUE(IsOk(device.Poll(&fresh)));
  ASSERT_EQ(fresh.operations.size(), 1);
  EXPECT_EQ(fresh.operations[0].state, cancel ? msg::CANCELED : msg::SUCCEEDED);
  EXPECT_TRUE(device.SafeToRelease());
  EXPECT_TRUE(IsOk(device.Deactivate()));
  EXPECT_TRUE(IsOk(channel->Shutdown()));
}
INSTANTIATE_TEST_SUITE_P(CommandAndCancel, DispatchOrderingTest,
                         ::testing::Bool());
TEST(ChannelTest, ReplyReceivedBeforeBackendReturnsRemainsCompletionEvidence) {
  auto backend = std::make_unique<TimestampBackend>();
  auto* source = backend.get();
  SwitchDevice device(std::move(backend));
  ASSERT_TRUE(IsOk(device.Initialize(Config().devices(0))));
  ASSERT_TRUE(IsOk(device.Activate()));
  source->feedback.negative = true;
  source->feedback.error = Error(msg::OK);
  source->feedback.received_at = std::chrono::steady_clock::now();
  DeviceUpdate initial;
  ASSERT_TRUE(IsOk(device.Poll(&initial)));
  source->reply_on_send = true;
  OperationContext context{
      "fast", std::chrono::steady_clock::now() + std::chrono::seconds(1),
      std::chrono::steady_clock::now()};
  ASSERT_TRUE(
      IsOk(device.Begin(context, Request("instance", SwitchGoal()).command())));
  DeviceUpdate reply;
  ASSERT_TRUE(IsOk(device.Poll(&reply)));
  ASSERT_EQ(reply.operations.size(), 1);
  EXPECT_EQ(reply.operations[0].state, msg::SUCCEEDED);
}
TEST(ChannelTest,
     QueuedPreCommandAndPreCancelFeedbackCannotCompleteAnOperation) {
  auto backend = std::make_unique<TimestampBackend>();
  auto* source = backend.get();
  SwitchDevice device(std::move(backend));
  ASSERT_TRUE(IsOk(device.Initialize(Config().devices(0))));
  ASSERT_TRUE(IsOk(device.Activate()));
  source->feedback.negative = true;
  source->feedback.error = Error(msg::OK);
  source->feedback.observed_at_unix_ns = UnixTimeNs();
  source->feedback.received_at = std::chrono::steady_clock::now();
  DeviceUpdate initial;
  ASSERT_TRUE(IsOk(device.Poll(&initial)));
  OperationContext context{
      "first", std::chrono::steady_clock::now() + std::chrono::seconds(1),
      std::chrono::steady_clock::now()};
  const auto command = Request("instance", SwitchGoal()).command();
  ASSERT_TRUE(IsOk(device.Begin(context, command)));
  source->feedback.negative = false;
  source->feedback.positive = true;
  source->feedback.received_at =
      context.evidence_after - std::chrono::nanoseconds(1);
  DeviceUpdate queued;
  ASSERT_TRUE(IsOk(device.Poll(&queued)));
  EXPECT_TRUE(queued.operations.empty());
  EXPECT_FALSE(device.SafeToRelease());
  source->feedback.received_at = std::chrono::steady_clock::now();
  DeviceUpdate fresh;
  ASSERT_TRUE(IsOk(device.Poll(&fresh)));
  ASSERT_EQ(fresh.operations.size(), 1);
  EXPECT_EQ(fresh.operations[0].state, msg::SUCCEEDED);
  context.operation_id = "second";
  context.evidence_after = std::chrono::steady_clock::now();
  ASSERT_TRUE(IsOk(
      device.Begin(context, Request("instance", SwitchGoal(false)).command())));
  const auto before_cancel = context.evidence_after;
  ASSERT_TRUE(IsOk(device.Cancel("second")));
  source->feedback.received_at = before_cancel;
  DeviceUpdate before;
  ASSERT_TRUE(IsOk(device.Poll(&before)));
  EXPECT_TRUE(before.operations.empty());
  EXPECT_FALSE(device.SafeToRelease());
  source->feedback.received_at = std::chrono::steady_clock::now();
  DeviceUpdate stopped;
  ASSERT_TRUE(IsOk(device.Poll(&stopped)));
  ASSERT_EQ(stopped.operations.size(), 1);
  EXPECT_EQ(stopped.operations[0].state, msg::CANCELED);
  EXPECT_TRUE(device.SafeToRelease());
}
TEST(ChannelTest, OneDeviceCanBindControlAndDiagnosticsOnDifferentBuses) {
  auto config = Config();
  auto diagnostic = config.channels(0);
  diagnostic.set_channel_id("diag_bus");
  diagnostic.mutable_stream()->mutable_can()->set_interface_name("diag_bus");
  *config.add_channels() = diagnostic;
  auto* device = config.mutable_devices(0);
  device->set_model("dual_fixture");
  device->mutable_bindings(0)->set_name("control");
  auto* binding = device->add_bindings();
  binding->set_name("diag");
  binding->set_channel_id("diag_bus");
  DeviceFactory factory;
  ASSERT_TRUE(factory.Register(
      "switch", "dual_fixture", [](const ChannelBindings& bindings) {
        return std::make_unique<SwitchDevice>(
            std::make_unique<DualBindingBackend>(bindings));
      }));
  auto first = std::make_shared<FakeStream>();
  auto second = std::make_shared<FakeStream>();
  EquipmentRuntime runtime(std::move(factory), [&](const StreamConfig& stream) {
    return stream.can().interface_name() == "diag_bus" ? second : first;
  });
  first->Feedback(0x200, 2);
  ASSERT_TRUE(IsOk(runtime.Initialize(config)));
  ASSERT_TRUE(IsOk(runtime.Start()));
  ASSERT_TRUE(WaitUntil(
      [&] { return IsOk(runtime.GetState(StateRequest()).error()); }));
  EXPECT_EQ(runtime.ListDevices({}).devices(0).channel_ids_size(), 2);
  second->throw_read = true;
  ASSERT_TRUE(WaitUntil([&] {
    return runtime.GetState(StateRequest()).error().code() ==
           msg::DEVICE_OFFLINE;
  }));
  EXPECT_TRUE(IsOk(runtime.Shutdown()));
}
}  // namespace apollo::drivers::equipment::testing

// Copyright 2026 WheelOS. All Rights Reserved.
#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "gtest/gtest.h"
#include "modules/drivers/equipment/devices/switch/switch_device.h"
#include "tests/fake_stream.h"

namespace apollo::drivers::equipment::testing {
namespace {
// Test-only wire format; this is not a supported manufacturer protocol.
class ByteBackend final : public BinaryBackend {
 public:
  explicit ByteBackend(ChannelBinding binding) : binding_(std::move(binding)) {}
  msg::EquipmentError Initialize(const DeviceConfig&) override {
    return binding_.channel->OpenStream(&stream_);
  }
  bool SupportsStop() const override { return false; }
  msg::EquipmentError Activate() override {
    return binding_.channel->Health().error();
  }
  msg::EquipmentError SetTarget(bool positive, SteadyTime* sent_at) override {
    const uint8_t target = positive ? 1 : 2;
    *sent_at = std::chrono::steady_clock::now();
    return stream_->write(&target, 1) == 1
               ? Error(msg::OK)
               : Error(msg::TRANSPORT_ERROR, "write failed", true);
  }
  msg::EquipmentError Poll(SignalFeedback* feedback, bool* updated) override {
    *updated = false;
    const auto count =
        stream_->read(bytes_.data() + used_, bytes_.size() - used_);
    used_ += count;
    if (count == 0 && stream_->get_status() != hal::Stream::Status::CONNECTED) {
      return binding_.channel->ReportFailure(
          Error(msg::TRANSPORT_ERROR, "TCP fixture disconnected", true));
    }
    if (used_ != bytes_.size()) {
      return Error(msg::OK);
    }
    used_ = 0;
    if (bytes_[0] != 0xA5 || (bytes_[1] != 1 && bytes_[1] != 2) ||
        bytes_[2] > 1) {
      return Error(msg::TRANSPORT_ERROR, "invalid test framing");
    }
    feedback->received_at = std::chrono::steady_clock::now();
    feedback->observed_at_unix_ns = UnixTimeNs();
    feedback->positive = bytes_[1] == 1;
    feedback->negative = bytes_[1] == 2;
    feedback->moving = bytes_[2] != 0;
    feedback->error = Error(msg::OK);
    *updated = true;
    return Error(msg::OK);
  }
  msg::EquipmentError Stop(SteadyTime*) override {
    return Error(msg::CANCEL_UNSUPPORTED, "test peer has no stop command");
  }
  msg::EquipmentError Deactivate() override { return Error(msg::OK); }

 private:
  std::shared_ptr<hal::Stream> stream_;
  ChannelBinding binding_;
  std::array<uint8_t, 3> bytes_{};
  size_t used_ = 0;
};

class LoopbackPeer {
 public:
  LoopbackPeer() {
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
      return;
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) !=
            0 ||
        listen(fd_, 1) != 0) {
      return;
    }
    socklen_t size = sizeof(address);
    if (getsockname(fd_, reinterpret_cast<sockaddr*>(&address), &size) != 0) {
      return;
    }
    port = ntohs(address.sin_port);
    worker_ = std::thread([this] {
      pollfd descriptor{fd_, POLLIN, 0};
      if (poll(&descriptor, 1, 3000) <= 0) {
        return;
      }
      const int peer = accept(fd_, nullptr, nullptr);
      if (peer < 0) {
        return;
      }
      const uint8_t prefix = 0xA5;
      const uint8_t initial[] = {2, 0};
      bool valid = send(peer, &prefix, 1, MSG_NOSIGNAL) == 1 &&
                   send(peer, initial, sizeof(initial), MSG_NOSIGNAL) ==
                       static_cast<ssize_t>(sizeof(initial));
      pollfd incoming{peer, POLLIN, 0};
      uint8_t command = 0;
      valid = valid && poll(&incoming, 1, 3000) > 0 &&
              recv(peer, &command, 1, 0) == 1 && command == 1;
      if (valid) {
        const uint8_t feedback[] = {0xA5, 1, 0};
        valid = send(peer, feedback, sizeof(feedback), MSG_NOSIGNAL) ==
                static_cast<ssize_t>(sizeof(feedback));
      }
      correct_command = valid;
      poll(&incoming, 1, 3000);
      close(peer);
    });
  }
  ~LoopbackPeer() {
    if (worker_.joinable()) {
      worker_.join();
    }
    if (fd_ >= 0) {
      close(fd_);
    }
  }
  uint16_t port = 0;
  std::atomic<bool> correct_command{false};

 private:
  int fd_ = -1;
  std::thread worker_;
};
}  // namespace

TEST(EquipmentStreamTest, RealHalTcpStreamPreservesUpperCapabilityContract) {
  LoopbackPeer peer;
  ASSERT_NE(peer.port, 0);
  auto config = Config();
  auto* device = config.mutable_devices(0);
  device->set_model("test_byte");
  device->clear_model_config();
  auto* channel = config.mutable_channels(0);
  channel->mutable_stream()->mutable_tcp()->set_address("127.0.0.1");
  channel->mutable_stream()->mutable_tcp()->set_port(peer.port);
  DeviceFactory factory;
  ASSERT_TRUE(factory.Register(
      "switch", "test_byte", [](const ChannelBindings& bindings) {
        return std::make_unique<SwitchDevice>(
            std::make_unique<ByteBackend>(bindings.at("main")));
      }));
  EquipmentRuntime runtime(std::move(factory));
  ASSERT_TRUE(IsOk(runtime.Initialize(config)));
  ASSERT_TRUE(IsOk(runtime.Start()));
  ASSERT_TRUE(WaitUntil(
      [&] { return IsOk(runtime.GetState(StateRequest()).error()); }));
  const auto request =
      Request(runtime.ListDevices({}).runtime_instance_id(), SwitchGoal());
  const auto response = runtime.Execute(request);
  ASSERT_EQ(response.error().code(), msg::OK);
  msg::GetOperationRequest query;
  query.set_runtime_instance_id(request.runtime_instance_id());
  query.set_operation_id(response.operation().operation_id());
  ASSERT_TRUE(WaitUntil([&] {
    return runtime.GetOperation(query).operation().state() == msg::SUCCEEDED;
  }));
  EXPECT_TRUE(peer.correct_command.load());
  EXPECT_TRUE(IsOk(runtime.Shutdown()));
}
}  // namespace apollo::drivers::equipment::testing

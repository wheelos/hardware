// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <linux/can.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <deque>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "modules/drivers/equipment/equipment_runtime.h"
#include "wheelos_msgs/equipment_msgs/capabilities/lift.pb.h"
#include "wheelos_msgs/equipment_msgs/capabilities/open_close.pb.h"
#include "wheelos_msgs/equipment_msgs/capabilities/switch.pb.h"

namespace apollo::drivers::equipment::testing {
class FakeStream : public hal::Stream {
 public:
  bool Connect() override {
    ++connections;
    status_ = Status::CONNECTED;
    return true;
  }
  bool Disconnect() override {
    ++disconnections;
    status_ = Status::DISCONNECTED;
    return true;
  }
  size_t read(uint8_t* data, size_t size) override {
    if (throw_read.load()) {
      throw std::runtime_error("injected read failure");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (incoming_.empty()) {
      last_errno_ = EAGAIN;
      return 0;
    }
    auto& frame = incoming_.front();
    const auto count = std::min(size, frame.size());
    std::memcpy(data, frame.data(), count);
    incoming_.pop_front();
    last_errno_ = 0;
    return count;
  }
  size_t write(const uint8_t* data, size_t size) override {
    std::lock_guard<std::mutex> lock(mutex_);
    writes_.emplace_back(data, data + size);
    return short_write.load() ? 0 : size;
  }
  void Feedback(uint32_t id, uint8_t bits, bool fd = false) {
    canfd_frame frame{};
    frame.can_id = id;
    frame.len = 1;
    frame.data[0] = bits;
    const auto* data = reinterpret_cast<const uint8_t*>(&frame);
    std::lock_guard<std::mutex> lock(mutex_);
    incoming_.emplace_back(
        data, data + (fd ? sizeof(canfd_frame) : sizeof(can_frame)));
  }
  size_t WriteCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return writes_.size();
  }
  uint8_t LastCommand() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (writes_.empty()) {
      throw std::runtime_error("no command written");
    }
    canfd_frame frame{};
    std::memcpy(&frame, writes_.back().data(), writes_.back().size());
    return frame.data[0];
  }
  size_t LastWriteSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return writes_.empty() ? 0 : writes_.back().size();
  }
  std::atomic<bool> throw_read{false};
  std::atomic<bool> short_write{false};
  std::atomic<unsigned> connections{0};
  std::atomic<unsigned> disconnections{0};

 private:
  mutable std::mutex mutex_;
  std::deque<std::vector<uint8_t>> incoming_;
  std::vector<std::vector<uint8_t>> writes_;
};

inline bool WaitUntil(const std::function<bool()>& predicate) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (std::chrono::steady_clock::now() < deadline) {
    if (predicate()) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  return predicate();
}

inline EquipmentModuleConfig Config(const std::string& type = "switch",
                                    bool stop = true, bool fd = false) {
  EquipmentModuleConfig result;
  result.set_max_operations(32);
  result.set_result_retention_ms(1000);
  result.set_max_execution_timeout_ms(5000);
  result.set_shutdown_timeout_ms(100);
  auto* channel = result.add_channels();
  channel->set_channel_id("main_bus");
  channel->mutable_stream()->set_timeout_usec(1000);
  channel->mutable_stream()->mutable_can()->set_interface_name("test_only_can");
  channel->mutable_stream()->mutable_can()->set_enable_can_fd(fd);
  auto* device = result.add_devices();
  device->set_device_id("tool");
  device->set_device_type(type);
  device->set_model("can_signal");
  device->set_capability_id("main");
  device->set_state_max_age_ms(1000);
  device->set_poll_interval_ms(2);
  auto* binding = device->add_bindings();
  binding->set_name("main");
  binding->set_channel_id("main_bus");
  CanSignalAddress address;
  address.set_command_id(0x100);
  address.set_feedback_id(0x200);
  address.set_requires_can_fd(fd);
  binding->mutable_address()->PackFrom(address);
  CanSignalConfig cap;
  cap.set_feedback_length(1);
  cap.set_positive_command(std::string(1, '\x01'));
  cap.set_negative_command(std::string(1, '\x02'));
  if (stop) {
    cap.set_stop_command(std::string(1, '\0'));
  }
  cap.set_positive_limit_bit(0);
  cap.set_negative_limit_bit(1);
  cap.set_moving_bit(2);
  cap.set_fault_bit(3);
  device->mutable_model_config()->PackFrom(cap);
  return result;
}

inline msg::GetStateRequest StateRequest(uint32_t max_age = 1000) {
  msg::GetStateRequest request;
  request.set_device_id("tool");
  request.set_max_age_ms(max_age);
  return request;
}
template <typename Goal>
msg::ExecuteRequest Request(const std::string& instance, const Goal& goal,
                            const std::string& key = "request",
                            const std::string& device = "tool",
                            const std::string& action = "set") {
  msg::ExecuteRequest request;
  request.set_runtime_instance_id(instance);
  request.set_device_id(device);
  request.mutable_request_key()->set_caller_id("test");
  request.mutable_request_key()->set_request_id(key);
  request.set_execution_timeout_ms(2000);
  request.mutable_command()->set_capability_id("main");
  request.mutable_command()->set_action(action);
  request.mutable_command()->mutable_arguments()->PackFrom(goal);
  return request;
}
inline msg::SwitchGoal SwitchGoal(bool enabled = true) {
  msg::SwitchGoal goal;
  goal.set_enabled(enabled);
  return goal;
}
}  // namespace apollo::drivers::equipment::testing

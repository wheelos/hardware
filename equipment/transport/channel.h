// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <array>
#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "modules/drivers/equipment/device.h"
#include "modules/drivers/hal/stream/stream.h"

namespace apollo::drivers::equipment {
struct CanFrame {
  uint32_t id = 0;
  bool extended = false;
  uint8_t length = 0;
  std::array<uint8_t, 64> data{};
  SteadyTime received_at;
  uint64_t observed_at_unix_ns = 0;
};
class Channel;
class CanSession {
 public:
  // Dispatch and receipt timestamps are ordered under the channel I/O lock.
  msg::EquipmentError Send(const CanFrame& frame, SteadyTime* sent_at);
  msg::EquipmentError Poll(CanFrame* frame, bool* updated);

 private:
  friend class Channel;
  CanSession(std::shared_ptr<Channel> channel, std::set<uint64_t> transmit);
  std::shared_ptr<Channel> channel_;
  std::set<uint64_t> transmit_;
  std::mutex mutex_;
  std::deque<CanFrame> queue_;
  msg::EquipmentError error_;
};
class TransactionSession {
 public:
  using Transaction = std::function<msg::EquipmentError(hal::Stream&)>;
  msg::EquipmentError Run(const Transaction& transaction);

 private:
  friend class Channel;
  explicit TransactionSession(std::shared_ptr<Channel> channel);
  std::shared_ptr<Channel> channel_;
};

// Owns transport lifecycle. CAN has one reader; byte protocols either have one
// exclusive reader or explicitly serialize complete protocol transactions.
class Channel : public std::enable_shared_from_this<Channel> {
 public:
  Channel(ChannelConfig config, std::shared_ptr<hal::Stream> stream);
  ~Channel();
  const ChannelConfig& config() const { return config_; }
  msg::EquipmentError OpenCan(const std::vector<uint32_t>& transmit,
                              const std::vector<uint32_t>& receive,
                              bool extended, bool requires_fd,
                              std::shared_ptr<CanSession>* session);
  msg::EquipmentError OpenStream(std::shared_ptr<hal::Stream>* stream);
  msg::EquipmentError OpenTransactions(
      const std::string& protocol, const std::string& address,
      std::shared_ptr<TransactionSession>* session);
  msg::EquipmentError Start();
  msg::EquipmentError Shutdown();
  msg::ChannelHealth Health() const;
  msg::EquipmentError ReportFailure(const msg::EquipmentError& error,
                                    bool bus_off = false);

 private:
  friend class CanSession;
  friend class TransactionSession;
  static uint64_t Key(uint32_t id, bool extended);
  msg::EquipmentError Send(const CanFrame& frame, SteadyTime* sent_at);
  msg::EquipmentError Status() const;
  void Receive();
  ChannelConfig config_;
  std::shared_ptr<hal::Stream> stream_;
  mutable std::mutex mutex_;
  std::mutex io_mutex_;
  msg::ChannelHealth health_;
  std::map<uint64_t, std::vector<std::weak_ptr<CanSession>>> routes_;
  std::set<uint64_t> transmit_;
  std::set<uint64_t> receive_;
  std::set<std::string> addresses_;
  bool exclusive_ = false;
  bool started_ = false;
  std::atomic<bool> running_{false};
  std::thread reader_;
};
struct ChannelBinding {
  std::shared_ptr<Channel> channel;
  google::protobuf::Any address;
};
using ChannelBindings = std::map<std::string, ChannelBinding>;
}  // namespace apollo::drivers::equipment

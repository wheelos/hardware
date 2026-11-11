// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/transport/channel.h"

#include <linux/can.h>
#include <linux/can/error.h>

#include <cerrno>
#include <cstring>
#include <exception>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "cyber/common/log.h"

namespace apollo::drivers::equipment {
CanSession::CanSession(std::shared_ptr<Channel> channel,
                       std::set<uint64_t> transmit)
    : channel_(std::move(channel)), transmit_(std::move(transmit)) {
  error_ = Error(msg::OK);
}
msg::EquipmentError CanSession::Send(const CanFrame& frame,
                                     SteadyTime* sent_at) {
  *sent_at = SteadyTime::max();
  if (!transmit_.count(Channel::Key(frame.id, frame.extended))) {
    return Error(msg::INVALID_ARGUMENT, "CAN ID is not owned by this session");
  }
  return channel_->Send(frame, sent_at);
}
msg::EquipmentError CanSession::Poll(CanFrame* frame, bool* updated) {
  *updated = false;
  const auto status = channel_->Status();
  if (!IsOk(status)) {
    return status;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (!IsOk(error_)) {
    return error_;
  }
  if (!queue_.empty()) {
    *frame = queue_.front();
    queue_.pop_front();
    *updated = true;
  }
  return Error(msg::OK);
}
TransactionSession::TransactionSession(std::shared_ptr<Channel> channel)
    : channel_(std::move(channel)) {}
msg::EquipmentError TransactionSession::Run(const Transaction& transaction) {
  if (!transaction) {
    return Error(msg::INVALID_ARGUMENT, "missing protocol transaction");
  }
  std::lock_guard<std::mutex> io(channel_->io_mutex_);
  const auto status = channel_->Status();
  if (!IsOk(status)) {
    return status;
  }
  try {
    const auto error = transaction(*channel_->stream_);
    return error.code() == msg::TRANSPORT_ERROR ? channel_->ReportFailure(error)
                                                : error;
  } catch (const std::exception& exception) {
    return channel_->ReportFailure(
        Error(msg::TRANSPORT_ERROR, exception.what(), true));
  }
}
Channel::Channel(ChannelConfig config, std::shared_ptr<hal::Stream> stream)
    : config_(std::move(config)), stream_(std::move(stream)) {
  health_.set_channel_id(config_.channel_id());
  health_.set_status(msg::ChannelHealth::DOWN);
  *health_.mutable_error() = Error(msg::NOT_READY, "channel not started");
}
Channel::~Channel() {
  const auto error = Shutdown();
  if (!IsOk(error)) {
    AERROR << config_.channel_id() << ": " << error.ShortDebugString();
  }
}
uint64_t Channel::Key(uint32_t id, bool extended) {
  return static_cast<uint64_t>(extended) << 32 | id;
}
msg::EquipmentError Channel::OpenCan(const std::vector<uint32_t>& transmit,
                                     const std::vector<uint32_t>& receive,
                                     bool extended, bool requires_fd,
                                     std::shared_ptr<CanSession>* session) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (started_ || !config_.stream().has_can() ||
      (transmit.empty() && receive.empty()) ||
      (requires_fd && !config_.stream().can().enable_can_fd())) {
    return Error(msg::INVALID_ARGUMENT, "incompatible CAN channel or claims");
  }
  std::set<uint64_t> tx;
  std::set<uint64_t> rx;
  const auto mask = extended ? CAN_EFF_MASK : CAN_SFF_MASK;
  for (const auto id : transmit) {
    const auto key = Key(id, extended);
    if (id > mask || !tx.insert(key).second || transmit_.count(key) ||
        receive_.count(key)) {
      return Error(msg::INVALID_ARGUMENT, "CAN transmit ownership conflict");
    }
  }
  for (const auto id : receive) {
    const auto key = Key(id, extended);
    if (id > mask || !rx.insert(key).second || tx.count(key) ||
        transmit_.count(key)) {
      return Error(msg::INVALID_ARGUMENT, "invalid CAN receive claim");
    }
  }
  auto result =
      std::shared_ptr<CanSession>(new CanSession(shared_from_this(), tx));
  transmit_.insert(tx.begin(), tx.end());
  receive_.insert(rx.begin(), rx.end());
  for (const auto key : rx) {
    routes_[key].push_back(result);
  }
  *session = std::move(result);
  return Error(msg::OK);
}
msg::EquipmentError Channel::OpenStream(std::shared_ptr<hal::Stream>* stream) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (started_ || config_.stream().has_can() || exclusive_ ||
      !config_.transaction_protocol().empty()) {
    return Error(msg::INVALID_ARGUMENT, "byte stream requires one reader");
  }
  exclusive_ = true;
  *stream = stream_;
  return Error(msg::OK);
}
msg::EquipmentError Channel::OpenTransactions(
    const std::string& protocol, const std::string& address,
    std::shared_ptr<TransactionSession>* session) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (started_ || config_.stream().has_can() || exclusive_ ||
      protocol.empty() || protocol != config_.transaction_protocol() ||
      address.empty() || !addresses_.insert(address).second) {
    return Error(msg::INVALID_ARGUMENT,
                 "incompatible protocol or duplicate transaction address");
  }
  *session = std::shared_ptr<TransactionSession>(
      new TransactionSession(shared_from_this()));
  return Error(msg::OK);
}
msg::ChannelHealth Channel::Health() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return health_;
}
msg::EquipmentError Channel::Status() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return health_.error();
}
msg::EquipmentError Channel::ReportFailure(const msg::EquipmentError& error,
                                           bool bus_off) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!error.has_code() || IsOk(error)) {
    return Error(msg::INVALID_ARGUMENT, "channel failure requires an error");
  }
  auto reported = error;
  reported.set_message(config_.channel_id() + ": " + error.message());
  if (health_.status() == msg::ChannelHealth::UP) {
    health_.set_status(bus_off ? msg::ChannelHealth::BUS_OFF
                               : msg::ChannelHealth::FAULT);
    *health_.mutable_error() = reported;
    AERROR << reported.ShortDebugString();
  }
  return reported;
}
msg::EquipmentError Channel::Start() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (started_) {
    return Error(msg::NOT_READY, "channel cannot be restarted");
  }
  started_ = true;
  if (!exclusive_ && addresses_.empty() && transmit_.empty() &&
      receive_.empty()) {
    return Error(msg::OK);  // An unused configured channel is never opened.
  }
  try {
    if (!stream_->Connect()) {
      health_.set_status(msg::ChannelHealth::FAULT);
      *health_.mutable_error() =
          Error(msg::TRANSPORT_ERROR,
                "channel connect failed: " + config_.channel_id());
      return health_.error();
    }
    health_.set_status(msg::ChannelHealth::UP);
    *health_.mutable_error() = Error(msg::OK);
    if (config_.stream().has_can()) {
      running_ = true;
      reader_ = std::thread(&Channel::Receive, this);
    }
  } catch (const std::exception& exception) {
    health_.set_status(msg::ChannelHealth::FAULT);
    *health_.mutable_error() = Error(
        msg::TRANSPORT_ERROR, config_.channel_id() + ": " + exception.what());
    running_ = false;
    return health_.error();
  }
  return Error(msg::OK);
}
msg::EquipmentError Channel::Shutdown() {
  running_ = false;
  if (reader_.joinable()) {
    reader_.join();
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (!started_ || health_.status() == msg::ChannelHealth::DOWN) {
    return Error(msg::OK);
  }
  try {
    if (!stream_->Disconnect()) {
      health_.set_status(msg::ChannelHealth::FAULT);
      *health_.mutable_error() =
          Error(msg::TRANSPORT_ERROR,
                "channel disconnect failed: " + config_.channel_id());
      return health_.error();
    }
  } catch (const std::exception& exception) {
    health_.set_status(msg::ChannelHealth::FAULT);
    *health_.mutable_error() = Error(
        msg::TRANSPORT_ERROR, config_.channel_id() + ": " + exception.what());
    return health_.error();
  }
  health_.set_status(msg::ChannelHealth::DOWN);
  *health_.mutable_error() = Error(msg::NOT_READY, "channel stopped");
  return Error(msg::OK);
}
msg::EquipmentError Channel::Send(const CanFrame& frame, SteadyTime* sent_at) {
  const bool fd = config_.stream().can().enable_can_fd();
  const auto size = frame.length;
  if (size > 64 ||
      (size > 8 &&
       (!fd || (size != 12 && size != 16 && size != 20 && size != 24 &&
                size != 32 && size != 48 && size != 64)))) {
    return Error(msg::INVALID_ARGUMENT, "invalid CAN payload length");
  }
  std::lock_guard<std::mutex> io(io_mutex_);
  const auto status = Status();
  if (!IsOk(status)) {
    return status;
  }
  canfd_frame wire{};
  wire.can_id = frame.id | (frame.extended ? CAN_EFF_FLAG : 0);
  wire.len = size;
  std::memcpy(wire.data, frame.data.data(), size);
  const size_t slot = fd ? sizeof(canfd_frame) : sizeof(can_frame);
  *sent_at = std::chrono::steady_clock::now();
  try {
    if (stream_->write(reinterpret_cast<const uint8_t*>(&wire), slot) != slot) {
      return ReportFailure(
          Error(msg::TRANSPORT_ERROR, "incomplete CAN write", true));
    }
  } catch (const std::exception& exception) {
    return ReportFailure(Error(msg::TRANSPORT_ERROR, exception.what(), true));
  }
  std::lock_guard<std::mutex> lock(mutex_);
  health_.set_transmitted_frames(health_.transmitted_frames() + 1);
  return Error(msg::OK);
}
void Channel::Receive() {
  try {
    while (running_) {
      canfd_frame wire{};
      const size_t slot = config_.stream().can().enable_can_fd()
                              ? sizeof(canfd_frame)
                              : sizeof(can_frame);
      size_t size;
      SteadyTime received_at;
      uint64_t observed_at_unix_ns = 0;
      {
        std::lock_guard<std::mutex> io(io_mutex_);
        size = stream_->read(reinterpret_cast<uint8_t*>(&wire), slot);
        if (size != 0) {
          received_at = std::chrono::steady_clock::now();
          observed_at_unix_ns = UnixTimeNs();
        }
        const auto error = stream_->get_last_error_code();
        if (size == 0 &&
            (stream_->get_status() != hal::Stream::Status::CONNECTED ||
             (error != 0 && error != EAGAIN && error != EWOULDBLOCK &&
              error != EINTR && error != ETIMEDOUT))) {
          ReportFailure(Error(msg::TRANSPORT_ERROR, "CAN read failed", true));
          return;
        }
      }
      if (size != 0) {
        if (size != slot || wire.len > (slot == sizeof(can_frame) ? 8 : 64)) {
          ReportFailure(Error(msg::TRANSPORT_ERROR, "invalid CAN frame", true));
          return;
        }
        if (wire.can_id & CAN_ERR_FLAG) {
          ReportFailure(Error(msg::TRANSPORT_ERROR, "CAN error frame", true),
                        (wire.can_id & CAN_ERR_BUSOFF) != 0);
          return;
        }
        if (!(wire.can_id & CAN_RTR_FLAG)) {
          CanFrame frame;
          frame.extended = (wire.can_id & CAN_EFF_FLAG) != 0;
          frame.id =
              wire.can_id & (frame.extended ? CAN_EFF_MASK : CAN_SFF_MASK);
          frame.length = wire.len;
          std::memcpy(frame.data.data(), wire.data, wire.len);
          frame.received_at = received_at;
          frame.observed_at_unix_ns = observed_at_unix_ns;
          std::lock_guard<std::mutex> lock(mutex_);
          health_.set_received_frames(health_.received_frames() + 1);
          const auto route = routes_.find(Key(frame.id, frame.extended));
          if (route != routes_.end()) {
            for (const auto& subscriber : route->second) {
              if (const auto session = subscriber.lock()) {
                std::lock_guard<std::mutex> queue(session->mutex_);
                if (session->queue_.size() >=
                    config_.receive_queue_capacity()) {
                  session->error_ =
                      Error(msg::RESOURCE_EXHAUSTED,
                            "CAN subscriber queue overflow", true);
                  session->queue_.clear();
                } else if (IsOk(session->error_)) {
                  session->queue_.push_back(frame);
                }
              }
            }
          }
        }
      }
      if (!IsOk(Status())) {
        return;
      }
      if (size == 0) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.poll_interval_ms()));
      }
    }
  } catch (const std::exception& exception) {
    ReportFailure(Error(msg::TRANSPORT_ERROR, exception.what(), true));
  }
}
}  // namespace apollo::drivers::equipment

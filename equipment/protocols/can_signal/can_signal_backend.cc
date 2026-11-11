// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/protocols/can_signal/can_signal_backend.h"

#include <cstring>
#include <memory>
#include <set>
#include <string>
#include <utility>

namespace apollo::drivers::equipment {
namespace {
bool ValidLength(size_t length, bool fd) {
  return length > 0 &&
         (length <= 8 || (fd && (length == 12 || length == 16 || length == 20 ||
                                 length == 24 || length == 32 || length == 48 ||
                                 length == 64)));
}
bool Bit(const uint8_t* data, uint32_t bit) {
  return (data[bit / 8] & (1u << (bit % 8))) != 0;
}
}  // namespace
CanSignalBackend::CanSignalBackend(ChannelBinding binding)
    : binding_(std::move(binding)) {}
msg::EquipmentError CanSignalBackend::Initialize(const DeviceConfig& config) {
  if (!binding_.channel || !binding_.address.UnpackTo(&address_) ||
      !config.model_config().UnpackTo(&mapping_)) {
    return Error(msg::INVALID_ARGUMENT,
                 "can_signal requires typed CAN mapping");
  }
  const auto& m = mapping_;
  const bool fd = binding_.channel->config().stream().can().enable_can_fd();
  if (!address_.has_command_id() || !address_.has_feedback_id() ||
      address_.command_id() == address_.feedback_id() ||
      !ValidLength(m.positive_command().size(), fd) ||
      !ValidLength(m.negative_command().size(), fd) ||
      (m.has_stop_command() && !ValidLength(m.stop_command().size(), fd)) ||
      !m.has_feedback_length() || !ValidLength(m.feedback_length(), fd) ||
      !m.has_positive_limit_bit() || !m.has_negative_limit_bit() ||
      !m.has_moving_bit()) {
    return Error(msg::INVALID_ARGUMENT, "invalid static CAN mapping");
  }
  std::set<uint32_t> bits{m.positive_limit_bit(), m.negative_limit_bit(),
                          m.moving_bit()};
  if (m.has_fault_bit()) {
    bits.insert(m.fault_bit());
  }
  if (bits.size() != (m.has_fault_bit() ? 4u : 3u) ||
      *bits.rbegin() >= m.feedback_length() * 8) {
    return Error(msg::INVALID_ARGUMENT,
                 "overlapping/out-of-range feedback bits");
  }
  return binding_.channel->OpenCan(
      {address_.command_id()}, {address_.feedback_id()}, address_.extended_id(),
      address_.requires_can_fd(), &session_);
}
bool CanSignalBackend::SupportsStop() const {
  return mapping_.has_stop_command();
}
msg::EquipmentError CanSignalBackend::Activate() {
  const auto health = binding_.channel->Health();
  return health.error();
}
msg::EquipmentError CanSignalBackend::Send(const std::string& payload,
                                           SteadyTime* sent_at) {
  CanFrame frame;
  frame.id = address_.command_id();
  frame.extended = address_.extended_id();
  frame.length = payload.size();
  std::memcpy(frame.data.data(), payload.data(), payload.size());
  return session_->Send(frame, sent_at);
}
msg::EquipmentError CanSignalBackend::SetTarget(bool positive,
                                                SteadyTime* sent_at) {
  return Send(
      positive ? mapping_.positive_command() : mapping_.negative_command(),
      sent_at);
}
msg::EquipmentError CanSignalBackend::Poll(SignalFeedback* feedback,
                                           bool* updated) {
  *updated = false;
  CanFrame frame;
  const auto error = session_->Poll(&frame, updated);
  if (!IsOk(error) || !*updated) {
    return error;
  }
  if (frame.length != mapping_.feedback_length()) {
    return Error(msg::TRANSPORT_ERROR, "wrong CAN feedback length", true);
  }
  feedback->positive = Bit(frame.data.data(), mapping_.positive_limit_bit());
  feedback->negative = Bit(frame.data.data(), mapping_.negative_limit_bit());
  feedback->moving = Bit(frame.data.data(), mapping_.moving_bit());
  feedback->received_at = frame.received_at;
  feedback->observed_at_unix_ns = frame.observed_at_unix_ns;
  feedback->error =
      (feedback->positive && feedback->negative) ||
              (mapping_.has_fault_bit() &&
               Bit(frame.data.data(), mapping_.fault_bit()))
          ? Error(msg::DEVICE_FAULT, "device fault or contradictory limits")
          : Error(msg::OK);
  *updated = true;
  return Error(msg::OK);
}
msg::EquipmentError CanSignalBackend::Stop(SteadyTime* sent_at) {
  return SupportsStop() ? Send(mapping_.stop_command(), sent_at)
                        : Error(msg::CANCEL_UNSUPPORTED, "no stop payload");
}
msg::EquipmentError CanSignalBackend::Deactivate() { return Error(msg::OK); }
}  // namespace apollo::drivers::equipment

// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/devices/binary_device.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "wheelos_msgs/equipment_msgs/capabilities/lift.pb.h"
#include "wheelos_msgs/equipment_msgs/capabilities/open_close.pb.h"
#include "wheelos_msgs/equipment_msgs/capabilities/switch.pb.h"

namespace apollo::drivers::equipment {
BinaryDevice::BinaryDevice(Kind kind, std::unique_ptr<BinaryBackend> backend)
    : kind_(kind), backend_(std::move(backend)) {}

msg::EquipmentError BinaryDevice::Initialize(const DeviceConfig& config) {
  if (!backend_ || config.capability_id().empty()) {
    return Error(msg::INVALID_ARGUMENT,
                 "missing binary device backend/identity");
  }
  const auto error = backend_->Initialize(config);
  if (!IsOk(error)) {
    return error;
  }
  config_ = config;
  descriptor_.set_device_id(config.device_id());
  descriptor_.set_device_type(config.device_type());
  descriptor_.set_model(config.model());
  auto* cap = descriptor_.add_capabilities();
  cap->set_capability_id(config.capability_id());
  cap->set_type(kind_ == Kind::kSwitch ? "switch"
                : kind_ == Kind::kLift ? "lift"
                                       : "open_close");
  cap->set_version(1);
  auto* action = cap->add_actions();
  action->set_name("set");
  action->set_request_type(
      kind_ == Kind::kSwitch ? msg::SwitchGoal::descriptor()->full_name()
      : kind_ == Kind::kLift ? msg::LiftGoal::descriptor()->full_name()
                             : msg::OpenCloseGoal::descriptor()->full_name());
  cap->set_state_type(
      kind_ == Kind::kSwitch ? msg::SwitchState::descriptor()->full_name()
      : kind_ == Kind::kLift ? msg::LiftState::descriptor()->full_name()
                             : msg::OpenCloseState::descriptor()->full_name());
  action->set_result_type(cap->state_type());
  action->set_supports_cancel(backend_->SupportsStop());
  action->set_supports_completion_feedback(backend_->SupportsCompletion());
  return Error(msg::OK);
}
msg::DeviceDescriptor BinaryDevice::Describe() const { return descriptor_; }
msg::EquipmentError BinaryDevice::Activate() {
  const auto error = backend_->Activate();
  std::lock_guard<std::mutex> lock(mutex_);
  connected_ = IsOk(error);
  return error;
}
bool BinaryDevice::Fresh() const {
  return connected_ && have_feedback_ &&
         std::chrono::steady_clock::now() - feedback_.received_at <=
             std::chrono::milliseconds(config_.state_max_age_ms());
}
msg::EquipmentError BinaryDevice::Decode(const msg::DeviceCommand& command,
                                         bool* positive) const {
  if (kind_ == Kind::kSwitch) {
    msg::SwitchGoal goal;
    if (command.arguments().UnpackTo(&goal) && goal.has_enabled()) {
      *positive = goal.enabled();
      return Error(msg::OK);
    }
  } else if (kind_ == Kind::kLift) {
    msg::LiftGoal goal;
    if (command.arguments().UnpackTo(&goal) &&
        (goal.target() == msg::LiftGoal::RAISED ||
         goal.target() == msg::LiftGoal::LOWERED)) {
      *positive = goal.target() == msg::LiftGoal::RAISED;
      return Error(msg::OK);
    }
  } else {
    msg::OpenCloseGoal goal;
    if (command.arguments().UnpackTo(&goal) &&
        (goal.target() == msg::OpenCloseGoal::OPEN ||
         goal.target() == msg::OpenCloseGoal::CLOSED)) {
      *positive = goal.target() == msg::OpenCloseGoal::OPEN;
      return Error(msg::OK);
    }
  }
  return Error(msg::INVALID_ARGUMENT, "invalid typed binary goal");
}
msg::EquipmentError BinaryDevice::Validate(
    const msg::DeviceCommand& command) const {
  auto error = ValidateEnvelope(command);
  bool positive = false;
  if (IsOk(error)) {
    error = Decode(command, &positive);
  }
  if (!IsOk(error)) {
    return error;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (!connected_) {
    return Error(msg::DEVICE_OFFLINE, "device is disconnected");
  }
  if (!have_feedback_) {
    return Error(msg::STATE_UNAVAILABLE, "no feedback");
  }
  if (!Fresh()) {
    return Error(msg::STALE_STATE, "feedback expired");
  }
  if (!IsOk(feedback_.error)) {
    return feedback_.error;
  }
  if (kind_ == Kind::kSwitch && !feedback_.positive && !feedback_.negative) {
    return Error(msg::STATE_UNAVAILABLE, "switch position is unknown");
  }
  return awaiting_ || feedback_.moving
             ? Error(msg::BUSY, "device is not confirmed stationary")
             : Error(msg::OK);
}
msg::EquipmentError BinaryDevice::Begin(const OperationContext& context,
                                        const msg::DeviceCommand& command) {
  auto error = Validate(command);
  if (!IsOk(error)) {
    return error;
  }
  bool positive = false;
  error = Decode(command, &positive);
  if (!IsOk(error)) {
    return error;
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    operation_ = context;
    positive_ = positive;
    stopping_ = false;
    already_ = positive ? feedback_.positive : feedback_.negative;
    awaiting_ = !already_;
    if (already_) {
      return Error(msg::OK);
    }
  }
  SteadyTime sent_at = SteadyTime::max();
  error = backend_->SetTarget(positive, &sent_at);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    operation_.evidence_after = sent_at;
    if (!IsOk(error) && !error.may_have_executed()) {
      awaiting_ = false;
      operation_.operation_id.clear();
    }
  }
  return error;
}
StateSample BinaryDevice::Sample() const {
  StateSample sample;
  sample.received_at = feedback_.received_at;
  auto& state = sample.state;
  state.set_capability_id(config_.capability_id());
  state.set_state_sequence(sequence_);
  state.set_observed_at_unix_ns(feedback_.observed_at_unix_ns);
  state.set_valid(IsOk(feedback_.error));
  state.set_stale(!Fresh());
  *state.mutable_error() = feedback_.error;
  if (kind_ == Kind::kSwitch) {
    msg::SwitchState value;
    if (feedback_.positive || feedback_.negative) {
      value.set_enabled(feedback_.positive);
    }
    value.set_transitioning(feedback_.moving);
    state.mutable_value()->PackFrom(value);
    if (!feedback_.positive && !feedback_.negative && IsOk(feedback_.error)) {
      state.set_valid(false);
      *state.mutable_error() =
          Error(msg::STATE_UNAVAILABLE, "switch position is unknown");
    }
  } else if (kind_ == Kind::kLift) {
    msg::LiftState value;
    value.set_position(feedback_.positive   ? msg::LiftState::RAISED
                       : feedback_.negative ? msg::LiftState::LOWERED
                                            : msg::LiftState::INTERMEDIATE);
    value.set_moving(feedback_.moving);
    state.mutable_value()->PackFrom(value);
  } else {
    msg::OpenCloseState value;
    value.set_position(feedback_.positive ? msg::OpenCloseState::OPEN
                       : feedback_.negative
                           ? msg::OpenCloseState::CLOSED
                           : msg::OpenCloseState::INTERMEDIATE);
    value.set_moving(feedback_.moving);
    state.mutable_value()->PackFrom(value);
  }
  return sample;
}
OperationEvent BinaryDevice::Completion(msg::OperationState state,
                                        bool already) const {
  OperationEvent event;
  event.operation_id = operation_.operation_id;
  event.state = state;
  event.evidence = Sample();
  event.error = state == msg::FAILED ? feedback_.error : Error(msg::OK);
  event.result = event.evidence.state.value();
  event.already_satisfied = already;
  return event;
}
msg::EquipmentError BinaryDevice::Poll(DeviceUpdate* update) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (already_) {
      already_ = false;
      if (Fresh() && std::chrono::steady_clock::now() <= operation_.deadline) {
        update->operations.push_back(
            Completion(stopping_ ? msg::CANCELED : msg::SUCCEEDED, true));
        operation_.operation_id.clear();
      }
    }
  }
  SignalFeedback feedback;
  bool updated = false;
  const auto error = backend_->Poll(&feedback, &updated);
  if (!IsOk(error)) {
    std::lock_guard<std::mutex> lock(mutex_);
    connected_ = false;
    return error;
  }
  if (!updated) {
    return Error(msg::OK);
  }
  std::lock_guard<std::mutex> lock(mutex_);
  feedback_ = feedback;
  have_feedback_ = true;
  ++sequence_;
  update->samples.push_back(Sample());
  const bool known =
      kind_ != Kind::kSwitch || feedback.positive || feedback.negative;
  const bool after_command = operation_.operation_id.empty() ||
                             feedback.received_at > operation_.evidence_after;
  if (after_command && IsOk(feedback.error) && !feedback.moving && known) {
    awaiting_ = false;
  }
  if (!operation_.operation_id.empty()) {
    if (!IsOk(feedback.error)) {
      update->operations.push_back(Completion(msg::FAILED, false));
    } else if (after_command && !feedback.moving && known && stopping_) {
      update->operations.push_back(Completion(msg::CANCELED, false));
      operation_.operation_id.clear();
    } else if (after_command && !feedback.moving &&
               (positive_ ? feedback.positive : feedback.negative) &&
               feedback.received_at <= operation_.deadline) {
      update->operations.push_back(Completion(msg::SUCCEEDED, false));
      operation_.operation_id.clear();
    }
  }
  return Error(msg::OK);
}
msg::EquipmentError BinaryDevice::Cancel(const std::string& operation_id) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (operation_id != operation_.operation_id) {
      return Error(msg::CANCEL_REJECTED, "operation is not owned by device");
    }
    if (already_) {
      stopping_ = true;
      return Error(msg::OK);
    }
  }
  return RequestStop();
}
msg::EquipmentError BinaryDevice::RequestStop() {
  if (!backend_->SupportsStop()) {
    return Error(msg::CANCEL_UNSUPPORTED, "device has no safe stop");
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    awaiting_ = true;
    stopping_ = true;
  }
  SteadyTime sent_at = SteadyTime::max();
  const auto error = backend_->Stop(&sent_at);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!IsOk(error) && !error.may_have_executed()) {
      stopping_ = false;
    } else {
      operation_.evidence_after = sent_at;
    }
  }
  return error;
}
bool BinaryDevice::SafeToRelease() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return Fresh() && IsOk(feedback_.error) && !feedback_.moving && !awaiting_ &&
         (kind_ != Kind::kSwitch || feedback_.positive || feedback_.negative);
}
bool BinaryDevice::HasControlLease() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return awaiting_;
}
void BinaryDevice::ForgetOperation(const std::string& operation_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (operation_.operation_id == operation_id) {
    operation_.operation_id.clear();
    already_ = false;
    stopping_ = false;
  }
}
msg::EquipmentError BinaryDevice::Deactivate() {
  const auto error = backend_->Deactivate();
  std::lock_guard<std::mutex> lock(mutex_);
  connected_ = false;
  return error;
}
}  // namespace apollo::drivers::equipment

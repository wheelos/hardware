// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/devices/charger/charger_device.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace apollo::drivers::equipment {
ChargerDevice::ChargerDevice(std::unique_ptr<ChargingBackend> backend)
    : backend_(std::move(backend)) {}
msg::EquipmentError ChargerDevice::Initialize(const DeviceConfig& config) {
  if (!backend_ || config.capability_id().empty()) {
    return Error(msg::INVALID_ARGUMENT, "missing charger backend/identity");
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
  cap->set_type("charging");
  cap->set_version(1);
  cap->set_state_type(msg::ChargingState::descriptor()->full_name());
  auto* start = cap->add_actions();
  start->set_name("start");
  start->set_request_type(msg::StartChargingGoal::descriptor()->full_name());
  start->set_result_type(msg::ChargingResult::descriptor()->full_name());
  start->set_supports_cancel(backend_->SupportsStop());
  start->set_supports_completion_feedback(true);
  if (backend_->SupportsStop()) {
    auto* stop = cap->add_actions();
    stop->set_name("stop");
    stop->set_request_type(msg::StopChargingGoal::descriptor()->full_name());
    stop->set_result_type(msg::ChargingResult::descriptor()->full_name());
    stop->set_supports_completion_feedback(true);
    stop->set_is_stop(true);
  }
  return Error(msg::OK);
}
msg::DeviceDescriptor ChargerDevice::Describe() const { return descriptor_; }
msg::EquipmentError ChargerDevice::Activate() {
  const auto error = backend_->Activate();
  std::lock_guard<std::mutex> lock(mutex_);
  connected_ = IsOk(error);
  return error;
}
bool ChargerDevice::Fresh() const {
  return connected_ && have_feedback_ &&
         std::chrono::steady_clock::now() - feedback_.received_at <=
             std::chrono::milliseconds(config_.state_max_age_ms());
}
msg::EquipmentError ChargerDevice::Validate(
    const msg::DeviceCommand& command) const {
  const auto error = ValidateEnvelope(command);
  if (!IsOk(error)) {
    return error;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (!connected_) {
    return Error(msg::DEVICE_OFFLINE, "charger is disconnected");
  }
  if (command.action() == "stop") {
    msg::StopChargingGoal goal;
    if (!command.arguments().UnpackTo(&goal) || session_.empty() ||
        goal.session_id() != session_) {
      return Error(msg::INVALID_ARGUMENT,
                   "stop requires the current session ID");
    }
    return stopping_ ? Error(msg::BUSY, "stop already pending")
                     : Error(msg::OK);
  }
  msg::StartChargingGoal goal;
  if (!command.arguments().UnpackTo(&goal)) {
    return Error(msg::INVALID_ARGUMENT, "invalid start-charging payload");
  }
  if (!have_feedback_) {
    return Error(msg::STATE_UNAVAILABLE, "no charger feedback");
  }
  if (!Fresh()) {
    return Error(msg::STALE_STATE, "charger feedback expired");
  }
  if (!IsOk(feedback_.error)) {
    return feedback_.error;
  }
  return lease_ || feedback_.value.phase() != msg::ChargingState::IDLE
             ? Error(msg::BUSY, "charger already has a session")
             : Error(msg::OK);
}
msg::EquipmentError ChargerDevice::Begin(const OperationContext& context,
                                         const msg::DeviceCommand& command) {
  const auto error = Validate(command);
  if (!IsOk(error)) {
    return error;
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (command.action() == "stop") {
      stop_ = context;
      stopping_ = true;
    } else {
      start_ = context;
      session_ = context.operation_id;
      lease_ = true;
    }
  }
  SteadyTime sent_at = SteadyTime::max();
  const auto result = command.action() == "stop"
                          ? backend_->StopCharging(&sent_at)
                          : backend_->StartCharging(&sent_at);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (IsOk(result) || result.may_have_executed()) {
      lease_evidence_after_ = sent_at;
      if (command.action() == "stop") {
        stop_->evidence_after = sent_at;
      } else {
        start_->evidence_after = sent_at;
      }
    } else if (command.action() == "stop") {
      stop_.reset();
      stopping_ = false;
    } else {
      start_.reset();
      lease_ = false;
      session_.clear();
    }
  }
  return result;
}
StateSample ChargerDevice::Sample() const {
  StateSample sample;
  sample.received_at = feedback_.received_at;
  auto& state = sample.state;
  state.set_capability_id(config_.capability_id());
  state.set_state_sequence(sequence_);
  state.set_observed_at_unix_ns(feedback_.observed_at_unix_ns);
  state.set_valid(IsOk(feedback_.error));
  state.set_stale(!Fresh());
  *state.mutable_error() = feedback_.error;
  auto value = feedback_.value;
  if (!session_.empty()) {
    value.set_session_id(session_);
  }
  state.mutable_value()->PackFrom(value);
  return sample;
}
OperationEvent ChargerDevice::Completion(const OperationContext& operation,
                                         msg::OperationState state) const {
  OperationEvent event;
  event.operation_id = operation.operation_id;
  event.state = state;
  event.evidence = Sample();
  event.error = state == msg::FAILED ? feedback_.error : Error(msg::OK);
  msg::ChargingResult result;
  result.set_session_id(session_);
  result.set_phase(feedback_.value.phase());
  event.result.PackFrom(result);
  return event;
}
msg::EquipmentError ChargerDevice::Poll(DeviceUpdate* update) {
  ChargingFeedback feedback;
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
  if (!feedback.value.has_phase() ||
      feedback.value.phase() == msg::ChargingState::PHASE_UNKNOWN) {
    return Error(msg::DEVICE_FAULT, "charger backend omitted phase", true);
  }
  std::lock_guard<std::mutex> lock(mutex_);
  feedback_ = feedback;
  if (feedback_.value.phase() == msg::ChargingState::FAULT) {
    feedback_.error =
        Error(msg::DEVICE_FAULT, "charger reported fault", lease_);
  }
  have_feedback_ = true;
  ++sequence_;
  if (!IsOk(feedback_.error)) {
    if (start_) {
      update->operations.push_back(Completion(*start_, msg::FAILED));
    }
    if (stop_) {
      update->operations.push_back(Completion(*stop_, msg::FAILED));
    }
  } else if (feedback_.received_at > lease_evidence_after_ &&
             feedback_.value.phase() == msg::ChargingState::IDLE &&
             (stopping_ || !start_)) {
    if (start_) {
      update->operations.push_back(Completion(*start_, msg::CANCELED));
      start_.reset();
    }
    if (stop_ && feedback_.received_at <= stop_->deadline) {
      update->operations.push_back(Completion(*stop_, msg::SUCCEEDED));
      stop_.reset();
    }
    lease_ = false;
    stopping_ = false;
    session_.clear();
  } else if (start_ && !stopping_ &&
             feedback_.received_at > start_->evidence_after &&
             feedback_.value.phase() == msg::ChargingState::CHARGING &&
             feedback_.received_at <= start_->deadline) {
    update->operations.push_back(Completion(*start_, msg::SUCCEEDED));
    start_.reset();
  }
  update->samples.push_back(Sample());
  return Error(msg::OK);
}
msg::EquipmentError ChargerDevice::Cancel(const std::string& operation_id) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!start_ || start_->operation_id != operation_id) {
      return Error(msg::CANCEL_UNSUPPORTED,
                   "only pending start can be canceled");
    }
  }
  return RequestStop();
}
msg::EquipmentError ChargerDevice::RequestStop() {
  if (!backend_->SupportsStop()) {
    return Error(msg::CANCEL_UNSUPPORTED, "charger has no stop support");
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stopping_ = true;
  }
  SteadyTime sent_at = SteadyTime::max();
  const auto error = backend_->StopCharging(&sent_at);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!IsOk(error) && !error.may_have_executed()) {
      stopping_ = false;
    } else {
      lease_evidence_after_ = sent_at;
    }
  }
  return error;
}
bool ChargerDevice::SafeToRelease() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return Fresh() && IsOk(feedback_.error) && !lease_ && !stopping_ &&
         feedback_.value.phase() == msg::ChargingState::IDLE;
}
bool ChargerDevice::HasControlLease() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return lease_;
}
void ChargerDevice::ForgetOperation(const std::string& operation_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (start_ && start_->operation_id == operation_id) {
    start_.reset();
  }
  if (stop_ && stop_->operation_id == operation_id) {
    stop_.reset();
  }
}
msg::EquipmentError ChargerDevice::Deactivate() {
  const auto error = backend_->Deactivate();
  std::lock_guard<std::mutex> lock(mutex_);
  connected_ = false;
  return error;
}
}  // namespace apollo::drivers::equipment

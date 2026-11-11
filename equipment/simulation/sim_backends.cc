// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/simulation/sim_backends.h"

namespace apollo::drivers::equipment {
namespace {
msg::EquipmentError Configure(const DeviceConfig& source,
                              SimulationConfig* config) {
  if (!source.bindings().empty() ||
      (source.has_model_config() && !source.model_config().UnpackTo(config))) {
    return Error(msg::INVALID_ARGUMENT,
                 "sim requires SimulationConfig and no hardware bindings");
  }
  return Error(msg::OK);
}
}  // namespace
msg::EquipmentError SimBinaryBackend::Initialize(const DeviceConfig& config) {
  const auto error = Configure(config, &config_);
  positive_ = config_.initial_positive();
  return error;
}
msg::EquipmentError SimBinaryBackend::Activate() {
  connected_ = true;
  return Error(msg::OK);
}
msg::EquipmentError SimBinaryBackend::SetTarget(bool positive,
                                                SteadyTime* sent_at) {
  if (!connected_) {
    return Error(msg::DEVICE_OFFLINE, "sim is disconnected");
  }
  *sent_at = std::chrono::steady_clock::now();
  commanded_ = true;
  pending_ = true;
  target_ = positive;
  due_ = *sent_at + std::chrono::milliseconds(config_.transition_delay_ms());
  return Error(msg::OK);
}
msg::EquipmentError SimBinaryBackend::Stop(SteadyTime* sent_at) {
  *sent_at = std::chrono::steady_clock::now();
  pending_ = false;
  return Error(msg::OK);
}
msg::EquipmentError SimBinaryBackend::Poll(SignalFeedback* feedback,
                                           bool* updated) {
  *updated = false;
  if (!connected_) {
    return Error(msg::DEVICE_OFFLINE, "sim is disconnected");
  }
  if (commanded_ && config_.freeze_after_command()) {
    return Error(msg::OK);
  }
  const auto now = std::chrono::steady_clock::now();
  if (pending_ && now >= due_) {
    positive_ = target_;
    pending_ = false;
  }
  feedback->positive = positive_;
  feedback->negative = !positive_;
  feedback->moving = pending_;
  feedback->received_at = now;
  feedback->observed_at_unix_ns = UnixTimeNs();
  feedback->error = commanded_ && config_.fault_on_command()
                        ? Error(msg::DEVICE_FAULT, "injected simulation fault")
                        : Error(msg::OK);
  *updated = true;
  return Error(msg::OK);
}
msg::EquipmentError SimBinaryBackend::Deactivate() {
  connected_ = false;
  return Error(msg::OK);
}
msg::EquipmentError SimChargingBackend::Initialize(const DeviceConfig& config) {
  return Configure(config, &config_);
}
msg::EquipmentError SimChargingBackend::Activate() {
  connected_ = true;
  return Error(msg::OK);
}
msg::EquipmentError SimChargingBackend::StartCharging(SteadyTime* sent_at) {
  if (!connected_) {
    return Error(msg::DEVICE_OFFLINE, "sim is disconnected");
  }
  *sent_at = std::chrono::steady_clock::now();
  commanded_ = true;
  phase_ = msg::ChargingState::STARTING;
  due_ = *sent_at + std::chrono::milliseconds(config_.transition_delay_ms());
  return Error(msg::OK);
}
msg::EquipmentError SimChargingBackend::StopCharging(SteadyTime* sent_at) {
  if (!connected_) {
    return Error(msg::DEVICE_OFFLINE, "sim is disconnected");
  }
  *sent_at = std::chrono::steady_clock::now();
  phase_ = msg::ChargingState::STOPPING;
  due_ = *sent_at + std::chrono::milliseconds(config_.transition_delay_ms());
  return Error(msg::OK);
}
msg::EquipmentError SimChargingBackend::Poll(ChargingFeedback* feedback,
                                             bool* updated) {
  *updated = false;
  if (!connected_) {
    return Error(msg::DEVICE_OFFLINE, "sim is disconnected");
  }
  if (commanded_ && config_.freeze_after_command()) {
    return Error(msg::OK);
  }
  const auto now = std::chrono::steady_clock::now();
  if (now >= due_) {
    if (phase_ == msg::ChargingState::STARTING) {
      phase_ = msg::ChargingState::CHARGING;
    } else if (phase_ == msg::ChargingState::STOPPING) {
      phase_ = msg::ChargingState::IDLE;
    }
  }
  const bool fault = commanded_ && config_.fault_on_command();
  feedback->value.set_phase(fault ? msg::ChargingState::FAULT : phase_);
  feedback->received_at = now;
  feedback->observed_at_unix_ns = UnixTimeNs();
  feedback->error = fault
                        ? Error(msg::DEVICE_FAULT, "injected simulation fault")
                        : Error(msg::OK);
  *updated = true;
  return Error(msg::OK);
}
msg::EquipmentError SimChargingBackend::Deactivate() {
  connected_ = false;
  return Error(msg::OK);
}
}  // namespace apollo::drivers::equipment

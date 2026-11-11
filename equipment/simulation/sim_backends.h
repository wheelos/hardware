// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include "modules/drivers/equipment/devices/binary_backend.h"
#include "modules/drivers/equipment/devices/charger/charging_backend.h"

namespace apollo::drivers::equipment {
// Explicit simulation models: no HAL, wire protocol or physical feedback.
class SimBinaryBackend final : public BinaryBackend {
 public:
  msg::EquipmentError Initialize(const DeviceConfig& config) override;
  bool SupportsStop() const override { return true; }
  msg::EquipmentError Activate() override;
  msg::EquipmentError SetTarget(bool positive, SteadyTime* sent_at) override;
  msg::EquipmentError Stop(SteadyTime* sent_at) override;
  msg::EquipmentError Poll(SignalFeedback* feedback, bool* updated) override;
  msg::EquipmentError Deactivate() override;

 private:
  SimulationConfig config_;
  bool connected_ = false;
  bool positive_ = false;
  bool target_ = false;
  bool pending_ = false;
  bool commanded_ = false;
  SteadyTime due_;
};
class SimChargingBackend final : public ChargingBackend {
 public:
  msg::EquipmentError Initialize(const DeviceConfig& config) override;
  bool SupportsStop() const override { return true; }
  msg::EquipmentError Activate() override;
  msg::EquipmentError StartCharging(SteadyTime* sent_at) override;
  msg::EquipmentError StopCharging(SteadyTime* sent_at) override;
  msg::EquipmentError Poll(ChargingFeedback* feedback, bool* updated) override;
  msg::EquipmentError Deactivate() override;

 private:
  SimulationConfig config_;
  bool connected_ = false;
  bool commanded_ = false;
  msg::ChargingState::Phase phase_ = msg::ChargingState::IDLE;
  SteadyTime due_;
};
}  // namespace apollo::drivers::equipment

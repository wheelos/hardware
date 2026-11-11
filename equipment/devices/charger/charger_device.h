// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "modules/drivers/equipment/devices/charger/charging_backend.h"

namespace apollo::drivers::equipment {
class ChargerDevice final : public Device {
 public:
  explicit ChargerDevice(std::unique_ptr<ChargingBackend> backend);
  msg::EquipmentError Initialize(const DeviceConfig& config) override;
  msg::DeviceDescriptor Describe() const override;
  msg::EquipmentError Activate() override;
  msg::EquipmentError Validate(
      const msg::DeviceCommand& command) const override;
  msg::EquipmentError Begin(const OperationContext& context,
                            const msg::DeviceCommand& command) override;
  msg::EquipmentError Poll(DeviceUpdate* update) override;
  msg::EquipmentError Cancel(const std::string& operation_id) override;
  msg::EquipmentError RequestStop() override;
  bool SafeToRelease() const override;
  bool HasControlLease() const override;
  void ForgetOperation(const std::string& operation_id) override;
  msg::EquipmentError Deactivate() override;

 private:
  bool Fresh() const;
  StateSample Sample() const;
  OperationEvent Completion(const OperationContext& operation,
                            msg::OperationState state) const;
  std::unique_ptr<ChargingBackend> backend_;
  DeviceConfig config_;
  msg::DeviceDescriptor descriptor_;
  mutable std::mutex mutex_;
  ChargingFeedback feedback_;
  bool connected_ = false;
  bool have_feedback_ = false;
  bool lease_ = false;
  bool stopping_ = false;
  std::string session_;
  std::optional<OperationContext> start_;
  std::optional<OperationContext> stop_;
  uint64_t sequence_ = 0;
  SteadyTime lease_evidence_after_;
};
}  // namespace apollo::drivers::equipment

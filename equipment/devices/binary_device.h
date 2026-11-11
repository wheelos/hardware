// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "modules/drivers/equipment/devices/binary_backend.h"

namespace apollo::drivers::equipment {
class BinaryDevice : public Device {
 public:
  enum class Kind { kSwitch, kOpenClose, kLift };
  BinaryDevice(Kind kind, std::unique_ptr<BinaryBackend> backend);
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
  msg::EquipmentError Decode(const msg::DeviceCommand& command,
                             bool* positive) const;
  bool Fresh() const;
  StateSample Sample() const;
  OperationEvent Completion(msg::OperationState state, bool already) const;
  Kind kind_;
  std::unique_ptr<BinaryBackend> backend_;
  DeviceConfig config_;
  msg::DeviceDescriptor descriptor_;
  mutable std::mutex mutex_;
  SignalFeedback feedback_;
  bool connected_ = false;
  bool have_feedback_ = false;
  bool awaiting_ = false;
  bool stopping_ = false;
  bool positive_ = false;
  bool already_ = false;
  uint64_t sequence_ = 0;
  OperationContext operation_;
};
}  // namespace apollo::drivers::equipment

// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include "modules/drivers/equipment/device.h"
#include "wheelos_msgs/equipment_msgs/capabilities/charging.pb.h"

namespace apollo::drivers::equipment {
struct ChargingFeedback {
  msg::ChargingState value;
  msg::EquipmentError error;
  SteadyTime received_at;
  uint64_t observed_at_unix_ns = 0;
};
class ChargingBackend {
 public:
  virtual ~ChargingBackend() = default;
  virtual msg::EquipmentError Initialize(const DeviceConfig& config) = 0;
  virtual bool SupportsStop() const = 0;
  virtual msg::EquipmentError Activate() = 0;
  // Return the dispatch boundary ordered with feedback receipt, not the call
  // entry/return time. Preserve it if the command may have executed.
  virtual msg::EquipmentError StartCharging(SteadyTime* sent_at) = 0;
  virtual msg::EquipmentError StopCharging(SteadyTime* sent_at) = 0;
  virtual msg::EquipmentError Poll(ChargingFeedback* feedback,
                                   bool* updated) = 0;
  virtual msg::EquipmentError Deactivate() = 0;
};
}  // namespace apollo::drivers::equipment

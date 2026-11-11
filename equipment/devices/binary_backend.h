// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include "modules/drivers/equipment/device.h"

namespace apollo::drivers::equipment {
struct SignalFeedback {
  bool positive = false;
  bool negative = false;
  bool moving = false;
  msg::EquipmentError error;
  SteadyTime received_at;
  uint64_t observed_at_unix_ns = 0;
};

// Wire adapters implement signal semantics, not public device goals.
class BinaryBackend {
 public:
  virtual ~BinaryBackend() = default;
  virtual msg::EquipmentError Initialize(const DeviceConfig& config) = 0;
  virtual bool SupportsStop() const = 0;
  virtual bool SupportsCompletion() const { return true; }
  virtual msg::EquipmentError Activate() = 0;
  // Set sent_at at dispatch, ordered with feedback receipt, even on uncertain
  // failure. Never sample it before waiting for transport ownership or on
  // return.
  virtual msg::EquipmentError SetTarget(bool positive, SteadyTime* sent_at) = 0;
  virtual msg::EquipmentError Poll(SignalFeedback* feedback, bool* updated) = 0;
  virtual msg::EquipmentError Stop(SteadyTime* sent_at) = 0;
  virtual msg::EquipmentError Deactivate() = 0;
};
}  // namespace apollo::drivers::equipment

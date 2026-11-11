// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <memory>
#include <utility>

#include "modules/drivers/equipment/devices/binary_device.h"

namespace apollo::drivers::equipment {
class LiftDevice final : public BinaryDevice {
 public:
  explicit LiftDevice(std::unique_ptr<BinaryBackend> backend)
      : BinaryDevice(Kind::kLift, std::move(backend)) {}
};
}  // namespace apollo::drivers::equipment

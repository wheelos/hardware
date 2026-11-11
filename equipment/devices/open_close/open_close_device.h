// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <memory>
#include <utility>

#include "modules/drivers/equipment/devices/binary_device.h"

namespace apollo::drivers::equipment {
class OpenCloseDevice final : public BinaryDevice {
 public:
  explicit OpenCloseDevice(std::unique_ptr<BinaryBackend> backend)
      : BinaryDevice(Kind::kOpenClose, std::move(backend)) {}
};
}  // namespace apollo::drivers::equipment

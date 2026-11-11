// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "modules/drivers/equipment/devices/binary_backend.h"
#include "modules/drivers/equipment/transport/channel.h"

namespace apollo::drivers::equipment {
class CanSignalBackend final : public BinaryBackend {
 public:
  explicit CanSignalBackend(ChannelBinding binding);
  msg::EquipmentError Initialize(const DeviceConfig& config) override;
  bool SupportsStop() const override;
  msg::EquipmentError Activate() override;
  msg::EquipmentError SetTarget(bool positive, SteadyTime* sent_at) override;
  msg::EquipmentError Poll(SignalFeedback* feedback, bool* updated) override;
  msg::EquipmentError Stop(SteadyTime* sent_at) override;
  msg::EquipmentError Deactivate() override;

 private:
  msg::EquipmentError Send(const std::string& payload, SteadyTime* sent_at);
  ChannelBinding binding_;
  std::shared_ptr<CanSession> session_;
  CanSignalAddress address_;
  CanSignalConfig mapping_;
};
}  // namespace apollo::drivers::equipment

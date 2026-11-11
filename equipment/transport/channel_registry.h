// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "modules/drivers/equipment/transport/channel.h"

namespace apollo::drivers::equipment {
class ChannelRegistry {
 public:
  using StreamCreator =
      std::function<std::shared_ptr<hal::Stream>(const StreamConfig&)>;
  explicit ChannelRegistry(StreamCreator creator = CreateStream);
  msg::EquipmentError Initialize(const EquipmentModuleConfig& config);
  msg::EquipmentError Resolve(const DeviceConfig& config,
                              ChannelBindings* bindings) const;
  msg::EquipmentError Start();
  msg::EquipmentError Shutdown();
  std::vector<msg::ChannelHealth> Health() const;
  static msg::EquipmentError ValidateStream(const StreamConfig& config);
  static std::shared_ptr<hal::Stream> CreateStream(const StreamConfig& config);

 private:
  StreamCreator creator_;
  std::map<std::string, std::shared_ptr<Channel>> channels_;
};
}  // namespace apollo::drivers::equipment

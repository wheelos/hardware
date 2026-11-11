// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "modules/drivers/equipment/device.h"
#include "modules/drivers/equipment/transport/channel.h"

namespace apollo::drivers::equipment {
class DeviceFactory {
 public:
  using Creator =
      std::function<std::unique_ptr<Device>(const ChannelBindings&)>;
  DeviceFactory();
  bool Register(const std::string& device_type, const std::string& model,
                Creator creator);
  std::unique_ptr<Device> Create(const DeviceConfig& config,
                                 const ChannelBindings& bindings) const;

 private:
  std::map<std::pair<std::string, std::string>, Creator> creators_;
};
}  // namespace apollo::drivers::equipment

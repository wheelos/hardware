// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/device_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "modules/drivers/equipment/devices/charger/charger_device.h"
#include "modules/drivers/equipment/devices/lift/lift_device.h"
#include "modules/drivers/equipment/devices/open_close/open_close_device.h"
#include "modules/drivers/equipment/devices/switch/switch_device.h"
#include "modules/drivers/equipment/protocols/can_signal/can_signal_backend.h"
#include "modules/drivers/equipment/simulation/sim_backends.h"

namespace apollo::drivers::equipment {
namespace {
template <typename DeviceType>
std::unique_ptr<Device> CreateSimBinary(const ChannelBindings& bindings) {
  return bindings.empty() ? std::make_unique<DeviceType>(
                                std::make_unique<SimBinaryBackend>())
                          : nullptr;
}
template <typename DeviceType>
std::unique_ptr<Device> CreateCanBinary(const ChannelBindings& bindings) {
  if (bindings.size() != 1 || !bindings.count("main")) {
    return nullptr;
  }
  return std::make_unique<DeviceType>(
      std::make_unique<CanSignalBackend>(bindings.at("main")));
}
}  // namespace
DeviceFactory::DeviceFactory() {
  Register("switch", "sim", CreateSimBinary<SwitchDevice>);
  Register("open_close", "sim", CreateSimBinary<OpenCloseDevice>);
  Register("lift", "sim", CreateSimBinary<LiftDevice>);
  Register("charger", "sim",
           [](const ChannelBindings& bindings) -> std::unique_ptr<Device> {
             return bindings.empty()
                        ? std::make_unique<ChargerDevice>(
                              std::make_unique<SimChargingBackend>())
                        : nullptr;
           });
  Register("switch", "can_signal", CreateCanBinary<SwitchDevice>);
  Register("open_close", "can_signal", CreateCanBinary<OpenCloseDevice>);
  Register("lift", "can_signal", CreateCanBinary<LiftDevice>);
}
bool DeviceFactory::Register(const std::string& device_type,
                             const std::string& model, Creator creator) {
  return !device_type.empty() && !model.empty() && creator &&
         creators_
             .emplace(std::make_pair(device_type, model), std::move(creator))
             .second;
}
std::unique_ptr<Device> DeviceFactory::Create(
    const DeviceConfig& config, const ChannelBindings& bindings) const {
  const auto found = creators_.find({config.device_type(), config.model()});
  return found == creators_.end() ? nullptr : found->second(bindings);
}
}  // namespace apollo::drivers::equipment

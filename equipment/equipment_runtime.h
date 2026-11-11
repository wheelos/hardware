// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <memory>

#include "modules/drivers/equipment/device_factory.h"
#include "modules/drivers/equipment/transport/channel_registry.h"

namespace apollo::drivers::equipment {
class EquipmentRuntime {
 public:
  explicit EquipmentRuntime(
      DeviceFactory factory = DeviceFactory(),
      ChannelRegistry::StreamCreator creator = ChannelRegistry::CreateStream);
  ~EquipmentRuntime();
  EquipmentRuntime(const EquipmentRuntime&) = delete;
  EquipmentRuntime& operator=(const EquipmentRuntime&) = delete;
  msg::EquipmentError Initialize(const EquipmentModuleConfig& config);
  msg::EquipmentError Start();
  msg::EquipmentError Shutdown();
  msg::ListDevicesResponse ListDevices(
      const msg::ListDevicesRequest& request) const;
  msg::GetStateResponse GetState(const msg::GetStateRequest& request) const;
  msg::ExecuteResponse Execute(const msg::ExecuteRequest& request);
  msg::GetOperationResponse GetOperation(
      const msg::GetOperationRequest& request) const;
  msg::CancelOperationResponse CancelOperation(
      const msg::CancelOperationRequest& request);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace apollo::drivers::equipment

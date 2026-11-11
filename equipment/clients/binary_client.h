// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include "modules/drivers/equipment/equipment_client.h"
#include "wheelos_msgs/equipment_msgs/capabilities/lift.pb.h"
#include "wheelos_msgs/equipment_msgs/capabilities/open_close.pb.h"
#include "wheelos_msgs/equipment_msgs/capabilities/switch.pb.h"

namespace apollo::drivers::equipment {
class BinaryClient {
 public:
  explicit BinaryClient(EquipmentClient& client) : client_(client) {}
  msg::ExecuteResponse SetEnabled(const DeviceTarget& target,
                                  const ExecuteContext& context, bool enabled) {
    msg::SwitchGoal goal;
    goal.set_enabled(enabled);
    return client_.Submit(target, context, "set", goal);
  }
  msg::ExecuteResponse Open(const DeviceTarget& target,
                            const ExecuteContext& context) {
    return SetOpen(target, context, msg::OpenCloseGoal::OPEN);
  }
  msg::ExecuteResponse Close(const DeviceTarget& target,
                             const ExecuteContext& context) {
    return SetOpen(target, context, msg::OpenCloseGoal::CLOSED);
  }
  msg::ExecuteResponse Raise(const DeviceTarget& target,
                             const ExecuteContext& context) {
    return SetLift(target, context, msg::LiftGoal::RAISED);
  }
  msg::ExecuteResponse Lower(const DeviceTarget& target,
                             const ExecuteContext& context) {
    return SetLift(target, context, msg::LiftGoal::LOWERED);
  }

 private:
  msg::ExecuteResponse SetOpen(const DeviceTarget& target,
                               const ExecuteContext& context,
                               msg::OpenCloseGoal::Target value) {
    msg::OpenCloseGoal goal;
    goal.set_target(value);
    return client_.Submit(target, context, "set", goal);
  }
  msg::ExecuteResponse SetLift(const DeviceTarget& target,
                               const ExecuteContext& context,
                               msg::LiftGoal::Target value) {
    msg::LiftGoal goal;
    goal.set_target(value);
    return client_.Submit(target, context, "set", goal);
  }
  EquipmentClient& client_;
};
}  // namespace apollo::drivers::equipment

// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <string>

#include "modules/drivers/equipment/equipment_client.h"
#include "wheelos_msgs/equipment_msgs/capabilities/charging.pb.h"

namespace apollo::drivers::equipment {
class ChargingClient {
 public:
  explicit ChargingClient(EquipmentClient& client) : client_(client) {}
  msg::ExecuteResponse StartCharging(const DeviceTarget& target,
                                     const ExecuteContext& context) {
    return client_.Submit(target, context, "start", msg::StartChargingGoal());
  }
  msg::ExecuteResponse StopCharging(const DeviceTarget& target,
                                    const ExecuteContext& context,
                                    const std::string& session_id) {
    msg::StopChargingGoal goal;
    goal.set_session_id(session_id);
    return client_.Submit(target, context, "stop", goal);
  }

 private:
  EquipmentClient& client_;
};
}  // namespace apollo::drivers::equipment

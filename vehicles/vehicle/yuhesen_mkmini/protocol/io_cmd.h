// Copyright 2025 daohu527@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//  Created Date: 2025-01-16
//  Author: daohu527

#pragma once

#include "modules/drivers/canbus/can_comm/protocol_data.h"

#include "modules/canbus/proto/chassis_detail.pb.h"

namespace apollo {
namespace canbus {
namespace yuhesen {

class IOCmd : public ::apollo::drivers::canbus::ProtocolData<
                  ::apollo::canbus::ChassisDetail> {
 public:
  static const int32_t ID;

  IOCmd();

  uint32_t GetPeriod() const override;

  void UpdateData(uint8_t* data) override;

  void Reset() override;

  IO_cmd* set_io_control_enable(bool io_control_enable);

  IO_cmd* set_turn_signal_and_hazard_light_switch(
      IO_cmd::TurnSignalAndHazardLightSwitch
          turn_signal_and_hazard_light_switch);

  IO_cmd* set_marker_light_switch(bool marker_light_switch);

  IO_cmd* set_charging_force_power_on_flag(bool charging_force_power_on_flag);

  IO_cmd* set_heartbeat_count(int heartbeat_count);

  IO_cmd* set_check_bcc(int check_bcc);

 private:
  bool io_control_enable_;
  IO_cmd::TurnSignalAndHazardLightSwitch turn_signal_and_hazard_light_switch_;
  bool marker_light_switch_;
  bool charging_force_power_on_flag_;
  int heartbeat_count_;
  int check_bcc_;
};

}  // namespace yuhesen
}  // namespace canbus
}  // namespace apollo

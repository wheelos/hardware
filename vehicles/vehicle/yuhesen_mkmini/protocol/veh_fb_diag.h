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

#include "modules/canbus/proto/chassis_detail.pb.h"
#include "modules/drivers/canbus/can_comm/protocol_data.h"

namespace apollo {
namespace canbus {
namespace yuhesen {

class VehFbDiag : public ::apollo::drivers::canbus::ProtocolData<
                         ::apollo::canbus::ChassisDetail> {
 public:
  static const int32_t ID;
  VehFbDiag();
  void Parse(const std::uint8_t* bytes, int32_t length,
             ChassisDetail* chassis) const override;

 private:
  VehicleFaultLevel vehicle_fault_level(const std::uint8_t* bytes, const int32_t length) const;

  bool auto_control_can_communication_error(const std::uint8_t* bytes, const int32_t length) const;

  bool auto_io_control_can_communication_error(const std::uint8_t* bytes, const int32_t length) const;

  bool eps_offline_fault(const std::uint8_t* bytes, const int32_t length) const;

  bool eps_general_fault(const std::uint8_t* bytes, const int32_t length) const;

  bool eps_mosfet_over_temperature(const std::uint8_t* bytes, const int32_t length) const;

  bool eps_alarm_fault(const std::uint8_t* bytes, const int32_t length) const;

  bool eps_over_current_fault(const std::uint8_t* bytes, const int32_t length) const;

  MotorDriverFault left_wheel_motor_driver_fault(const std::uint8_t* bytes, const int32_t length) const;

  MotorDriverFault right_wheel_motor_driver_fault(const std::uint8_t* bytes, const int32_t length) const;

  bool bms_can_communication_offline(const std::uint8_t* bytes, const int32_t length) const;

  bool emergency_stop_fault(const std::uint8_t* bytes, const int32_t length) const;

  bool remote_controller_off_warning(const std::uint8_t* bytes, const int32_t length) const;

  bool remote_receiver_offline_fault(const std::uint8_t* bytes, const int32_t length) const;

  uint8_t heartbeat_count(const std::uint8_t* bytes, const int32_t length) const;
};

}  // namespace yuhesen
}  // namespace canbus
}  // namespace apollo

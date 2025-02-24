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
namespace ros {

class TwistCmd : public ::apollo::drivers::canbus::ProtocolData<
                     ::apollo::canbus::ChassisDetail> {
 public:
  static const int32_t ID;

  TwistCmd();

  uint32_t GetPeriod() const override;

  void UpdateData(uint8_t* data) override;

  void Reset() override;

 private:
  void set_x_target_speed(uint8_t* data, uint32_t x_target_speed);

  void set_y_target_speed(uint8_t* data, uint32_t y_target_speed);

  void set_angular_velocity_z(uint8_t* data, int angular_velocity_z);

  void set_checksum(uint8_t* data, int checksum);

 private:
  uint32_t x_target_speed_;
  uint32_t y_target_speed_;
  int angular_velocity_z_;
};

}  // namespace ros
}  // namespace canbus
}  // namespace apollo

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

class IOFb : public ::apollo::drivers::canbus::ProtocolData<
                 ::apollo::canbus::ChassisDetail> {
 public:
  static const int32_t ID;
  IOFb();
  void Parse(const std::uint8_t* bytes, int32_t length,
             ChassisDetail* chassis) const override;

 private:
  bool io_control_enable(const std::uint8_t* bytes, const int32_t length) const;

  TurnSignalAndHazardLightSwitch turn_signal_and_hazard_light_switch(
      const std::uint8_t* bytes, const int32_t length) const;

  bool marker_light_switch(const std::uint8_t* bytes,
                           const int32_t length) const;

  bool charging_force_power_on_flag(const std::uint8_t* bytes,
                                    const int32_t length) const;

  uint8_t heartbeat_count(const std::uint8_t* bytes,
                          const int32_t length) const;
};

}  // namespace yuhesen
}  // namespace canbus
}  // namespace apollo

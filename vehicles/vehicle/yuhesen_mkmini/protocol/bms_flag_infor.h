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

class BmsFlagInfoR : public ::apollo::drivers::canbus::ProtocolData<
                         ::apollo::canbus::ChassisDetail> {
 public:
  static const int32_t ID;
  BmsFlagInfoR();
  void Parse(const std::uint8_t* bytes, int32_t length,
             ChassisDetail* chassis) const override;

 private:
  uint8_t remaining_capacity_percent(const std::uint8_t* bytes,
                                     const int32_t length) const;

  bool cell_over_voltage_protection(const std::uint8_t* bytes,
                                    const int32_t length) const;

  bool cell_under_voltage_protection(const std::uint8_t* bytes,
                                     const int32_t length) const;

  bool pack_over_voltage_protection(const std::uint8_t* bytes,
                                    const int32_t length) const;

  bool pack_under_voltage_protection(const std::uint8_t* bytes,
                                     const int32_t length) const;

  bool charging_over_temperature_protection(const std::uint8_t* bytes,
                                            const int32_t length) const;

  bool charging_under_temperature_protection(const std::uint8_t* bytes,
                                             const int32_t length) const;

  bool discharging_over_temperature_protection(const std::uint8_t* bytes,
                                               const int32_t length) const;

  bool discharging_under_temperature_protection(const std::uint8_t* bytes,
                                                const int32_t length) const;

  bool charging_over_current_protection(const std::uint8_t* bytes,
                                        const int32_t length) const;

  bool discharging_over_current_protection(const std::uint8_t* bytes,
                                           const int32_t length) const;

  bool short_circuit_protection(const std::uint8_t* bytes,
                                const int32_t length) const;

  bool frontend_ic_error(const std::uint8_t* bytes, const int32_t length) const;

  bool software_mos_lock(const std::uint8_t* bytes, const int32_t length) const;

  ChargingStatus charging_status(const std::uint8_t* bytes,
                                 const int32_t length) const;

  bool soc_low_alarm(const std::uint8_t* bytes, const int32_t length) const;

  bool battery_low_power_alarm(const std::uint8_t* bytes,
                               const int32_t length) const;

  uint8_t max_cell_temperature(const std::uint8_t* bytes,
                               const int32_t length) const;

  uint8_t min_cell_temperature(const std::uint8_t* bytes,
                               const int32_t length) const;

  uint8_t heartbeat_count(const std::uint8_t* bytes,
                          const int32_t length) const;
};

}  // namespace yuhesen
}  // namespace canbus
}  // namespace apollo

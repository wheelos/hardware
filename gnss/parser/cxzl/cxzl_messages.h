// Copyright 2024 daohu527@gmail.com
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

//  Created Date: 2024-12-30
//  Author: daohu527

#pragma once

#include <cstdint>

namespace apollo {
namespace drivers {
namespace gnss {
namespace cxzl {

// Every binary message has 32-bit CRC performed on all data including the
// header.
constexpr uint16_t CRC_LENGTH = 2;

enum class SoluStatus : uint8_t {
  INIT_STATUS = 0,
  SATELLITE_NAVIGATION = 1,
  INTEGRATED_NAVIGATION = 2,
  INERTIAL_NAVIGATION = 3,
};

enum class SatelliteStatus : uint8_t {
  POS_NONE = 0,
  POS_LOCATEDIRECT_SINGLE = 1,
  POS_LOCATEDIRECT_DIFF = 2,
  POS_LOCATEDIRECT_COMB = 3,
  POS_LOCATEDIRECT_FIXED = 4,
  POS_LOCATEDIRECT_FLOAT = 5,
  POS_LOCATE_SINGLE = 6,
  POS_LOCATE_DIFF = 7,
  POS_LOCATE_FIXED = 8,
  POS_LOCATE_FLOAT = 9,
};

struct CXINSPVA {
  uint32_t gps_week;
  double gps_time;

  double gyro_x;
  double gyro_y;
  double gyro_z;
  double acc_x;
  double acc_y;
  double acc_z;

  SoluStatus solution_status;
  uint8_t nsv1;
  SatelliteStatus satellite_status;
  uint8_t nsv2;
  uint32_t age;
  int nsvd1;
  int nsvd2;

  double altitude;

  double pe_std = 0.0;
  double pn_std = 0.0;
  double pu_std = 0.0;

  double ve;
  double vn;
  double vu;
  double v;
  double vn_std = 0.0;
  double ve_std = 0.0;
  double vu_std = 0.0;
  double v_std = 0.0;

  double veh_acc_x;
  double veh_acc_y;
  double veh_acc_z;

  double roll;
  double pitch;
  double heading;
  double roll_std = 0.0;
  double pitch_std = 0.0;
  double heading_std = 0.0;
  double veh_gyro_x;
  double veh_gyro_y;
  double veh_gyro_z;

  double longitude;
  double latitude;

  unsigned char check;
  double gps_timestamp_sec = 0;
};

}  // namespace cxzl
}  // namespace gnss
}  // namespace drivers
}  // namespace apollo

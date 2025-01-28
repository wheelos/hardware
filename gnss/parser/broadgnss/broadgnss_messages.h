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

#include <string>

namespace apollo {
namespace drivers {
namespace gnss {
namespace broadgnss {

// Every binary message has 32-bit CRC performed on all data including the
// header.
constexpr uint16_t CRC_LENGTH = 2;

enum class Status : uint8_t {
  // System status (lower nibble)
  INIT = 0x00,      // 0: Initialization
  GUIDANCE = 0x01,  // 1: Guidance mode
  COMBINED = 0x02,  // 2: Combined navigation mode
  INERTIAL = 0x03,  // 3: Pure inertial mode

  // Satellite status (upper nibble)
  // 0: No positioning, no orientation
  NO_POS_NO_ORIENT = 0x10,
  // 1: Single point positioning and orientation
  SINGLE_POS_ORIENT = 0x11,
  // 2: Pseudorange differential positioning and orientation
  PSEUDORANGE_DIFF_ORIENT = 0x12,
  // 3: Combined prediction
  COMBINED_PREDICTION = 0x13,
  // 4: RTK stable solution positioning and orientation
  RTK_STABLE_ORIENT = 0x14,
  // 5: RTK float solution positioning and orientation
  RTK_FLOAT_ORIENT = 0x15,
  // 6: Single point positioning, no orientation
  SINGLE_POS_NO_ORIENT = 0x16,
  // 7: Pseudorange differential positioning, no orientation
  PSEUDORANGE_DIFF_NO_ORIENT = 0x17,
  // 8: RTK stable solution positioning, no orientation
  RTK_STABLE_NO_ORIENT = 0x18,
  // 9: RTK float solution positioning, no orientation
  RTK_FLOAT_NO_ORIENT = 0x19
};

struct GPYJ {
  // GPCHC protocol header, default value "$GPCHC"
  char header[6];
  // GPS week number since 1980-1-6
  uint32_t gps_week;
  // Seconds since the start of the current GPS week
  double gps_time;
  // Heading angle (0 to 359.99)
  double heading;
  // Pitch angle (-90 to 90)
  double pitch;
  // Roll angle (-180 to 180)
  double roll;
  // Gyroscope X-axis
  double gyro_x;
  // Gyroscope Y-axis
  double gyro_y;
  // Gyroscope Z-axis
  double gyro_z;
  // Accelerometer X-axis
  double acc_x;
  // Accelerometer Y-axis
  double acc_y;
  // Accelerometer Z-axis
  double acc_z;
  // Latitude (-90° to 90°)
  double latitude;
  // Longitude (-180° to 180°)
  double longitude;
  // Altitude in meters
  double altitude;
  // Eastward velocity in m/s
  double ve;
  // Northward velocity in m/s
  double vn;
  // Upward velocity in m/s
  double vu;
  // Vehicle speed in m/s
  double v;
  // Number of satellites for main antenna
  uint8_t nsv1;
  // Number of satellites for secondary antenna
  uint8_t nsv2;
  Status status;
  // Differential delay
  uint32_t age;
  // TODO(zero): Why warming but not warning?
  // Warning flags:
  // bit0: 1: No GPS message, 0: Normal
  // bit1: 1: No vehicle message, 0: Normal
  // bit3: 1: Gyroscope error, 0: Normal
  // bit4: 1: Accelerometer error, 0: Normal
  uint8_t warming;
};

}  // namespace broadgnss
}  // namespace gnss
}  // namespace drivers
}  // namespace apollo

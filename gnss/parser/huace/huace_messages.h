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
namespace huace {

constexpr uint16_t CRC_LENGTH = 2;

enum class Status : uint8_t {
  // System status (lower nibble)
  INIT = 0x00,      // 0: Initialization
  GUIDANCE = 0x01,  // 1: Guidance mode
  COMBINED = 0x02,  // 2: Combined navigation mode
  INERTIAL = 0x03,  // 3: Pure inertial mode

  // Satellite status (upper nibble)
  NO_POS_NO_ORIENT = 0x10,   // 0: No positioning, no orientation
  SINGLE_POS_ORIENT = 0x11,  // 1: Single point positioning and orientation
  PSEUDORANGE_DIFF_ORIENT =
      0x12,  // 2: Pseudorange differential positioning and orientation
  COMBINED_PREDICTION = 0x13,  // 3: Combined prediction
  RTK_STABLE_ORIENT =
      0x14,  // 4: RTK stable solution positioning and orientation
  RTK_FLOAT_ORIENT = 0x15,  // 5: RTK float solution positioning and orientation
  SINGLE_POS_NO_ORIENT = 0x16,  // 6: Single point positioning, no orientation
  PSEUDORANGE_DIFF_NO_ORIENT =
      0x17,  // 7: Pseudorange differential positioning, no orientation
  RTK_STABLE_NO_ORIENT =
      0x18,  // 8: RTK stable solution positioning, no orientation
  RTK_FLOAT_NO_ORIENT =
      0x19  // 9: RTK float solution positioning, no orientation
};

struct GPCHC {
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
  // Warning flags:
  // bit0: 1: No GPS message, 0: Normal
  // bit1: 1: No vehicle message, 0: Normal
  // bit3: 1: Gyroscope error, 0: Normal
  // bit4: 1: Accelerometer error, 0: Normal
  uint8_t warning;
};

struct GPCHCX {
  // GPCHCX protocol header, default value "$GPCHCX"
  char header[7];
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
  // Status
  Status status;
  // Differential delay
  uint32_t age;
  // Warning flags:
  // bit0: 1: No GPS message, 0: Normal
  // bit1: 1: No vehicle message, 0: Normal
  // bit3: 1: Gyroscope error, 0: Normal
  // bit4: 1: Accelerometer error, 0: Normal
  uint8_t warning;

  // Latitude standard deviation, unit (meters)
  double latitude_std = 0.0;
  // Longitude standard deviation, unit (meters)
  double longitude_std = 0.0;
  // Altitude standard deviation, unit (meters)
  double altitude_std = 0.0;
  // Eastward velocity standard deviation, unit (m/s)
  double ve_std = 0.0;
  // Northward velocity standard deviation, unit (m/s)
  double vn_std = 0.0;
  // Upward velocity standard deviation, unit (m/s)
  double vu_std = 0.0;

  // Roll angle standard deviation, unit (degrees)
  double roll_std = 0.0;
  // Pitch angle standard deviation, unit (degrees)
  double pitch_std = 0.0;
  // Heading angle standard deviation, unit (degrees)
  double heading_std = 0.0;

  // Separator symbol "X" for easy parsing and reference
  char separator = 'X';
  // Speed heading (0 to 359.99) (accurate to two decimal places)
  double speed_heading;
  // Speed heading standard deviation, unit (degrees), accurate to two decimal
  // places
  double speed_heading_std;
  // Antenna position X-axis lever arm relative to the device, in vehicle
  // coordinate system, unit (meters), accurate to two decimal places
  float antenna_x;
  // Antenna position Y-axis lever arm relative to the device, in vehicle
  // coordinate system, unit (meters), accurate to two decimal places
  float antenna_y;
  // Antenna position Z-axis lever arm relative to the device, in vehicle
  // coordinate system, unit (meters), accurate to two decimal places
  float antenna_z;
  // Rotation Euler angle from device coordinate system to vehicle coordinate
  // system, X-axis angle, unit (meters), accurate to two decimal places
  float rotation_angle_x;
  // Rotation Euler angle from device coordinate system to vehicle coordinate
  // system, Y-axis angle, unit (meters), accurate to two decimal places
  float rotation_angle_y;
  // Rotation Euler angle from device coordinate system to vehicle coordinate
  // system, Z-axis angle, unit (meters), accurate to two decimal places
  float rotation_angle_z;
  // Rotation angle from vehicle heading to GNSS heading direction, along
  // vehicle coordinate system Z-axis, unit (degrees), accurate to two decimal
  // places
  float direction_angle;
  // Number of satellites used by main antenna 1
  uint8_t main_antenna_satellite_count;
  // Number of satellites used by secondary antenna 2
  uint8_t secondary_antenna_satellite_count;
  // Device serial number
  char device_sn[7];
};

}  // namespace huace
}  // namespace gnss
}  // namespace drivers
}  // namespace apollo

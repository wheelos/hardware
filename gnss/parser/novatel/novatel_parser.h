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

#include <vector>

#include "gnss/parser/novatel_messages.h"
#include "gnss/parser/parser.h"

#include "gnss/proto/gnss.pb.h"
#include "gnss/proto/gnss_best_pose.pb.h"
#include "gnss/proto/gnss_raw_observation.pb.h"
#include "gnss/proto/heading.pb.h"
#include "gnss/proto/imu.pb.h"
#include "gnss/proto/ins.pb.h"

namespace apollo {
namespace drivers {
namespace gnss {

class NovatelParser : public Parser {
 public:
  NovatelParser();
  explicit NovatelParser(const config::Config& config);

  virtual void GetMessages(std::vector<Parser::Message>* messages);

 private:
  MessageType GetMessage(MessagePtr* message_ptr);

  bool CheckCRC();

  Parser::MessageType PrepareMessage(MessagePtr* message_ptr);

  // The handle_xxx functions return whether a message is ready.
  bool HandleBestPos(const novatel::BestPos* pos, uint16_t gps_week,
                     uint32_t gps_millisecs);

  bool HandleGnssBestpos(const novatel::BestPos* pos, uint16_t gps_week,
                         uint32_t gps_millisecs);

  bool HandleBestVel(const novatel::BestVel* vel, uint16_t gps_week,
                     uint32_t gps_millisecs);

  bool HandleCorrImuData(const novatel::CorrImuData* imu);

  bool HandleInsCov(const novatel::InsCov* cov);

  bool HandleInsPva(const novatel::InsPva* pva);

  bool HandleInsPvax(const novatel::InsPvaX* pvax, uint16_t gps_week,
                     uint32_t gps_millisecs);

  bool HandleRawImuX(const novatel::RawImuX* imu);

  bool HandleRawImu(const novatel::RawImu* imu);

  bool HandleBdsEph(const novatel::BDS_Ephemeris* bds_emph);

  bool HandleGpsEph(const novatel::GPS_Ephemeris* gps_emph);

  bool HandleGloEph(const novatel::GLO_Ephemeris* glo_emph);

  void SetObservationTime();

  bool DecodeGnssObservation(const uint8_t* obs_data,
                             const uint8_t* obs_data_end);

  bool HandleHeading(const novatel::Heading* heading, uint16_t gps_week,
                     uint32_t gps_millisecs);
  double gyro_scale_ = 0.0;

  double accel_scale_ = 0.0;

  float imu_measurement_span_ = 1.0f / 200.0f;
  float imu_measurement_hz_ = 200.0f;

  int imu_frame_mapping_ = 5;

  double imu_measurement_time_previous_ = -1.0;

  std::vector<uint8_t> buffer_;

  size_t header_length_ = 0;

  size_t total_length_ = 0;

  config::ImuType imu_type_ = config::ImuType::ADIS16488;

  // -1 is an unused value.
  novatel::SolutionStatus solution_status_ =
      static_cast<novatel::SolutionStatus>(novatel::SolutionStatus::NONE);
  novatel::SolutionType position_type_ =
      static_cast<novatel::SolutionType>(novatel::SolutionType::NONE);
  novatel::SolutionType velocity_type_ =
      static_cast<novatel::SolutionType>(novatel::SolutionType::NONE);
  novatel::InsStatus ins_status_ =
      static_cast<novatel::InsStatus>(novatel::InsStatus::NONE);

  raw_t raw_;  // used for observation data

  ::apollo::drivers::gnss::Gnss gnss_;
  ::apollo::drivers::gnss::GnssBestPose bestpos_;
  ::apollo::drivers::gnss::Imu imu_;
  ::apollo::drivers::gnss::Ins ins_;
  ::apollo::drivers::gnss::InsStat ins_stat_;
  ::apollo::drivers::gnss::GnssEphemeris gnss_ephemeris_;
  ::apollo::drivers::gnss::EpochObservation gnss_observation_;
  ::apollo::drivers::gnss::Heading heading_;
};

}  // namespace gnss
}  // namespace drivers
}  // namespace apollo

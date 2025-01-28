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

#include "gnss/parser/enbroad/enbroad_parser.h"

namespace apollo {
namespace drivers {
namespace gnss {

void EnbroadParser::GetMessages(std::vector<Parser::Message>* messages) {
  // Why do we need a loop? `data_` may have multiple messages, for example:
  // BestPos+BestVel, so we need a loop to read multiple messages.
  while (cyber::OK()) {
    MessagePtr msg_ptr;
    Parser::MessageType msg_type = GetMessage(&msg_ptr);
    // When return NONE, it means no more messages in the buffer, we need stop.
    if (msg_type == MessageType::NONE) {
      break;
    }
    messages->emplace_back(msg_type, msg_ptr);
  }
}

Parser::MessageType EnbroadParser::GetMessage(MessagePtr* message_ptr) {
  if (data_ == nullptr) {
    return false;
  }

  while (data_ < data_end_) {
    if (buffer_.empty()) {  // Looking for SYNC_HEAD_0
      if (*data_ == enbroad::SYNC_HEAD_0) {
        buffer_.push_back(*data_);
      }
      ++data_;
    } else if (buffer_.size() == 1) {  // Looking for SYNC_HEAD_1
      if (*data_ == enbroad::SYNC_HEAD_1) {
        buffer_.push_back(*data_++);
      } else {
        buffer_.clear();
      }
    } else if (buffer_.size() == 2) {  // Looking for SYNC_HEAD_2
      if (*data_ == enbroad::SYNC_HEAD_2) {
        buffer_.push_back(*data_++);
        header_length_ = sizeof(enbroad::Header);
      } else {
        buffer_.clear();
      }
    } else if (header_length_ > 0) {  // Working on header.
      if (buffer_.size() < header_length_) {
        buffer_.push_back(*data_++);
      } else {
        total_length_ =
            header_length_ +
            reinterpret_cast<enbroad::Header*>(buffer_.data())->message_length +
            1 + 2;
        header_length_ = 0;
      }
    } else if (total_length_ > 0) {
      if (buffer_.size() < total_length_) {  // Working on body.
        buffer_.push_back(*data_++);
        continue;
      }
      MessageType type = PrepareMessage(message_ptr);
      buffer_.clear();
      total_length_ = 0;
      if (type != MessageType::NONE) {
        return type;
      }
    }
  }

  return true;
}

Parser::MessageType EnbroadParser::PrepareMessage(MessagePtr* message_ptr) {
  if (!CheckCRC()) {
    AERROR << "CRC check failed.";
    return MessageType::NONE;
  }

  uint8_t* message = buffer_.data() + sizeof(enbroad::Header);

  auto header = reinterpret_cast<const enbroad::Header*>(buffer_.data());
  enbroad::MessageId message_id = header->message_id;
  uint16_t message_length = header->message_length;

  switch (message_id) {
    case enbroad::BIN_NAV_DATA:
      if (message_length != sizeof(enbroad::NAV_DATA)) {
        AWARN << "Incorrect message_length";
        break;
      }
      if (!HandleNavData(reinterpret_cast<enbroad::NAV_DATA*>(message))) {
        AWARN << "HandleNavData fail";
        return false;
      }
      break;
    case enbroad::BIN_SINS_DATA:
      if (message_length != sizeof(enbroad::NAV_SINS)) {
        AWARN << "Incorrect message_length";
        break;
      }
      if (!HandleSINSData(reinterpret_cast<enbroad::NAV_SINS*>(message))) {
        AWARN << "HandleSINSData fail";
        return false;
      }
      break;
    case enbroad::BIN_IMU_DATA:
      if (message_length != sizeof(enbroad::NAV_IMU)) {
        AWARN << "Incorrect message_length";
        break;
      }
      if (!HandleIMUData(reinterpret_cast<enbroad::NAV_IMU*>(message))) {
        AWARN << "HandleIMUData fail";
        return false;
      }
      break;
    case enbroad::BIN_GNSS_DATA:
      if (message_length != sizeof(enbroad::NAV_GNSS)) {
        AWARN << "Incorrect message_length";
        break;
      }
      if (!HandleGNSSData(reinterpret_cast<enbroad::NAV_GNSS*>(message))) {
        return false;
      }
      break;
    default:
      return false;
  }
  return true;
}

bool EnbroadParser::CheckCRC(const std::string& frame) {
  size_t l = frame.size() - huace::CRC_LENGTH;
  return crc32_block(frame.data(), l) ==
         *reinterpret_cast<uint32_t*>(frame.data() + l);
}

bool EnbroadParser::HandleSINSData(const enbroad::NAV_SINS* p_sins) {
  double seconds =
      p_sins->gps_week * SECONDS_PER_WEEK + p_sins->gpssecond * 1e-3;
  ins_.set_measurement_time(seconds);
  ins_.mutable_header()->set_timestamp_sec(cyber::Time::Now().ToSecond());
  ins_.mutable_euler_angles()->set_x(p_sins->roll * DEG_TO_RAD);
  ins_.mutable_euler_angles()->set_y(p_sins->pitch * DEG_TO_RAD);
  // enbroad set northwest as right direction, Here northeast as right direction
  ins_.mutable_euler_angles()->set_z(
      azimuth_deg_to_yaw_rad(normalizeAngleTo180(-p_sins->heading)));
  ins_.mutable_position()->set_lon(p_sins->longitude);
  ins_.mutable_position()->set_lat(p_sins->latitude);
  ins_.mutable_position()->set_height(p_sins->altitude);
  ins_.mutable_linear_velocity()->set_x(p_sins->ve);
  ins_.mutable_linear_velocity()->set_y(p_sins->vn);
  ins_.mutable_linear_velocity()->set_z(p_sins->vu);
  if (enbroad::E_NAV_STATUS_IN_NAV == p_sins->navStatus) {
    ins_.set_type(Ins::GOOD);
  } else if (enbroad::E_NAV_STATUS_SYSTEM_STANDARD == p_sins->navStatus) {
    ins_.set_type(Ins::CONVERGING);
  } else {
    ins_.set_type(Ins::INVALID);
  }

  bestpos_.set_measurement_time(seconds);
  bestpos_.set_longitude(p_sins->longitude);
  bestpos_.set_latitude(p_sins->latitude);
  bestpos_.set_height_msl(p_sins->altitude);
  // bestpos_.set_undulation(0.0); //undulation = height_wgs84 - height_msl
  // datum id number.WGS84
  bestpos_.set_datum_id(
      static_cast<apollo::drivers::gnss::DatumId>(enbroad::DatumId::WGS84));
  // sins standard deviation
  bestpos_.set_latitude_std_dev(p_sins->xigema_lat);
  bestpos_.set_longitude_std_dev(p_sins->xigema_lon);
  bestpos_.set_height_std_dev(p_sins->xigema_alt);

  ins_stat_.mutable_header()->set_timestamp_sec(cyber::Time::Now().ToSecond());
  // ins pos type define as follows
  // fusion GPS as INS_RTKFIXED,
  // fusion wheel as INS_RTKFLOAT,fusion motion as SINGLE,others as NONE
  if (enbroad::E_FUNSION_GPS == p_sins->fusion) {
    ins_stat_.set_pos_type(SolutionType::INS_RTKFIXED);
    bestpos_.set_sol_type(SolutionType::INS_RTKFIXED);
  } else {
    ins_stat_.set_pos_type(SolutionType::NONE);
    bestpos_.set_sol_type(SolutionType::NONE);
  }

  if (enbroad::E_NAV_STATUS_IN_NAV == p_sins->navStatus) {
    ins_stat_.set_ins_status(SolutionStatus::SOL_COMPUTED);
    bestpos_.set_sol_status(SolutionStatus::SOL_COMPUTED);
  } else if (enbroad::E_NAV_STATUS_SYSTEM_STANDARD == p_sins->navStatus) {
    ins_stat_.set_ins_status(SolutionStatus::COLD_START);
    bestpos_.set_sol_status(SolutionStatus::COLD_START);
  } else {
    ins_stat_.set_ins_status(SolutionStatus::INSUFFICIENT_OBS);
    bestpos_.set_sol_status(SolutionStatus::INSUFFICIENT_OBS);
  }
  return true;
}

bool EnbroadParser::HandleIMUData(const enbroad::NAV_IMU* p_imu) {
  float imu_measurement_span = 1.0f / 100.0f;
  double seconds = p_imu->gps_week * SECONDS_PER_WEEK + p_imu->gpssecond * 1e-3;

  imu_.set_measurement_time(seconds);
  imu_.set_measurement_span(imu_measurement_span);
  imu_.mutable_linear_acceleration()->set_x(p_imu->accX);
  imu_.mutable_linear_acceleration()->set_y(p_imu->accY);
  imu_.mutable_linear_acceleration()->set_z(p_imu->accZ);
  imu_.mutable_angular_velocity()->set_x(p_imu->gyroX * DEG_TO_RAD);
  imu_.mutable_angular_velocity()->set_y(p_imu->gyroY * DEG_TO_RAD);
  imu_.mutable_angular_velocity()->set_z(p_imu->gyroZ * DEG_TO_RAD);
  return true;
}

bool EnbroadParser::HandleGNSSData(const enbroad::NAV_GNSS* p_gnss) {
  double seconds =
      p_gnss->gps_week * SECONDS_PER_WEEK + p_gnss->gpssecond * 1e-3;
  // bestpos_.set_base_station_id("0");  // base station id
  bestpos_.set_solution_age(p_gnss->age);  // solution age (sec)
  bestpos_.set_num_sats_tracked(p_gnss->satsNum);
  bestpos_.set_num_sats_in_solution(p_gnss->satsNum);
  bestpos_.set_num_sats_in_solution(p_gnss->satsNum);
  bestpos_.set_num_sats_multi(p_gnss->satsNum);
  // bestpos_.set_galileo_beidou_used_mask(0);
  // bestpos_.set_gps_glonass_used_mask(0);

  heading_.set_measurement_time(seconds);
  heading_.set_heading(p_gnss->heading);
  heading_.set_baseline_length(p_gnss->baseline);
  heading_.set_reserved(0);
  // heading_.set_heading_std_dev(0.0);
  // heading_.set_pitch_std_dev(0.0);
  // heading_.set_station_id("0");
  heading_.set_satellite_tracked_number(p_gnss->satsNum);
  heading_.set_satellite_soulution_number(p_gnss->satsNum);
  heading_.set_satellite_number_obs(p_gnss->satsNum);
  heading_.set_satellite_number_multi(p_gnss->satsNum);
  // heading_.set_solution_source(0);
  // heading_.set_extended_solution_status(0);
  // heading_.set_galileo_beidou_sig_mask(0);
  // heading_.set_gps_glonass_sig_mask(0);
  if (enbroad::E_GPS_RTK_FIXED == p_gnss->headingStatus) {
    heading_.set_position_type(SolutionType::INS_RTKFIXED);
  } else if (enbroad::E_GPS_RTK_FLOAT == p_gnss->headingStatus) {
    heading_.set_position_type(SolutionType::INS_RTKFLOAT);
  } else if (enbroad::E_GPS_RTK_SPP == p_gnss->headingStatus ||
             enbroad::E_GPS_RTK_DGPS == p_gnss->headingStatus) {
    heading_.set_position_type(SolutionType::SINGLE);
  } else {
    heading_.set_position_type(SolutionType::NONE);
  }
  return true;
}

bool EnbroadParser::HandleNavData(const enbroad::NAV_DATA* p_nav) {
  static uint16_t rtkStatus;
  static uint16_t Nav_Standard_flag;
  static uint16_t Sate_Num;
  static float baseline;
  float imu_measurement_span = 1.0f / 100.0f;
  double seconds =
      p_nav->gps_week * SECONDS_PER_WEEK + p_nav->gps_millisecs * 1e-3;
  ins_.set_measurement_time(seconds);
  ins_.mutable_header()->set_timestamp_sec(cyber::Time::Now().ToSecond());
  ins_.mutable_euler_angles()->set_x(static_cast<double>(
      p_nav->roll * Coder_Angle_Scale / Coder_Sensor_Scale * DEG_TO_RAD));
  ins_.mutable_euler_angles()->set_y(static_cast<double>(
      p_nav->pitch * Coder_Angle_Scale / Coder_Sensor_Scale * DEG_TO_RAD));
  // enbroad set northwest as right direction, Here northeast as right direction
  ins_.mutable_euler_angles()->set_z(azimuth_deg_to_yaw_rad(normalizeAngleTo180(
      -p_nav->head * Coder_Angle_Scale / Coder_Sensor_Scale)));
  ins_.mutable_position()->set_lon(
      static_cast<double>(p_nav->lon / Coder_Pos_Scale));
  ins_.mutable_position()->set_lat(
      static_cast<double>(p_nav->lat / Coder_Pos_Scale));
  ins_.mutable_position()->set_height(static_cast<double>(p_nav->alt / 1000.0));
  ins_.mutable_linear_acceleration()->set_x(static_cast<double>(
      p_nav->accX * Coder_Accel_Scale / Coder_Sensor_Scale));
  ins_.mutable_linear_acceleration()->set_y(static_cast<double>(
      p_nav->accY * Coder_Accel_Scale / Coder_Sensor_Scale));
  ins_.mutable_linear_acceleration()->set_z(static_cast<double>(
      p_nav->accZ * Coder_Accel_Scale / Coder_Sensor_Scale));
  ins_.mutable_angular_velocity()->set_x(
      static_cast<double>(p_nav->gyroX * Coder_Rate_Scale /
                          Coder_Sensor_Scale) *
      DEG_TO_RAD);
  ins_.mutable_angular_velocity()->set_y(
      static_cast<double>(p_nav->gyroY * Coder_Rate_Scale /
                          Coder_Sensor_Scale) *
      DEG_TO_RAD);
  ins_.mutable_angular_velocity()->set_z(
      static_cast<double>(p_nav->gyroZ * Coder_Rate_Scale /
                          Coder_Sensor_Scale) *
      DEG_TO_RAD);
  ins_.mutable_linear_velocity()->set_x(
      static_cast<double>(p_nav->ve * Coder_Vel_Scale / Coder_Sensor_Scale));
  ins_.mutable_linear_velocity()->set_y(
      static_cast<double>(p_nav->vn * Coder_Vel_Scale / Coder_Sensor_Scale));
  ins_.mutable_linear_velocity()->set_z(
      static_cast<double>(p_nav->vu * Coder_Vel_Scale / Coder_Sensor_Scale));
  ins_.set_type(Ins::GOOD);
  switch (p_nav->poll_type) {
    case enbroad::E_POLL_DEV_TEMP:
      break;
    case enbroad::E_POLL_GNSS_STATE:
      rtkStatus = p_nav->poll_frame1;
      break;
    case enbroad::E_POLL_CAN_STATE:
      break;
    case enbroad::E_POLL_INS_STATE:
      Nav_Standard_flag = p_nav->poll_frame1;
      break;
    case enbroad::E_POLL_GNSS2_STATE:
      Sate_Num = p_nav->poll_frame1;
      baseline = p_nav->poll_frame2 / 1000.0;
      break;
    default:
      break;
  }
  imu_.set_measurement_time(seconds);
  imu_.set_measurement_span(imu_measurement_span);
  imu_.mutable_linear_acceleration()->set_x(static_cast<double>(
      p_nav->accX * Coder_Accel_Scale / Coder_Sensor_Scale));
  imu_.mutable_linear_acceleration()->set_y(static_cast<double>(
      p_nav->accY * Coder_Accel_Scale / Coder_Sensor_Scale));
  imu_.mutable_linear_acceleration()->set_z(static_cast<double>(
      p_nav->accZ * Coder_Accel_Scale / Coder_Sensor_Scale));
  imu_.mutable_angular_velocity()->set_x(
      static_cast<double>(p_nav->gyroX * Coder_Rate_Scale /
                          Coder_Sensor_Scale) *
      DEG_TO_RAD);
  imu_.mutable_angular_velocity()->set_y(
      static_cast<double>(p_nav->gyroY * Coder_Rate_Scale /
                          Coder_Sensor_Scale) *
      DEG_TO_RAD);
  imu_.mutable_angular_velocity()->set_z(
      static_cast<double>(p_nav->gyroZ * Coder_Rate_Scale /
                          Coder_Sensor_Scale) *
      DEG_TO_RAD);
  bestpos_.set_measurement_time(seconds);
  bestpos_.set_longitude(static_cast<double>(p_nav->lon / Coder_Pos_Scale));
  bestpos_.set_latitude(static_cast<double>(p_nav->lat / Coder_Pos_Scale));
  bestpos_.set_height_msl(static_cast<double>(p_nav->alt / 1000.0));
  bestpos_.set_undulation(0.0);  // undulation = height_wgs84 - height_msl
  bestpos_.set_datum_id(
      static_cast<apollo::drivers::gnss::DatumId>(enbroad::DatumId::WGS84));
  bestpos_.set_latitude_std_dev(0.0);
  bestpos_.set_longitude_std_dev(0.0);
  bestpos_.set_height_std_dev(0.0);
  bestpos_.set_base_station_id("0");
  bestpos_.set_solution_age(0.0);  // solution age (sec)
  bestpos_.set_num_sats_tracked(Sate_Num);
  bestpos_.set_num_sats_in_solution(Sate_Num);
  bestpos_.set_num_sats_in_solution(Sate_Num);
  bestpos_.set_num_sats_multi(Sate_Num);
  bestpos_.set_extended_solution_status(SolutionType::INS_RTKFIXED);
  bestpos_.set_galileo_beidou_used_mask(0);
  bestpos_.set_gps_glonass_used_mask(0);
  heading_.set_measurement_time(seconds);
  heading_.set_pitch(static_cast<double>(p_nav->pitch * Coder_Angle_Scale /
                                         Coder_Sensor_Scale));
  // enbroad set northwest as right direction,
  // Here northeast as right direction
  heading_.set_heading(normalizeAngleTo180(-p_nav->head * Coder_Angle_Scale /
                                           Coder_Sensor_Scale));
  heading_.set_baseline_length(baseline);
  heading_.set_reserved(0);
  heading_.set_heading_std_dev(0.0);
  heading_.set_pitch_std_dev(0.0);
  heading_.set_station_id("0");
  heading_.set_satellite_tracked_number(Sate_Num);
  heading_.set_satellite_soulution_number(Sate_Num);
  heading_.set_satellite_number_obs(Sate_Num);
  heading_.set_satellite_number_multi(Sate_Num);
  heading_.set_solution_source(0);
  heading_.set_extended_solution_status(0);
  heading_.set_galileo_beidou_sig_mask(0);
  heading_.set_gps_glonass_sig_mask(0);
  ins_stat_.mutable_header()->set_timestamp_sec(cyber::Time::Now().ToSecond());
  // According to the RTK state:
  // fixed as "INS-RTKFIXED",float as "INS-RTKFLOAT",
  // single or RTD as "SINGLE",no solution as "NONE"
  if (enbroad::E_GPS_RTK_FIXED == rtkStatus) {
    ins_stat_.set_pos_type(SolutionType::INS_RTKFIXED);
    bestpos_.set_sol_type(SolutionType::INS_RTKFIXED);
    heading_.set_position_type(SolutionType::INS_RTKFIXED);
  } else if (enbroad::E_GPS_RTK_FLOAT == rtkStatus) {
    ins_stat_.set_pos_type(SolutionType::INS_RTKFLOAT);
    bestpos_.set_sol_type(SolutionType::INS_RTKFLOAT);
    heading_.set_position_type(SolutionType::INS_RTKFLOAT);
  } else if (enbroad::E_GPS_RTK_SPP == rtkStatus ||
             enbroad::E_GPS_RTK_DGPS == rtkStatus) {
    ins_stat_.set_pos_type(SolutionType::SINGLE);
    bestpos_.set_sol_type(SolutionType::SINGLE);
    heading_.set_position_type(SolutionType::SINGLE);
  } else {
    ins_stat_.set_pos_type(SolutionType::NONE);
    bestpos_.set_sol_type(SolutionType::NONE);
    heading_.set_position_type(SolutionType::NONE);
  }
  // According to sins calibration status:
  // no calibration as "SOL_COMPUTED",
  // during calibration as "SOL_COMPUTED",
  // completing calibration as "SOL_COMPUTED"
  if (enbroad::E_NAV_STANDARD_PROCCSSED == Nav_Standard_flag) {
    ins_stat_.set_ins_status(SolutionStatus::SOL_COMPUTED);
    bestpos_.set_sol_status(SolutionStatus::SOL_COMPUTED);
    heading_.set_solution_status(SolutionStatus::SOL_COMPUTED);
  } else if (enbroad::E_NAV_STANDARD_PROCCSSING == Nav_Standard_flag) {
    ins_stat_.set_ins_status(SolutionStatus::COLD_START);
    bestpos_.set_sol_status(SolutionStatus::COLD_START);
    heading_.set_solution_status(SolutionStatus::COLD_START);
  } else {
    ins_stat_.set_ins_status(SolutionStatus::INSUFFICIENT_OBS);
    bestpos_.set_sol_status(SolutionStatus::INSUFFICIENT_OBS);
    heading_.set_solution_status(SolutionStatus::INSUFFICIENT_OBS);
  }
  return true;
}

}  // namespace gnss
}  // namespace drivers
}  // namespace apollo

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

#include "gnss/parser/forsense/forsense_parser.h"

namespace apollo {
namespace drivers {
namespace gnss {

void ForsenseParser::GetMessages(std::vector<Parser::Message>* messages) {
  std::string frame;
  ExtractMessage(&frame);
  ParseMessage(frame);
  FillMessages(messages);
}

bool ForsenseParser::ExtractFrame(std::string* frame) {
  if (data_ == nullptr) {
    return false;
  }

  while (data_ < data_end_) {
    // Looking for '$'
    if (*data_ == NMEA_START_FLAG) {
      frame->assign(data_, data_end_);
      break;
    }
    ++data_;
  }

  return true;
}

bool ForsenseParser::ParseMessage(const std::string& frame) {
  if (!CheckCRC(frame)) {
    AERROR << "CRC check failed.";
    return false;
  }

  // Split the frame into items
  std::stringstream ss(frame);
  std::string item;
  std::vector<std::string> items;
  while (std::getline(ss, item, NMEA_FIELD_SEPARATOR)) {
    items.push_back(item);
  }

  if (items.empty()) {
    AERROR << "Invalid message.";
    return false;
  }

  // Parse message by type
  std::string message_id{items[0]};

  if (message_id == "$GPYJ") {
    HandleGPYJ(items);
  } else {
    AERROR << "Unknown message id: " << message_id;
    return false;
  }
  return true;
}

bool ForsenseParser::CheckCRC(const std::string& frame) {
  size_t l = frame.size() - huace::CRC_LENGTH;
  return crc32_block(frame.data(), l) ==
         *reinterpret_cast<uint32_t*>(frame.data() + l);
}

bool ForsenseParser::HandleGPYJ(const std::vector<std::string>& items) {
  str_to_char_array(items[0], gpyj_->header);
  gpyj_->gps_week = std::stoul(items[1]);
  gpyj_->gps_time = std::stod(items[2]);
  gpyj_->heading = std::stod(items[3]);
  gpyj_->pitch = std::stod(items[4]);
  gpyj_->roll = std::stod(items[5]);
  gpyj_->gyro_x = std::stod(items[6]);
  gpyj_->gyro_y = std::stod(items[7]);
  gpyj_->gyro_z = std::stod(items[8]);
  gpyj_->acc_x = std::stod(items[9]);
  gpyj_->acc_y = std::stod(items[10]);
  gpyj_->acc_z = std::stod(items[11]);
  gpyj_->latitude = std::stod(items[12]);
  gpyj_->longitude = std::stod(items[13]);
  gpyj_->altitude = std::stod(items[14]);
  gpyj_->ve = std::stod(items[15]);
  gpyj_->vn = std::stod(items[16]);
  gpyj_->vu = std::stod(items[17]);
  gpyj_->v = std::stod(items[18]);
  gpyj_->nsv1 = str_to_uint8(items[19]);
  gpyj_->nsv2 = str_to_uint8(items[20]);
  gpyj_->status = huace::Status(std::stoul(items[21]));
  gpyj_->age = std::stoul(items[22]);
  gpyj_->warning = std::stoul(items[23]);
  return true;
}

void ForsenseParser::FillMessages(std::vector<Parser::Message>* messages) {
  FillGnssBestpos();
  FillImu();
  FillHeading();
  FillIns();
  FillInsStat();

  // Fill messages
  messages->emplace_back(MessageType::BEST_GNSS_POS, bestpos_);
  messages->emplace_back(MessageType::IMU, imu_);
  messages->emplace_back(MessageType::INS, ins_);
  messages->emplace_back(MessageType::INS_STAT, ins_stat_);
  messages->emplace_back(MessageType::HEADING, heading_);
}

apollo::drivers::gnss::SolutionStatus ToSolutionStatus(
    const huace::Status& status) {
  switch (status) {
    case Status::RTK_STABLE_ORIENT:
      return SolutionStatus::INS_RTKFIXED;
    case Status::SINGLE_POS_NO_ORIENT:
      return SolutionStatus::INSUFFICIENT_OBS;
    case Status::RTK_FLOAT_NO_ORIENT:
      return SolutionStatus::NO_CONVERGENCE;
    case Status::NO_POS_NO_ORIENT:
      return SolutionStatus::COLD_START;
    case Status::PSEUDORANGE_DIFF_NO_ORIENT:
      return SolutionStatus::V_H_LIMIT;
    case Status::PSEUDORANGE_DIFF_ORIENT:
      return SolutionStatus::INVALID_FIX;
    case Status::INIT:
      return SolutionStatus::INVALID_RATE;
    default:
      // TODO(zero):  // Handle other cases
      return SolutionStatus::INS_RTKFIXED;
  }
}

apollo::drivers::gnss::SolutionType ToSolutionType(
    const huace::Status& status) {
  switch (status) {
    case Status::SINGLE_POS_ORIENT:
      return SolutionType::FIXEDPOS;
    case Status::RTK_FLOAT_ORIENT:
      return SolutionType::FLOATCONV;
    case Status::RTK_STABLE_ORIENT:
      return SolutionType::WIDELANE;
    case Status::SINGLE_POS_NO_ORIENT:
      return SolutionType::SINGLE;
    case Status::COMBINED_PREDICTION:
      return SolutionType::RTK_DIRECT_INS;
    case Status::RTK_FLOAT_NO_ORIENT:
      return SolutionType::PPP;
    default:
      // TODO(zero):  // Handle other cases
      return SolutionType::FIXEDPOS;
  }
}

void ForsenseParser::FillGnssBestpos() {
  bestpos_.set_measurement_time(gpyj_.gps_week * kSecondsPerWeek +
                                gpyj_.gps_time);
  bestpos_.set_sol_status(ToSolutionStatus(gpyj_.status));
  bestpos_.set_sol_type(ToSolutionType(gpyj_.status));
  bestpos_.set_latitude(gpyj_.latitude);
  bestpos_.set_longitude(gpyj_.longitude);
  bestpos_.set_height_msl(gpyj_.altitude);
  bestpos_.set_latitude_std_dev(gpyj_.latitude_std);
  bestpos_.set_longitude_std_dev(gpyj_.longitude_std);
  bestpos_.set_height_std_dev(gpyj_.altitude_std);
  bestpos_.set_num_sats_tracked(gpyj_.nsv1 + gpyj_.nsv2);
  // bestpos_.set_num_sats_in_solution(gpyj_.satellites_num);
  // bestpos_.set_num_sats_l1(gpyj_.satellites_num);
  // bestpos_.set_num_sats_multi(gpyj_.satellites_num);
}

void ForsenseParser::FillImu() {
  imu_.set_measurement_time(gpyj_.gps_week * kSecondsPerWeek +
                            gpyj_.gps_time);

  rfu_to_flu(gpyj_.acc_x, gpyj_.acc_y, gpyj_.acc_z,
             imu_.mutable_linear_acceleration());
  rfu_to_flu(gpyj_.gyro_x, gpyj_.gyro_y, gpyj_.gyro_z,
             imu_.mutable_angular_velocity());
}

void ForsenseParser::FillHeading() {
  heading_.set_measurement_time(gpyj_.gps_week * kSecondsPerWeek +
                                gpyj_.gps_time);

  heading_.set_sol_status(ToSolutionStatus(gpyj_.status));
  heading_.set_sol_type(ToSolutionType(gpyj_.status));

  heading_.set_heading(gpyj_.heading);
  heading_.set_pitch(gpyj_.pitch);
  heading_.set_heading_std_dev(gpyj_.heading_std);
  heading_.set_pitch_std_dev(gpyj_.pitch_std);

  // heading_.set_station_id("0");
  // heading_.set_satellite_number_multi(gpyj_.satellites_num);
  // heading_.set_satellite_soulution_number(gpyj_.satellites_num);
}

void ForsenseParser::FillIns() {
  ins_.mutable_header()->set_timestamp_sec(cyber::Time::Now().ToSecond());
  ins_.set_measurement_time(gpyj_.gps_week * kSecondsPerWeek +
                            gpyj_.gps_time);

  solution_type = ToSolutionType(gpyj_.status);
  switch (solution_type) {
    case SolutionType::INS_RTKFIXED:
    case SolutionType::NARROW_INT:
    case SolutionType::INS_RTKFLOAT:
    case SolutionType::NARROW_FLOAT:
      ins_.set_type(Ins::GOOD);
      break;
    case SolutionType::SINGLE:
      ins_.set_type(Ins::CONVERGING);
      break;
    default:
      ins_.set_type(Ins::INVALID);
      break;
  }

  ins_.mutable_position()->set_lon(gpyj_.longitude);
  ins_.mutable_position()->set_lat(gpyj_.latitude);
  ins_.mutable_position()->set_height(gpyj_.altitude);

  ins_.mutable_euler_angles()->set_x(gpyj_.roll * DEG_TO_RAD);
  ins_.mutable_euler_angles()->set_y(-gpyj_.pitch * DEG_TO_RAD);
  ins_.mutable_euler_angles()->set_z(azimuth_deg_to_yaw_rad(gpyj_.heading));

  ins_.mutable_linear_velocity()->set_x(gpyj_.ve);
  ins_.mutable_linear_velocity()->set_y(gpyj_.vn);
  ins_.mutable_linear_velocity()->set_z(gpyj_.vu);

  rfu_to_flu(gpyj_.gyro_x, gpyj_.gyro_y, gpyj_.gyro_z,
             ins_.mutable_angular_velocity());

  rfu_to_flu(gpyj_.acc_x, gpyj_.acc_y, gpyj_.acc_z,
             ins_.mutable_linear_acceleration());
}

void ForsenseParser::FillInsStat() {
  ins_stat_.set_ins_status(gpyj_.status);
  // ins_stat_.set_pos_type(gpyj_.solution_type);
}

}  // namespace gnss
}  // namespace drivers
}  // namespace apollo

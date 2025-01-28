/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

#include "gnss/util/macros.h"
#include "google/protobuf/message.h"

#include "gnss/proto/config.pb.h"

namespace apollo {
namespace drivers {
namespace gnss {

// Anonymous namespace that contains helper constants and functions.
namespace {

constexpr int kSecondsPerWeek = 60 * 60 * 24 * 7;

constexpr double kDegToRad = M_PI / 180.0;

constexpr float kFloatNaN = std::numeric_limits<float>::quiet_NaN();

// The NovAtel's orientation covariance matrix is pitch, roll, and yaw. We use
// the index array below
// to convert it to the orientation covariance matrix with order roll, pitch,
// and yaw.
constexpr int INDEX[] = {4, 3, 5, 1, 0, 2, 7, 6, 8};
static_assert(sizeof(INDEX) == 9 * sizeof(int), "Incorrect size of INDEX");

template <typename T>
constexpr bool is_zero(T value) {
  return value == static_cast<T>(0);
}

// CRC algorithm from the NovAtel document.
inline uint32_t crc32_word(uint32_t word) {
  for (int j = 0; j < 8; ++j) {
    if (word & 1) {
      word = (word >> 1) ^ 0xEDB88320;
    } else {
      word >>= 1;
    }
  }
  return word;
}

inline uint32_t crc32_block(const uint8_t *buffer, size_t length) {
  uint32_t word = 0;
  while (length--) {
    uint32_t t1 = (word >> 8) & 0xFFFFFF;
    uint32_t t2 = crc32_word((word ^ *buffer++) & 0xFF);
    word = t1 ^ t2;
  }
  return word;
}

// Converts NovAtel's azimuth (north = 0, east = 90) to FLU yaw (east = 0, north
// = pi/2).
constexpr double azimuth_deg_to_yaw_rad(double azimuth) {
  return (90.0 - azimuth) * DEG_TO_RAD;
}

// A helper that fills an Point3D object (which uses the FLU frame) using RFU
// measurements.
inline void rfu_to_flu(double r, double f, double u,
                       ::apollo::common::Point3D *flu) {
  flu->set_x(f);
  flu->set_y(-r);
  flu->set_z(u);
}

uint8_t str_to_uint8(const std::string &str) {
  try {
    unsigned long num = std::stoul(str);

    if (num > 255) {
      throw std::out_of_range("Number out of uint8_t range");
    }

    return static_cast<uint8_t>(num);
  } catch (const std::invalid_argument &e) {
    throw std::invalid_argument("Invalid number format");
  } catch (const std::out_of_range &e) {
    throw std::out_of_range("Number out of uint8_t range");
  }
}

void str_to_char_array(const std::string &src, char *dest) {
  strncpy(dest, src.c_str(), size_of(src));
}

}  // namespace


constexpr char NMEA_START_FLAG = '$';         // Start flag
constexpr char NMEA_FIELD_SEPARATOR = ',';    // Field separator
constexpr char NMEA_CHECKSUM_SEPARATOR = '*'; // Checksum separator
constexpr char NMEA_END_OF_LINE_CR = '\r';    // End of line: Carriage return
constexpr char NMEA_END_OF_LINE_LF = '\n';    // End of line: Line feed

// convert gps time (base on Jan 6 1980) to system time (base on Jan 1 1970)
// notice: Jan 6 1980
//
// linux shell:
// time1 = date +%s -d"Jan 6, 1980 00:00:01"
// time2 = date +%s -d"Jan 1, 1970 00:00:01"
// dif_tick = time1-time2
// 315964800 = 315993601 - 28801

#define EPOCH_AND_SYSTEM_DIFF_SECONDS 315964800

// A helper function that returns a pointer to a protobuf message of type T.
template <class T>
inline T *As(::google::protobuf::Message *message_ptr) {
  return dynamic_cast<T *>(message_ptr);
}

// An abstract class of Parser.
// One should use the create_xxx() functions to create a Parser object.
class Parser {
 public:
  // A general pointer to a protobuf message.
  using MessagePtr = ::google::protobuf::Message *;
  using Message = std::pair<MessageType, MessagePtr>;

  virtual ~Parser() = default;

  static std::unique_ptr<Parser> CreateParser(const config::Config &config);

  // Return a pointer to rtcm v3 parser. The caller should take ownership.
  static Parser *CreateRtcmV3(bool is_base_station = false);

  // Updates the parser with new data. The caller must keep the data valid until
  // GetMessage() returns NONE.
  void Update(const uint8_t *data, size_t length) {
    data_ = data;
    data_end_ = data + length;
  }

  void Update(const std::string &data) {
    Update(reinterpret_cast<const uint8_t *>(data.data()), data.size());
  }

  enum class MessageType {
    NONE,
    GNSS,
    GNSS_RANGE,
    IMU,
    INS,
    INS_STAT,
    WHEEL,
    EPHEMERIDES,
    OBSERVATION,
    GPGGA,
    BDSEPHEMERIDES,
    RAWIMU,
    GPSEPHEMERIDES,
    GLOEPHEMERIDES,
    BEST_GNSS_POS,
    HEADING,
  };

  // Gets a parsed protobuf message. The caller must consume the message before
  // calling another
  // GetMessage() or Update();
  virtual void GetMessages(std::vector<Parser::Message> *messages) = 0;

 protected:
  Parser() = default;

  // Point to the beginning and end of data. Do not take ownership.
  const uint8_t *data_ = nullptr;
  const uint8_t *data_end_ = nullptr;

 private:
  DISABLE_COPY_AND_ASSIGN(Parser);
};

std::unique_ptr<Parser> Parser::CreateParser(const config::Config &config) {
  std::unordered_map<config::Stream, std::function<std::unique_ptr<Parser>(
                                         const config::Config &)>>
      parser_map = {
          {config::Stream::NOVATEL_BINARY,
           [](const config::Config &cfg) {
             return std::make_unique<NovatelParser>(cfg);
           }},
          {config::Stream::HUACE_TEXT,
           [](const config::Config &cfg) {
             return std::make_unique<HuaCeTextParser>(cfg);
           }},
          // Add more parser types here...
      };

  auto it = parser_map.find(config.data().format());
  if (it != parser_map.end()) {
    return it->second(config);
  }

  return nullptr;
}

}  // namespace gnss
}  // namespace drivers
}  // namespace apollo

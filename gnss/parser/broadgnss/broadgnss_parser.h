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

#include "gnss/parser/broadgnss/broadgnss_messages.h"
#include "gnss/parser/parser.h"

namespace apollo {
namespace drivers {
namespace gnss {

class BroadgnssParser : public Parser {
 public:
  BroadgnssParser() = default;
  ~BroadgnssParser() = default;

  virtual void GetMessages(std::vector<Parser::Message>* messages);

 private:
  Parser::MessageType PrepareMessage(MessagePtr* message_ptr);

  ::apollo::drivers::gnss::GnssBestPose bestpos_;
  ::apollo::drivers::gnss::Imu imu_;
  ::apollo::drivers::gnss::Ins ins_;
  ::apollo::drivers::gnss::InsStat ins_stat_;
  ::apollo::drivers::gnss::Heading heading_;

  broadgnss::GPYJ gpyj_;
};

}  // namespace gnss
}  // namespace drivers
}  // namespace apollo

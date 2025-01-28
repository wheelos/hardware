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

#include "vehicles/vehicle/yuhesen_mkmini/yuhesen_message_manager.h"

#include "gtest/gtest.h"

#include "modules/canbus/vehicle/yuhesen/protocol/bms_report_512.h"
#include "modules/canbus/vehicle/yuhesen/protocol/brake_command_101.h"
#include "modules/canbus/vehicle/yuhesen/protocol/brake_report_501.h"
#include "modules/canbus/vehicle/yuhesen/protocol/gear_command_103.h"
#include "modules/canbus/vehicle/yuhesen/protocol/gear_report_503.h"
#include "modules/canbus/vehicle/yuhesen/protocol/park_command_104.h"
#include "modules/canbus/vehicle/yuhesen/protocol/park_report_504.h"
#include "modules/canbus/vehicle/yuhesen/protocol/steering_command_102.h"
#include "modules/canbus/vehicle/yuhesen/protocol/steering_report_502.h"
#include "modules/canbus/vehicle/yuhesen/protocol/throttle_command_100.h"
#include "modules/canbus/vehicle/yuhesen/protocol/throttle_report_500.h"
#include "modules/canbus/vehicle/yuhesen/protocol/ultr_sensor_1_507.h"
#include "modules/canbus/vehicle/yuhesen/protocol/ultr_sensor_2_508.h"
#include "modules/canbus/vehicle/yuhesen/protocol/ultr_sensor_3_509.h"
#include "modules/canbus/vehicle/yuhesen/protocol/ultr_sensor_4_510.h"
#include "modules/canbus/vehicle/yuhesen/protocol/ultr_sensor_5_511.h"
#include "modules/canbus/vehicle/yuhesen/protocol/vcu_report_505.h"
#include "modules/canbus/vehicle/yuhesen/protocol/wheelspeed_report_506.h"

namespace apollo {
namespace canbus {
namespace yuhesen {

class YuhesenMessageManagerTest : public ::testing::Test {
 public:
  virtual void SetUp() {}

 protected:
  YuhesenMessageManager manager_;
};

TEST_F(YuhesenMessageManagerTest, GetSendProtocols) {
  EXPECT_NE(manager_.GetMutableProtocolDataById(Brakecommand101::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Gearcommand103::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Parkcommand104::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Steeringcommand102::ID),
            nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Throttlecommand100::ID),
            nullptr);
}

TEST_F(YuhesenMessageManagerTest, GetRecvProtocols) {
  EXPECT_NE(manager_.GetMutableProtocolDataById(Bmsreport512::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Brakereport501::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Gearreport503::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Parkreport504::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Steeringreport502::ID),
            nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Throttlereport500::ID),
            nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Ultrsensor1507::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Ultrsensor2508::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Ultrsensor3509::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Ultrsensor4510::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Ultrsensor5511::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Vcureport505::ID), nullptr);
  EXPECT_NE(manager_.GetMutableProtocolDataById(Wheelspeedreport506::ID),
            nullptr);
}

}  // namespace yuhesen
}  // namespace canbus
}  // namespace apollo

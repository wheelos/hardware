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

#include "vehicles/vehicle/yuhesen_mkmini/protocol/bms_flag_infor.h"
#include "vehicles/vehicle/yuhesen_mkmini/protocol/bms_infor.h"
#include "vehicles/vehicle/yuhesen_mkmini/protocol/ctrl_cmd.h"
#include "vehicles/vehicle/yuhesen_mkmini/protocol/ctrl_fb.h"
#include "vehicles/vehicle/yuhesen_mkmini/protocol/io_cmd.h"
#include "vehicles/vehicle/yuhesen_mkmini/protocol/io_fb.h"
#include "vehicles/vehicle/yuhesen_mkmini/protocol/lr_wheel_fb.h"
#include "vehicles/vehicle/yuhesen_mkmini/protocol/odo_fb.h"
#include "vehicles/vehicle/yuhesen_mkmini/protocol/rr_wheel_fb.h"
#include "vehicles/vehicle/yuhesen_mkmini/protocol/ultrasonic_1_fb.h"
#include "vehicles/vehicle/yuhesen_mkmini/protocol/ultrasonic_2_fb.h"
#include "vehicles/vehicle/yuhesen_mkmini/protocol/veh_fb_diag.h"

namespace apollo {
namespace canbus {
namespace yuhesen {

YuhesenMessageManager::YuhesenMessageManager() {
  // Control Messages
  AddSendProtocolData<Ctrl_cmd, true>();
  AddSendProtocolData<IO_cmd, true>();

  // Report Messages
  AddRecvProtocolData<Ctrl_fb, true>();
  AddRecvProtocolData<Lr_wheel_fb, true>();
  AddRecvProtocolData<Rr_wheel_fb, true>();
  AddRecvProtocolData<IO_fb, true>();
  AddRecvProtocolData<Odo_fb, true>();
  AddRecvProtocolData<Bms_Infor, true>();
  AddRecvProtocolData<Bms_flag_Infor, true>();
  AddRecvProtocolData<Ultrasonic_1_fb, true>();
  AddRecvProtocolData<Ultrasonic_2_fb, true>();
  AddRecvProtocolData<Veh_fb_diag, true>();
}

YuhesenMessageManager::~YuhesenMessageManager() {}

}  // namespace yuhesen
}  // namespace canbus
}  // namespace apollo

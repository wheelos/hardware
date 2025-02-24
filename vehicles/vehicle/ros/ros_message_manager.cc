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

#include "vehicles/vehicle/ros/ros_message_manager.h"

#include "vehicles/vehicle/ros/protocol/bms_flag_infor.h"
#include "vehicles/vehicle/ros/protocol/bms_infor.h"
#include "vehicles/vehicle/ros/protocol/ctrl_cmd.h"
#include "vehicles/vehicle/ros/protocol/ctrl_fb.h"
#include "vehicles/vehicle/ros/protocol/io_cmd.h"
#include "vehicles/vehicle/ros/protocol/io_fb.h"
#include "vehicles/vehicle/ros/protocol/lr_wheel_fb.h"
#include "vehicles/vehicle/ros/protocol/odo_fb.h"
#include "vehicles/vehicle/ros/protocol/rr_wheel_fb.h"
#include "vehicles/vehicle/ros/protocol/ultrasonic_1_fb.h"
#include "vehicles/vehicle/ros/protocol/ultrasonic_2_fb.h"
#include "vehicles/vehicle/ros/protocol/veh_fb_diag.h"

namespace apollo {
namespace canbus {
namespace ros {

RosMessageManager::RosMessageManager() {
  // Control Messages
  AddSendProtocolData<TwistCmd, true>();

  // Report Messages
  AddRecvProtocolData<TwistFb, true>();
  AddRecvProtocolData<AccFb, true>();
  AddRecvProtocolData<AngVelFb, true>();
}

RosMessageManager::~RosMessageManager() {}

}  // namespace ros
}  // namespace canbus
}  // namespace apollo

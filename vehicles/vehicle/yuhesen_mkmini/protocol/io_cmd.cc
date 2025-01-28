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

#include "vehicles/vehicle/yuhesen_mkmini/protocol/io_cmd.h"

#include "modules/drivers/canbus/common/byte.h"

namespace apollo {
namespace canbus {
namespace yuhesen {

using ::apollo::drivers::canbus::Byte;

const int32_t IOCmd::ID = 0x103;

// public
IOCmd::IOCmd() { Reset(); }

uint32_t IOCmd::GetPeriod() const {
  // TODO(All) :  modify every protocol's period manually
  static const uint32_t PERIOD = 20 * 1000;
  return PERIOD;
}

void IOCmd::UpdateData(uint8_t* data) {
  set_p_gear_target(data, gear_target_);
  set_p_gear_en_ctrl(data, gear_en_ctrl_);
  checksum_103_ =
      data[0] ^ data[1] ^ data[2] ^ data[3] ^ data[4] ^ data[5] ^ data[6];
  set_p_checksum_103(data, checksum_103_);
}

void IOCmd::Reset() {
  // TODO(All) :  you should check this manually
  gear_target_ = Gear_command_103::GEAR_TARGET_NEUTRAL;
  gear_en_ctrl_ = Gear_command_103::GEAR_EN_CTRL_DISABLE;
  checksum_103_ = 0;
}

}  // namespace yuhesen
}  // namespace canbus
}  // namespace apollo

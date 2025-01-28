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

#include "vehicles/vehicle/yuhesen_mkmini/protocol/bms_infor.h"

#include "glog/logging.h"
#include "modules/drivers/canbus/common/byte.h"
#include "modules/drivers/canbus/common/canbus_consts.h"

namespace apollo {
namespace canbus {
namespace yuhesen {

using ::apollo::drivers::canbus::Byte;

BmsInfoR::BmsInfoR() {}
const int32_t BmsInfoR::ID = 0x512;

void BmsInfoR::Parse(const std::uint8_t* bytes, int32_t length,
                         ChassisDetail* chassis) const {

}

}  // namespace yuhesen
}  // namespace canbus
}  // namespace apollo

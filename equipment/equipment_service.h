// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "cyber/node/node.h"
#include "cyber/service/service_base.h"
#include "modules/drivers/equipment/equipment_runtime.h"

namespace apollo::drivers::equipment {
class EquipmentService {
 public:
  msg::EquipmentError Start(std::shared_ptr<cyber::Node> node,
                            std::shared_ptr<EquipmentRuntime> runtime,
                            const std::string& root);
  void Stop();

 private:
  std::vector<std::shared_ptr<cyber::ServiceBase>> services_;
};
}  // namespace apollo::drivers::equipment

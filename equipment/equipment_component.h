// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <memory>

#include "cyber/component/component.h"
#include "modules/drivers/equipment/equipment_service.h"

namespace apollo::drivers::equipment {
class EquipmentComponent : public cyber::Component<> {
 public:
  ~EquipmentComponent() override;
  bool Init() override;

 protected:
  virtual DeviceFactory CreateFactory() const { return DeviceFactory(); }

 private:
  void Clear() override;
  std::shared_ptr<EquipmentRuntime> runtime_;
  EquipmentService service_;
};
CYBER_REGISTER_COMPONENT(EquipmentComponent)
}  // namespace apollo::drivers::equipment

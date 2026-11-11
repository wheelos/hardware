// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/equipment_component.h"

#include <memory>

namespace apollo::drivers::equipment {
EquipmentComponent::~EquipmentComponent() { Clear(); }

bool EquipmentComponent::Init() {
  EquipmentModuleConfig config;
  if (!GetProtoConfig(&config)) {
    AERROR << "Unable to load equipment configuration";
    return false;
  }
  runtime_ = std::make_shared<EquipmentRuntime>(CreateFactory());
  auto error = runtime_->Initialize(config);
  if (IsOk(error)) {
    error = runtime_->Start();
  }
  if (IsOk(error)) {
    error = service_.Start(node_, runtime_, config.service_root());
  }
  if (!IsOk(error)) {
    AERROR << error.ShortDebugString();
    Clear();
    return false;
  }
  return true;
}

void EquipmentComponent::Clear() {
  service_.Stop();
  if (runtime_) {
    const auto error = runtime_->Shutdown();
    if (!IsOk(error)) {
      AERROR << error.ShortDebugString();
    }
    runtime_.reset();
  }
}
}  // namespace apollo::drivers::equipment

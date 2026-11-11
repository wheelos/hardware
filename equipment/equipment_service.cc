// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/equipment_service.h"

#include <memory>
#include <string>

#include "cyber/service_discovery/topology_manager.h"

namespace apollo::drivers::equipment {
namespace {
template <typename Request, typename Response, typename Method>
std::shared_ptr<cyber::ServiceBase> Bind(
    const std::shared_ptr<cyber::Node>& node,
    const std::shared_ptr<EquipmentRuntime>& runtime, const std::string& name,
    Method method) {
  if (cyber::service_discovery::TopologyManager::Instance()
          ->service_manager()
          ->HasService(name)) {
    AERROR << "Equipment service already exists: " << name;
    return nullptr;
  }
  return node->CreateService<Request, Response>(
      name, [runtime, method](const std::shared_ptr<Request>& request,
                              std::shared_ptr<Response>& response) {
        *response = ((*runtime).*method)(*request);
      });
}
}  // namespace

msg::EquipmentError EquipmentService::Start(
    std::shared_ptr<cyber::Node> node,
    std::shared_ptr<EquipmentRuntime> runtime, const std::string& root) {
  if (!node || !runtime || !services_.empty() || root.empty() ||
      root.front() != '/' || root.back() == '/') {
    return Error(msg::INVALID_ARGUMENT, "invalid service owner or root");
  }
  services_.push_back(Bind<msg::ListDevicesRequest, msg::ListDevicesResponse>(
      node, runtime, root + "/list", &EquipmentRuntime::ListDevices));
  services_.push_back(Bind<msg::ExecuteRequest, msg::ExecuteResponse>(
      node, runtime, root + "/execute", &EquipmentRuntime::Execute));
  services_.push_back(Bind<msg::GetStateRequest, msg::GetStateResponse>(
      node, runtime, root + "/get_state", &EquipmentRuntime::GetState));
  services_.push_back(Bind<msg::GetOperationRequest, msg::GetOperationResponse>(
      node, runtime, root + "/get_operation", &EquipmentRuntime::GetOperation));
  services_.push_back(
      Bind<msg::CancelOperationRequest, msg::CancelOperationResponse>(
          node, runtime, root + "/cancel", &EquipmentRuntime::CancelOperation));
  for (const auto& service : services_) {
    if (!service) {
      Stop();
      return Error(msg::RPC_UNAVAILABLE, "equipment service creation failed");
    }
  }
  return Error(msg::OK);
}

void EquipmentService::Stop() { services_.clear(); }
}  // namespace apollo::drivers::equipment

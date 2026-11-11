// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "cyber/node/node.h"
#include "modules/drivers/equipment/device.h"

namespace apollo::drivers::equipment {
struct DeviceTarget {
  std::string device_id;
  std::string capability_id;
};
struct ExecuteContext {
  std::string runtime_instance_id;
  msg::RequestKey request_key;
  uint32_t execution_timeout_ms = 0;
  std::chrono::seconds rpc_timeout{0};
};
class EquipmentClient {
 public:
  msg::EquipmentError Initialize(std::shared_ptr<cyber::Node> node,
                                 const std::string& root);
  bool WaitForServices(std::chrono::milliseconds timeout);
  msg::ListDevicesResponse ListDevices(const msg::ListDevicesRequest& request,
                                       std::chrono::seconds timeout);
  msg::ExecuteResponse Execute(const msg::ExecuteRequest& request,
                               std::chrono::seconds timeout);
  msg::GetStateResponse GetState(const msg::GetStateRequest& request,
                                 std::chrono::seconds timeout);
  msg::GetOperationResponse GetOperation(
      const msg::GetOperationRequest& request, std::chrono::seconds timeout);
  msg::CancelOperationResponse CancelOperation(
      const msg::CancelOperationRequest& request, std::chrono::seconds timeout);
  template <typename Goal>
  msg::ExecuteResponse Submit(const DeviceTarget& target,
                              const ExecuteContext& context,
                              const std::string& action, const Goal& goal) {
    msg::ExecuteRequest request;
    request.set_device_id(target.device_id);
    request.set_runtime_instance_id(context.runtime_instance_id);
    *request.mutable_request_key() = context.request_key;
    request.set_execution_timeout_ms(context.execution_timeout_ms);
    request.mutable_command()->set_capability_id(target.capability_id);
    request.mutable_command()->set_action(action);
    if (!request.mutable_command()->mutable_arguments()->PackFrom(goal)) {
      msg::ExecuteResponse response;
      *response.mutable_error() =
          Error(msg::INVALID_ARGUMENT, "cannot serialize typed device goal");
      AERROR << response.error().ShortDebugString();
      return response;
    }
    return Execute(request, context.rpc_timeout);
  }

 private:
  std::shared_ptr<
      cyber::Client<msg::ListDevicesRequest, msg::ListDevicesResponse>>
      list_;
  std::shared_ptr<cyber::Client<msg::ExecuteRequest, msg::ExecuteResponse>>
      execute_;
  std::shared_ptr<cyber::Client<msg::GetStateRequest, msg::GetStateResponse>>
      state_;
  std::shared_ptr<
      cyber::Client<msg::GetOperationRequest, msg::GetOperationResponse>>
      operation_;
  std::shared_ptr<
      cyber::Client<msg::CancelOperationRequest, msg::CancelOperationResponse>>
      cancel_;
};
}  // namespace apollo::drivers::equipment

// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/equipment_client.h"

#include <memory>
#include <string>

namespace apollo::drivers::equipment {
namespace {
template <typename Request, typename Response>
Response Call(const std::shared_ptr<cyber::Client<Request, Response>>& client,
              const Request& request, std::chrono::seconds timeout,
              bool may_have_executed = false) {
  Response result;
  if (timeout.count() <= 0) {
    *result.mutable_error() =
        Error(msg::INVALID_ARGUMENT, "positive RPC timeout required");
  } else if (!client || !client->ServiceIsReady()) {
    *result.mutable_error() =
        Error(msg::RPC_UNAVAILABLE, "equipment service unavailable");
  } else {
    const auto response =
        client->SendRequest(std::make_shared<Request>(request), timeout);
    if (!response) {
      *result.mutable_error() =
          Error(msg::RPC_TIMEOUT, "RPC result unknown; do not resend goal",
                may_have_executed);
    } else if (!response->has_error() || !response->error().has_code()) {
      *result.mutable_error() =
          Error(msg::RPC_UNAVAILABLE, "malformed equipment response",
                may_have_executed);
    } else {
      result = *response;
    }
  }
  if (!IsOk(result.error())) {
    AERROR << result.error().ShortDebugString();
  }
  return result;
}

template <typename Client>
bool Wait(const Client& client,
          std::chrono::steady_clock::time_point deadline) {
  const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
      deadline - std::chrono::steady_clock::now());
  return client && left.count() > 0 && client->WaitForService(left);
}
}  // namespace

msg::EquipmentError EquipmentClient::Initialize(
    std::shared_ptr<cyber::Node> node, const std::string& root) {
  if (!node || list_ || root.empty() || root.front() != '/' ||
      root.back() == '/') {
    return Error(msg::INVALID_ARGUMENT, "invalid client node or service root");
  }
  list_ = node->CreateClient<msg::ListDevicesRequest, msg::ListDevicesResponse>(
      root + "/list");
  execute_ = node->CreateClient<msg::ExecuteRequest, msg::ExecuteResponse>(
      root + "/execute");
  state_ = node->CreateClient<msg::GetStateRequest, msg::GetStateResponse>(
      root + "/get_state");
  operation_ =
      node->CreateClient<msg::GetOperationRequest, msg::GetOperationResponse>(
          root + "/get_operation");
  cancel_ = node->CreateClient<msg::CancelOperationRequest,
                               msg::CancelOperationResponse>(root + "/cancel");
  if (!list_ || !execute_ || !state_ || !operation_ || !cancel_) {
    list_.reset();
    execute_.reset();
    state_.reset();
    operation_.reset();
    cancel_.reset();
    return Error(msg::RPC_UNAVAILABLE, "equipment client creation failed");
  }
  return Error(msg::OK);
}

bool EquipmentClient::WaitForServices(std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  return Wait(list_, deadline) && Wait(execute_, deadline) &&
         Wait(state_, deadline) && Wait(operation_, deadline) &&
         Wait(cancel_, deadline);
}

msg::ListDevicesResponse EquipmentClient::ListDevices(
    const msg::ListDevicesRequest& request, std::chrono::seconds timeout) {
  return Call(list_, request, timeout);
}
msg::ExecuteResponse EquipmentClient::Execute(
    const msg::ExecuteRequest& request, std::chrono::seconds timeout) {
  return Call(execute_, request, timeout, true);
}
msg::GetStateResponse EquipmentClient::GetState(
    const msg::GetStateRequest& request, std::chrono::seconds timeout) {
  return Call(state_, request, timeout);
}
msg::GetOperationResponse EquipmentClient::GetOperation(
    const msg::GetOperationRequest& request, std::chrono::seconds timeout) {
  return Call(operation_, request, timeout);
}
msg::CancelOperationResponse EquipmentClient::CancelOperation(
    const msg::CancelOperationRequest& request, std::chrono::seconds timeout) {
  return Call(cancel_, request, timeout, true);
}

}  // namespace apollo::drivers::equipment

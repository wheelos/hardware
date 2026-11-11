// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/device.h"

#include <string>

namespace apollo::drivers::equipment {
msg::EquipmentError Error(msg::ErrorCode code, const std::string& message,
                          bool may_have_executed) {
  msg::EquipmentError result;
  result.set_code(code);
  result.set_message(message);
  result.set_may_have_executed(may_have_executed);
  return result;
}
bool IsOk(const msg::EquipmentError& error) {
  return error.has_code() && error.code() == msg::OK;
}
bool IsTerminal(msg::OperationState state) {
  return state == msg::SUCCEEDED || state == msg::FAILED ||
         state == msg::CANCELED || state == msg::TIMED_OUT ||
         state == msg::UNKNOWN;
}
uint64_t UnixTimeNs() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}
msg::EquipmentError Device::ValidateEnvelope(
    const msg::DeviceCommand& command) const {
  const auto descriptor = Describe();
  for (const auto& cap : descriptor.capabilities()) {
    if (cap.capability_id() != command.capability_id()) {
      continue;
    }
    for (const auto& action : cap.actions()) {
      if (action.name() != command.action()) {
        continue;
      }
      const auto& url = command.arguments().type_url();
      const auto slash = url.rfind('/');
      if (slash == std::string::npos ||
          url.substr(slash + 1) != action.request_type()) {
        return Error(msg::INVALID_ARGUMENT, "action payload type mismatch");
      }
      return action.supports_completion_feedback()
                 ? Error(msg::OK)
                 : Error(msg::COMPLETION_UNSUPPORTED, "no completion feedback");
    }
  }
  return Error(msg::UNSUPPORTED_CAPABILITY, "unknown capability or action");
}
}  // namespace apollo::drivers::equipment

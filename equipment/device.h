// Copyright 2026 WheelOS. All Rights Reserved.
#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "modules/drivers/equipment/proto/equipment_config.pb.h"
#include "wheelos_msgs/equipment_msgs/equipment_service.pb.h"

namespace apollo::drivers::equipment {
namespace msg = ::apollo::equipment;
using SteadyTime = std::chrono::steady_clock::time_point;

msg::EquipmentError Error(msg::ErrorCode code, const std::string& message = "",
                          bool may_have_executed = false);
bool IsOk(const msg::EquipmentError& error);
bool IsTerminal(msg::OperationState state);
uint64_t UnixTimeNs();

struct StateSample {
  msg::CapabilityState state;
  SteadyTime received_at;
};
struct OperationContext {
  std::string operation_id;
  SteadyTime deadline;
  // Runtime lower bound; devices refine it with the backend dispatch boundary.
  SteadyTime evidence_after;
};
struct OperationEvent {
  std::string operation_id;
  msg::OperationState state = msg::OPERATION_UNSPECIFIED;
  msg::EquipmentError error;
  StateSample evidence;
  google::protobuf::Any result;
  bool already_satisfied = false;
};
struct DeviceUpdate {
  std::vector<StateSample> samples;
  std::vector<OperationEvent> operations;
};

// Only the device worker calls lifecycle/I/O methods. Query methods must be
// thread-safe; service callbacks never call the backend directly.
class Device {
 public:
  virtual ~Device() = default;
  virtual msg::EquipmentError Initialize(const DeviceConfig& config) = 0;
  virtual msg::DeviceDescriptor Describe() const = 0;
  virtual msg::EquipmentError Activate() = 0;
  virtual msg::EquipmentError Validate(
      const msg::DeviceCommand& command) const = 0;
  virtual msg::EquipmentError Begin(const OperationContext& context,
                                    const msg::DeviceCommand& command) = 0;
  virtual msg::EquipmentError Poll(DeviceUpdate* update) = 0;
  virtual msg::EquipmentError Cancel(const std::string& operation_id) = 0;
  virtual msg::EquipmentError RequestStop() = 0;
  virtual bool SafeToRelease() const = 0;
  virtual bool HasControlLease() const = 0;
  virtual void ForgetOperation(const std::string& operation_id) = 0;
  virtual msg::EquipmentError Deactivate() = 0;

 protected:
  msg::EquipmentError ValidateEnvelope(const msg::DeviceCommand& command) const;
};
}  // namespace apollo::drivers::equipment

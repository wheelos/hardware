// Copyright 2026 WheelOS. All Rights Reserved.
#include <memory>
#include <string>

#include "cyber/common/file.h"
#include "gtest/gtest.h"
#include "modules/drivers/equipment/equipment_runtime.h"
#include "tests/fake_stream.h"
#include "wheelos_msgs/equipment_msgs/capabilities/charging.pb.h"

namespace apollo::drivers::equipment::testing {
namespace {
EquipmentModuleConfig SimConfig(const std::string& type) {
  auto config = Config(type);
  config.clear_channels();
  auto* device = config.mutable_devices(0);
  device->set_model("sim");
  device->clear_bindings();
  device->clear_model_config();
  return config;
}
msg::OperationSnapshot Operation(EquipmentRuntime* runtime,
                                 const std::string& id) {
  msg::GetOperationRequest request;
  request.set_runtime_instance_id(
      runtime->ListDevices({}).runtime_instance_id());
  request.set_operation_id(id);
  return runtime->GetOperation(request).operation();
}
}  // namespace
TEST(SimulationTest, BinaryModelsCompleteThroughTheUnchangedOperationContract) {
  for (const std::string type : {"switch", "lift", "open_close"}) {
    EquipmentRuntime runtime(
        DeviceFactory(),
        [](const StreamConfig&) -> std::shared_ptr<hal::Stream> {
          ADD_FAILURE() << "Simulation must not create HAL streams";
          return nullptr;
        });
    ASSERT_TRUE(IsOk(runtime.Initialize(SimConfig(type))));
    ASSERT_TRUE(IsOk(runtime.Start()));
    ASSERT_TRUE(WaitUntil(
        [&] { return IsOk(runtime.GetState(StateRequest()).error()); }));
    const auto instance = runtime.ListDevices({}).runtime_instance_id();
    msg::ExecuteRequest request;
    if (type == "switch") {
      request = Request(instance, SwitchGoal());
    } else if (type == "lift") {
      msg::LiftGoal goal;
      goal.set_target(msg::LiftGoal::RAISED);
      request = Request(instance, goal);
    } else {
      msg::OpenCloseGoal goal;
      goal.set_target(msg::OpenCloseGoal::OPEN);
      request = Request(instance, goal);
    }
    const auto accepted = runtime.Execute(request);
    ASSERT_EQ(accepted.error().code(), msg::OK);
    ASSERT_TRUE(WaitUntil([&] {
      return Operation(&runtime, accepted.operation().operation_id()).state() ==
             msg::SUCCEEDED;
    }));
    EXPECT_EQ(runtime.ListDevices({}).devices(0).model(), "sim");
    EXPECT_EQ(runtime.ListDevices({}).channels_size(), 0);
    EXPECT_TRUE(IsOk(runtime.Shutdown()));
  }
}
TEST(SimulationTest, ChargingSessionSurvivesStartAndRequiresExplicitStop) {
  EquipmentRuntime runtime;
  ASSERT_TRUE(IsOk(runtime.Initialize(SimConfig("charger"))));
  ASSERT_TRUE(IsOk(runtime.Start()));
  ASSERT_TRUE(WaitUntil(
      [&] { return IsOk(runtime.GetState(StateRequest()).error()); }));
  const auto instance = runtime.ListDevices({}).runtime_instance_id();
  const auto start = runtime.Execute(
      Request(instance, msg::StartChargingGoal(), "start", "tool", "start"));
  ASSERT_TRUE(IsOk(start.error()));
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(&runtime, start.operation().operation_id()).state() ==
           msg::SUCCEEDED;
  }));
  EXPECT_TRUE(runtime.GetState(StateRequest()).state().blocked());
  msg::ChargingResult result;
  ASSERT_TRUE(Operation(&runtime, start.operation().operation_id())
                  .result()
                  .UnpackTo(&result));
  msg::StopChargingGoal goal;
  goal.set_session_id(result.session_id());
  const auto stop =
      runtime.Execute(Request(instance, goal, "stop", "tool", "stop"));
  ASSERT_TRUE(IsOk(stop.error()));
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(&runtime, stop.operation().operation_id()).state() ==
           msg::SUCCEEDED;
  }));
  EXPECT_FALSE(runtime.GetState(StateRequest()).state().blocked());
  EXPECT_TRUE(IsOk(runtime.Shutdown()));
}
TEST(SimulationTest, FrozenFeedbackCannotBecomeSuccessOrReleaseOwnership) {
  auto config = SimConfig("switch");
  SimulationConfig simulation;
  simulation.set_freeze_after_command(true);
  ASSERT_TRUE(
      config.mutable_devices(0)->mutable_model_config()->PackFrom(simulation));
  EquipmentRuntime runtime;
  ASSERT_TRUE(IsOk(runtime.Initialize(config)));
  ASSERT_TRUE(IsOk(runtime.Start()));
  ASSERT_TRUE(WaitUntil(
      [&] { return IsOk(runtime.GetState(StateRequest()).error()); }));
  auto request =
      Request(runtime.ListDevices({}).runtime_instance_id(), SwitchGoal());
  request.set_execution_timeout_ms(50);
  const auto accepted = runtime.Execute(request);
  ASSERT_TRUE(IsOk(accepted.error()));
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(&runtime, accepted.operation().operation_id()).state() ==
           msg::TIMED_OUT;
  }));
  EXPECT_TRUE(runtime.GetState(StateRequest()).state().blocked());
  EXPECT_TRUE(runtime.Shutdown().may_have_executed());
}
TEST(SimulationTest, ShippedConfigurationIsExplicitAndHasNoHardwareBindings) {
  EquipmentModuleConfig config;
  ASSERT_TRUE(
      cyber::common::GetProtoFromFile("conf/simulation.pb.txt", &config));
  EquipmentRuntime runtime;
  ASSERT_TRUE(IsOk(runtime.Initialize(config)));
  ASSERT_TRUE(IsOk(runtime.Start()));
  ASSERT_TRUE(WaitUntil([&] {
    auto query = StateRequest();
    for (const auto& device : config.devices()) {
      query.set_device_id(device.device_id());
      if (!IsOk(runtime.GetState(query).error())) {
        return false;
      }
    }
    return true;
  }));
  EXPECT_TRUE(IsOk(runtime.Shutdown()));
}
TEST(SimulationTest, InjectedDeviceFaultIsNotSuccessfulCompletion) {
  auto config = SimConfig("switch");
  SimulationConfig simulation;
  simulation.set_fault_on_command(true);
  ASSERT_TRUE(
      config.mutable_devices(0)->mutable_model_config()->PackFrom(simulation));
  EquipmentRuntime runtime;
  ASSERT_TRUE(IsOk(runtime.Initialize(config)));
  ASSERT_TRUE(IsOk(runtime.Start()));
  ASSERT_TRUE(WaitUntil(
      [&] { return IsOk(runtime.GetState(StateRequest()).error()); }));
  const auto accepted = runtime.Execute(
      Request(runtime.ListDevices({}).runtime_instance_id(), SwitchGoal()));
  ASSERT_TRUE(IsOk(accepted.error()));
  ASSERT_TRUE(WaitUntil([&] {
    return Operation(&runtime, accepted.operation().operation_id()).state() ==
           msg::FAILED;
  }));
  EXPECT_EQ(runtime.GetState(StateRequest()).error().code(), msg::DEVICE_FAULT);
  EXPECT_TRUE(runtime.Shutdown().may_have_executed());
}
}  // namespace apollo::drivers::equipment::testing

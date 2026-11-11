// Copyright 2026 WheelOS. All Rights Reserved.
#include "modules/drivers/equipment/equipment_service.h"

#include <unistd.h>

#include <cstdlib>
#include <memory>
#include <string>

#include "cyber/cyber.h"
#include "gtest/gtest.h"
#include "modules/drivers/equipment/clients/binary_client.h"
#include "modules/drivers/equipment/clients/charging_client.h"
#include "tests/fake_stream.h"

namespace apollo::drivers::equipment::testing {
TEST(EquipmentServiceTest, FourTypedActionsRoundTripThroughCyber) {
  const std::string root = "/equipment_test_" + std::to_string(getpid());
  auto config = Config("open_close");
  config.set_service_root(root);
  config.mutable_devices(0)->set_state_max_age_ms(5000);
  auto lift = Config("lift").devices(0);
  lift.set_state_max_age_ms(5000);
  lift.set_device_id("lift");
  lift.mutable_bindings(0)->set_channel_id("lift_bus");
  auto second_channel = config.channels(0);
  second_channel.set_channel_id("lift_bus");
  second_channel.mutable_stream()->mutable_can()->set_interface_name(
      "test_only_can_2");
  *config.add_channels() = second_channel;
  *config.add_devices() = lift;
  auto door_stream = std::make_shared<FakeStream>();
  auto lift_stream = std::make_shared<FakeStream>();
  auto runtime = std::make_shared<EquipmentRuntime>(
      DeviceFactory(), [door_stream, lift_stream](const StreamConfig& stream) {
        return stream.can().interface_name() == "test_only_can_2" ? lift_stream
                                                                  : door_stream;
      });
  door_stream->Feedback(0x200, 2);
  lift_stream->Feedback(0x200, 2);
  ASSERT_TRUE(IsOk(runtime->Initialize(config)));
  ASSERT_TRUE(IsOk(runtime->Start()));
  std::shared_ptr<cyber::Node> server(
      cyber::CreateNode("equipment_test_server"));
  std::shared_ptr<cyber::Node> node(cyber::CreateNode("equipment_test_client"));
  EquipmentService service;
  ASSERT_TRUE(IsOk(service.Start(server, runtime, root)));
  EquipmentClient client;
  ASSERT_TRUE(IsOk(client.Initialize(node, root)));
  BinaryClient binary(client);
  ASSERT_TRUE(client.WaitForServices(std::chrono::seconds(5)));
  msg::ListDevicesResponse inventory;
  ASSERT_TRUE(WaitUntil([&] {
    inventory = client.ListDevices({}, std::chrono::seconds(1));
    return IsOk(inventory.error());
  }));
  ASSERT_EQ(inventory.devices_size(), 2);
  const auto instance = inventory.runtime_instance_id();
  ASSERT_TRUE(WaitUntil([&] {
    return IsOk(
        client.GetState(StateRequest(5000), std::chrono::seconds(1)).error());
  }));

  ExecuteContext context;
  context.runtime_instance_id = instance;
  context.request_key.set_caller_id("rpc_test");
  context.execution_timeout_ms = 4000;
  context.rpc_timeout = std::chrono::seconds(1);
  for (int action = 0; action < 4; ++action) {
    context.request_key.set_request_id(std::to_string(action));
    msg::ExecuteResponse response;
    std::shared_ptr<FakeStream> stream;
    const auto expected_writes = action % 2 + 1;
    if (action == 0) {
      stream = door_stream;
      response = binary.Open({"tool", "main"}, context);
    } else if (action == 1) {
      stream = door_stream;
      response = binary.Close({"tool", "main"}, context);
    } else if (action == 2) {
      stream = lift_stream;
      response = binary.Raise({"lift", "main"}, context);
    } else {
      stream = lift_stream;
      response = binary.Lower({"lift", "main"}, context);
    }
    ASSERT_EQ(response.error().code(), msg::OK) << response.ShortDebugString();
    ASSERT_TRUE(WaitUntil([&] {
      return stream->WriteCount() == static_cast<size_t>(expected_writes);
    }));
    EXPECT_EQ(stream->LastCommand(), action % 2 == 0 ? 1 : 2);
    stream->Feedback(0x200, action % 2 == 0 ? 1 : 2);
    msg::GetOperationRequest query;
    query.set_runtime_instance_id(instance);
    query.set_operation_id(response.operation().operation_id());
    ASSERT_TRUE(WaitUntil([&] {
      const auto operation =
          client.GetOperation(query, std::chrono::seconds(1));
      return IsOk(operation.error()) &&
             operation.operation().state() == msg::SUCCEEDED;
    }));
  }
  service.Stop();
  EXPECT_TRUE(IsOk(runtime->Shutdown()));
}

TEST(EquipmentServiceTest, MissingServiceAndInvalidTimeoutAreExplicit) {
  EquipmentClient uninitialized;
  const auto response = uninitialized.Execute({}, std::chrono::seconds(1));
  EXPECT_EQ(response.error().code(), msg::RPC_UNAVAILABLE);
  EXPECT_EQ(uninitialized.Execute({}, std::chrono::seconds(0)).error().code(),
            msg::INVALID_ARGUMENT);
  BinaryClient binary(uninitialized);
  ChargingClient charging(uninitialized);
  ExecuteContext context;
  context.rpc_timeout = std::chrono::seconds(1);
  EXPECT_EQ(binary.SetEnabled({"tool", "main"}, context, true).error().code(),
            msg::RPC_UNAVAILABLE);
  EXPECT_EQ(charging.StartCharging({"tool", "main"}, context).error().code(),
            msg::RPC_UNAVAILABLE);
}

TEST(EquipmentServiceTest, LostExecuteReplyDoesNotResendPhysicalCommand) {
  const auto root = "/equipment_lost_" + std::to_string(getpid());
  auto stream = std::make_shared<FakeStream>();
  auto runtime = std::make_shared<EquipmentRuntime>(
      DeviceFactory(), [stream](const StreamConfig&) { return stream; });
  auto config = Config("open_close");
  config.mutable_devices(0)->set_state_max_age_ms(5000);
  ASSERT_TRUE(IsOk(runtime->Initialize(config)));
  stream->Feedback(0x200, 2);
  ASSERT_TRUE(IsOk(runtime->Start()));
  ASSERT_TRUE(WaitUntil(
      [&] { return IsOk(runtime->GetState(StateRequest()).error()); }));
  std::shared_ptr<cyber::Node> server(
      cyber::CreateNode("equipment_lost_server"));
  std::shared_ptr<cyber::Node> node(cyber::CreateNode("equipment_lost_client"));
  auto list =
      server->CreateService<msg::ListDevicesRequest, msg::ListDevicesResponse>(
          root + "/list", [runtime](const auto& request, auto& response) {
            *response = runtime->ListDevices(*request);
          });
  auto execute =
      server->CreateService<msg::ExecuteRequest, msg::ExecuteResponse>(
          root + "/execute", [runtime](const auto& request, auto& response) {
            *response = runtime->Execute(*request);
            std::this_thread::sleep_for(std::chrono::milliseconds(1200));
          });
  auto query = server->CreateService<msg::GetOperationRequest,
                                     msg::GetOperationResponse>(
      root + "/get_operation", [runtime](const auto& request, auto& response) {
        *response = runtime->GetOperation(*request);
      });
  auto state =
      server->CreateService<msg::GetStateRequest, msg::GetStateResponse>(
          root + "/get_state", [runtime](const auto& request, auto& response) {
            *response = runtime->GetState(*request);
          });
  auto cancel = server->CreateService<msg::CancelOperationRequest,
                                      msg::CancelOperationResponse>(
      root + "/cancel", [runtime](const auto& request, auto& response) {
        *response = runtime->CancelOperation(*request);
      });
  ASSERT_TRUE(list && execute && query && state && cancel);
  EquipmentClient client;
  ASSERT_TRUE(IsOk(client.Initialize(node, root)));
  BinaryClient binary(client);
  ASSERT_TRUE(client.WaitForServices(std::chrono::seconds(5)));
  msg::ListDevicesResponse inventory;
  ASSERT_TRUE(WaitUntil([&] {
    inventory = client.ListDevices({}, std::chrono::seconds(1));
    return IsOk(inventory.error());
  }));
  ExecuteContext context;
  context.runtime_instance_id = inventory.runtime_instance_id();
  context.request_key.set_caller_id("lost_reply");
  context.request_key.set_request_id("one-physical-command");
  context.execution_timeout_ms = 4000;
  context.rpc_timeout = std::chrono::seconds(1);
  const auto response = binary.Open({"tool", "main"}, context);
  EXPECT_EQ(response.error().code(), msg::RPC_TIMEOUT);
  EXPECT_TRUE(response.error().may_have_executed());
  msg::GetOperationRequest lookup;
  lookup.set_runtime_instance_id(context.runtime_instance_id);
  *lookup.mutable_request_key() = context.request_key;
  const auto operation = client.GetOperation(lookup, std::chrono::seconds(1));
  ASSERT_EQ(operation.error().code(), msg::OK);
  EXPECT_EQ(stream->WriteCount(), 1);
  stream->Feedback(0x200, 1);
  ASSERT_TRUE(WaitUntil([&] {
    return runtime->GetOperation(lookup).operation().state() == msg::SUCCEEDED;
  }));
  EXPECT_EQ(stream->WriteCount(), 1);
  EXPECT_TRUE(IsOk(runtime->Shutdown()));
}
}  // namespace apollo::drivers::equipment::testing

int main(int argc, char** argv) {
  setenv("CYBER_IP", "127.0.0.1", 1);
  const auto domain = std::to_string(180 + getpid() % 40);
  setenv("CYBER_DOMAIN_ID", domain.c_str(), 1);
  setenv("GLOG_logtostderr", "1", 1);
  ::testing::InitGoogleTest(&argc, argv);
  if (!apollo::cyber::Init(argv[0])) {
    return 1;
  }
  const int result = RUN_ALL_TESTS();
  apollo::cyber::Clear();
  return result;
}

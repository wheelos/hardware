load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

CANBUS_COPTS = ["-DMODULE_NAME=\\\"canbus\\\""]

cc_library(
    name = "transit_vehicle_factory",
    srcs = ["transit_vehicle_factory.cc"],
    hdrs = ["transit_vehicle_factory.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/transit",
    strip_include_prefix = "/brands/transit",
    deps = [
        ":transit_controller",
        ":transit_message_manager",
        "//brands:abstract_vehicle_factory",
        "//brands:vehicle_factory_core",
    ],
    alwayslink = True,
)

cc_library(
    name = "transit_message_manager",
    srcs = ["transit_message_manager.cc"],
    hdrs = ["transit_message_manager.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/transit",
    strip_include_prefix = "/brands/transit",
    deps = [
        "//brands:chassis_extension_tools",
        "//brands/transit/protocol:canbus_transit_protocol",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_library(
    name = "transit_controller",
    srcs = ["transit_controller.cc"],
    hdrs = ["transit_controller.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/transit",
    strip_include_prefix = "/brands/transit",
    deps = [
        ":transit_message_manager",
        "//brands:vehicle_controller_base",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_common//modules/common/kv_db",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:vehicle_signal_cc_proto",
    ],
)

cc_test(
    name = "transit_vehicle_factory_test",
    size = "small",
    srcs = ["transit_vehicle_factory_test.cc"],
    deps = [
        ":transit_vehicle_factory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "transit_controller_test",
    size = "small",
    srcs = ["transit_controller_test.cc"],
    data = ["//:test_data"],
    deps = [
        ":transit_controller",
        "@com_google_googletest//:gtest_main",
        "@wheelos_common//modules/common/util",
    ],
)

cpplint()

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

CANBUS_COPTS = ["-DMODULE_NAME=\\\"canbus\\\""]

cc_library(
    name = "lexus_vehicle_factory",
    srcs = ["lexus_vehicle_factory.cc"],
    hdrs = ["lexus_vehicle_factory.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/lexus",
    strip_include_prefix = "/brands/lexus",
    deps = [
        ":lexus_controller",
        ":lexus_message_manager",
        "//brands:abstract_vehicle_factory",
        "//brands:vehicle_factory_core",
        "//proto:canbus_conf_cc_proto",
        "//proto:vehicle_parameter_cc_proto",
    ],
    alwayslink = True,
)

cc_library(
    name = "lexus_message_manager",
    srcs = ["lexus_message_manager.cc"],
    hdrs = ["lexus_message_manager.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/lexus",
    strip_include_prefix = "/brands/lexus",
    deps = [
        "//brands/lexus/protocol:canbus_lexus_protocol",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_library(
    name = "lexus_controller",
    srcs = ["lexus_controller.cc"],
    hdrs = ["lexus_controller.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/lexus",
    strip_include_prefix = "/brands/lexus",
    deps = [
        ":lexus_message_manager",
        "//brands:vehicle_controller_base",
        "//brands/lexus/protocol:canbus_lexus_protocol",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_test(
    name = "lexus_controller_test",
    size = "small",
    srcs = ["lexus_controller_test.cc"],
    data = ["//:test_data"],
    deps = [
        "//brands/lexus:lexus_controller",
        "@com_google_googletest//:gtest_main",
    ],
)

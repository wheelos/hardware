load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

CANBUS_COPTS = ["-DMODULE_NAME=\\\"canbus\\\""]

cc_library(
    name = "ch_vehicle_factory",
    srcs = ["ch_vehicle_factory.cc"],
    hdrs = ["ch_vehicle_factory.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/ch",
    strip_include_prefix = "/brands/ch",
    deps = [
        ":ch_controller",
        ":ch_message_manager",
        "//brands:abstract_vehicle_factory",
        "//brands:vehicle_factory_core",
    ],
    alwayslink = True,
)

cc_library(
    name = "ch_message_manager",
    srcs = ["ch_message_manager.cc"],
    hdrs = ["ch_message_manager.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/ch",
    strip_include_prefix = "/brands/ch",
    deps = [
        "//brands/ch/protocol:canbus_ch_protocol",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_library(
    name = "ch_controller",
    srcs = ["ch_controller.cc"],
    hdrs = ["ch_controller.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/ch",
    strip_include_prefix = "/brands/ch",
    deps = [
        ":ch_message_manager",
        "//brands:vehicle_controller_base",
        "//brands/ch/proto:ch_cc_proto",
        "//brands/ch/protocol:canbus_ch_protocol",
        "//proto:canbus_conf_cc_proto",
        "//proto:vehicle_parameter_cc_proto",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:error_code_cc_proto",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_cc_proto",
        "@wheelos_msgs//wheelos_msgs/control_msgs:control_cmd_cc_proto",
    ],
)

cc_test(
    name = "ch_controller_test",
    size = "small",
    srcs = ["ch_controller_test.cc"],
    data = ["//:test_data"],
    deps = [
        ":ch_controller",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "ch_message_manager_test",
    size = "small",
    srcs = ["ch_message_manager_test.cc"],
    deps = [
        "//brands/ch:ch_message_manager",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "ch_vehicle_factory_test",
    size = "small",
    srcs = ["ch_vehicle_factory_test.cc"],
    deps = [
        ":ch_vehicle_factory",
        "@com_google_googletest//:gtest_main",
    ],
)

load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

CANBUS_COPTS = ["-DMODULE_NAME=\\\"canbus\\\""]

cc_library(
    name = "devkit_vehicle_factory",
    srcs = [
        "devkit_vehicle_factory.cc",
    ],
    hdrs = [
        "devkit_vehicle_factory.h",
    ],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/devkit",
    strip_include_prefix = "/brands/devkit",
    deps = [
        ":devkit_controller",
        ":devkit_message_manager",
        "//brands:abstract_vehicle_factory",
        "//brands:vehicle_factory_core",
    ],
    alwayslink = True,
)

cc_test(
    name = "devkit_vehicle_factory_test",
    size = "small",
    srcs = ["devkit_vehicle_factory_test.cc"],
    deps = [
        ":devkit_vehicle_factory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "devkit_message_manager",
    srcs = ["devkit_message_manager.cc"],
    hdrs = ["devkit_message_manager.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/devkit",
    strip_include_prefix = "/brands/devkit",
    deps = [
        "//brands/devkit/protocol:canbus_devkit_protocol",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_test(
    name = "devkit_message_manager_test",
    size = "small",
    srcs = ["devkit_message_manager_test.cc"],
    deps = [
        "//brands/devkit:devkit_message_manager",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "devkit_controller",
    srcs = ["devkit_controller.cc"],
    hdrs = ["devkit_controller.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/devkit",
    strip_include_prefix = "/brands/devkit",
    deps = [
        ":devkit_message_manager",
        "//brands:vehicle_controller_base",
        "//brands/devkit/proto:devkit_cc_proto",
        "//brands/devkit/protocol:canbus_devkit_protocol",
        "//common:canbus_common",
        "//proto:canbus_conf_cc_proto",
        "//proto:vehicle_parameter_cc_proto",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:error_code_cc_proto",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_cc_proto",
        "@wheelos_msgs//wheelos_msgs/control_msgs:control_cmd_cc_proto",
    ],
)

cc_test(
    name = "devkit_controller_test",
    size = "small",
    srcs = ["devkit_controller_test.cc"],
    data = ["//:test_data"],
    deps = [
        ":devkit_controller",
        "@com_google_googletest//:gtest_main",
    ],
)

cpplint()

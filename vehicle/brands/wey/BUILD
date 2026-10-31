load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "wey_vehicle_factory",
    srcs = ["wey_vehicle_factory.cc"],
    hdrs = ["wey_vehicle_factory.h"],
    include_prefix = "modules/canbus/vehicle/wey",
    strip_include_prefix = "/brands/wey",
    deps = [
        ":wey_controller",
        ":wey_message_manager",
        "//brands:abstract_vehicle_factory",
        "//brands:vehicle_factory_core",
    ],
    alwayslink = True,
)

cc_test(
    name = "wey_vehicle_factory_test",
    size = "small",
    srcs = ["wey_vehicle_factory_test.cc"],
    deps = [
        ":wey_vehicle_factory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "wey_message_manager",
    srcs = ["wey_message_manager.cc"],
    hdrs = ["wey_message_manager.h"],
    include_prefix = "modules/canbus/vehicle/wey",
    strip_include_prefix = "/brands/wey",
    deps = [
        "//brands/wey/protocol:canbus_wey_protocol",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_test(
    name = "wey_message_manager_test",
    size = "small",
    srcs = ["wey_message_manager_test.cc"],
    deps = [
        ":wey_message_manager",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "wey_controller",
    srcs = ["wey_controller.cc"],
    hdrs = ["wey_controller.h"],
    include_prefix = "modules/canbus/vehicle/wey",
    strip_include_prefix = "/brands/wey",
    deps = [
        ":wey_message_manager",
        "//brands:vehicle_controller_base",
        "//brands/wey/protocol:canbus_wey_protocol",
        "//proto:canbus_conf_cc_proto",
        "//proto:vehicle_parameter_cc_proto",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:error_code_cc_proto",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_cc_proto",
        "@wheelos_msgs//wheelos_msgs/control_msgs:control_cmd_cc_proto",
    ],
)

cc_test(
    name = "wey_controller_test",
    size = "small",
    srcs = ["wey_controller_test.cc"],
    data = ["//:test_data"],
    deps = [
        ":wey_controller",
        "@com_google_googletest//:gtest_main",
        "@wheelos_common//modules/common/util",
    ],
)

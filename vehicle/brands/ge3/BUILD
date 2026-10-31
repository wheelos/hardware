load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "ge3_vehicle_factory",
    srcs = ["ge3_vehicle_factory.cc"],
    hdrs = ["ge3_vehicle_factory.h"],
    include_prefix = "modules/canbus/vehicle/ge3",
    strip_include_prefix = "/brands/ge3",
    deps = [
        ":ge3_controller",
        ":ge3_message_manager",
        "//brands:abstract_vehicle_factory",
        "//brands:vehicle_factory_core",
        "//proto:vehicle_parameter_cc_proto",
    ],
    alwayslink = True,
)

cc_test(
    name = "ge3_vehicle_factory_test",
    size = "small",
    srcs = ["ge3_vehicle_factory_test.cc"],
    deps = [
        ":ge3_vehicle_factory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "ge3_message_manager",
    srcs = ["ge3_message_manager.cc"],
    hdrs = ["ge3_message_manager.h"],
    include_prefix = "modules/canbus/vehicle/ge3",
    strip_include_prefix = "/brands/ge3",
    deps = [
        "//brands/ge3/protocol:canbus_ge3_protocol_pc_bcm_201",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_test(
    name = "ge3_message_manager_test",
    size = "small",
    srcs = ["ge3_message_manager_test.cc"],
    deps = [
        ":ge3_message_manager",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "ge3_controller",
    srcs = ["ge3_controller.cc"],
    hdrs = ["ge3_controller.h"],
    include_prefix = "modules/canbus/vehicle/ge3",
    strip_include_prefix = "/brands/ge3",
    deps = [
        ":ge3_message_manager",
        "//brands:vehicle_controller_base",
        "//brands/ge3/protocol:canbus_ge3_protocol_pc_bcm_201",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_test(
    name = "ge3_controller_test",
    size = "small",
    srcs = ["ge3_controller_test.cc"],
    data = ["//:test_data"],
    deps = [
        ":ge3_controller",
        "@com_google_googletest//:gtest_main",
        "@wheelos_common//modules/common/util",
    ],
)

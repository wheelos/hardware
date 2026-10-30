load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

CANBUS_COPTS = ["-DMODULE_NAME=\\\"canbus\\\""]

cc_library(
    name = "lincoln_vehicle_factory",
    srcs = ["lincoln_vehicle_factory.cc"],
    hdrs = ["lincoln_vehicle_factory.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/lincoln",
    strip_include_prefix = "/brands/lincoln",
    deps = [
        ":lincoln_controller",
        ":lincoln_message_manager",
        "//brands:abstract_vehicle_factory",
        "//brands:vehicle_factory_core",
    ],
    alwayslink = True,
)

cc_library(
    name = "lincoln_message_manager",
    srcs = ["lincoln_message_manager.cc"],
    hdrs = ["lincoln_message_manager.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/lincoln",
    strip_include_prefix = "/brands/lincoln",
    deps = [
        "//brands/lincoln/protocol:canbus_lincoln_protocol",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_library(
    name = "lincoln_controller",
    srcs = ["lincoln_controller.cc"],
    hdrs = ["lincoln_controller.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/lincoln",
    strip_include_prefix = "/brands/lincoln",
    deps = [
        ":lincoln_message_manager",
        "//brands:vehicle_controller_base",
        "//brands/lincoln/protocol:canbus_lincoln_protocol",
        "@com_google_googletest//:gtest",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_common//modules/common/kv_db",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_test(
    name = "lincoln_controller_test",
    size = "small",
    srcs = ["lincoln_controller_test.cc"],
    data = ["//:test_data"],
    deps = [
        "//brands/lincoln:lincoln_controller",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "lincoln_message_manager_test",
    size = "small",
    srcs = ["lincoln_message_manager_test.cc"],
    deps = [
        "//brands/lincoln:lincoln_message_manager",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "lincoln_vehicle_factory_test",
    size = "small",
    srcs = ["lincoln_vehicle_factory_test.cc"],
    deps = [
        ":lincoln_vehicle_factory",
        "@com_google_googletest//:gtest_main",
    ],
)

cpplint()

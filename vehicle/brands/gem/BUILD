load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

CANBUS_COPTS = ["-DMODULE_NAME=\\\"canbus\\\""]

cc_library(
    name = "gem_vehicle_factory",
    srcs = ["gem_vehicle_factory.cc"],
    hdrs = ["gem_vehicle_factory.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/gem",
    strip_include_prefix = "/brands/gem",
    deps = [
        ":gem_controller",
        ":gem_message_manager",
        "//brands:abstract_vehicle_factory",
        "//brands:vehicle_factory_core",
        "//proto:canbus_conf_cc_proto",
        "//proto:vehicle_parameter_cc_proto",
    ],
    alwayslink = True,
)

cc_test(
    name = "gem_vehicle_factory_test",
    size = "small",
    srcs = ["gem_vehicle_factory_test.cc"],
    deps = [
        ":gem_vehicle_factory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "gem_message_manager",
    srcs = ["gem_message_manager.cc"],
    hdrs = ["gem_message_manager.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/gem",
    strip_include_prefix = "/brands/gem",
    deps = [
        "//brands/gem/proto:gem_cc_proto",
        "//brands/gem/protocol:canbus_gem_protocol",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_test(
    name = "gem_message_manager_test",
    size = "small",
    srcs = ["gem_message_manager_test.cc"],
    deps = [
        ":gem_message_manager",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "gem_controller",
    srcs = ["gem_controller.cc"],
    hdrs = ["gem_controller.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/gem",
    strip_include_prefix = "/brands/gem",
    deps = [
        ":gem_message_manager",
        "//brands:vehicle_controller_base",
        "//brands/gem/proto:gem_cc_proto",
        "//brands/gem/protocol:canbus_gem_protocol",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_test(
    name = "gem_controller_test",
    size = "small",
    srcs = ["gem_controller_test.cc"],
    data = ["//:test_data"],
    deps = [
        ":gem_controller",
        "@com_google_googletest//:gtest_main",
        "@wheelos_common//modules/common/util",
    ],
)

cpplint()

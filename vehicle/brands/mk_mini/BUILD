load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mk_mini_vehicle_factory",
    srcs = [
        "mk_mini_vehicle_factory.cc",
    ],
    hdrs = [
        "mk_mini_vehicle_factory.h",
    ],
    include_prefix = "modules/canbus/vehicle/mk_mini",
    strip_include_prefix = "/brands/mk_mini",
    deps = [
        ":mk_mini_controller",
        ":mk_mini_message_manager",
        "//brands:abstract_vehicle_factory",
        "//brands:vehicle_factory_core",
    ],
    alwayslink = True,
)

cc_library(
    name = "mk_mini_message_manager",
    srcs = [
        "mk_mini_message_manager.cc",
    ],
    hdrs = [
        "mk_mini_message_manager.h",
    ],
    include_prefix = "modules/canbus/vehicle/mk_mini",
    strip_include_prefix = "/brands/mk_mini",
    deps = [
        "//brands/mk_mini/protocol:canbus_mk_mini_protocol",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_library(
    name = "mk_mini_controller",
    srcs = [
        "mk_mini_controller.cc",
    ],
    hdrs = [
        "mk_mini_controller.h",
    ],
    include_prefix = "modules/canbus/vehicle/mk_mini",
    strip_include_prefix = "/brands/mk_mini",
    deps = [
        ":mk_mini_message_manager",
        "//brands:vehicle_controller_base",
        "//brands/mk_mini/protocol:canbus_mk_mini_protocol",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

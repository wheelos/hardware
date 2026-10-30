load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "yunle_vehicle_factory",
    srcs = [
        "yunle_vehicle_factory.cc",
    ],
    hdrs = [
        "yunle_vehicle_factory.h",
    ],
    include_prefix = "modules/canbus/vehicle/yunle",
    strip_include_prefix = "/brands/yunle",
    deps = [
        ":yunle_controller",
        ":yunle_message_manager",
        "//brands:abstract_vehicle_factory",
        "//brands:vehicle_factory_core",
    ],
    alwayslink = True,
)

cc_library(
    name = "yunle_message_manager",
    srcs = [
        "yunle_message_manager.cc",
    ],
    hdrs = [
        "yunle_message_manager.h",
    ],
    include_prefix = "modules/canbus/vehicle/yunle",
    strip_include_prefix = "/brands/yunle",
    deps = [
        "//brands/yunle/protocol:canbus_yunle_protocol",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_library(
    name = "yunle_controller",
    srcs = [
        "yunle_controller.cc",
    ],
    hdrs = [
        "yunle_controller.h",
    ],
    include_prefix = "modules/canbus/vehicle/yunle",
    strip_include_prefix = "/brands/yunle",
    deps = [
        ":yunle_message_manager",
        "//brands:vehicle_controller_base",
        "//brands/yunle/protocol:canbus_yunle_protocol",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cpplint()

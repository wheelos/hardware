load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

CANBUS_COPTS = ["-DMODULE_NAME=\\\"canbus\\\""]

cc_library(
    name = "neolix_edu_vehicle_factory",
    srcs = [
        "neolix_edu_vehicle_factory.cc",
    ],
    hdrs = [
        "neolix_edu_vehicle_factory.h",
    ],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/neolix_edu",
    strip_include_prefix = "/brands/neolix_edu",
    deps = [
        ":neolix_edu_controller",
        ":neolix_edu_message_manager",
        "//brands:abstract_vehicle_factory",
        "//brands:vehicle_factory_core",
    ],
    alwayslink = True,
)

cc_library(
    name = "neolix_edu_message_manager",
    srcs = [
        "neolix_edu_message_manager.cc",
    ],
    hdrs = [
        "neolix_edu_message_manager.h",
    ],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/neolix_edu",
    strip_include_prefix = "/brands/neolix_edu",
    deps = [
        "//brands/neolix_edu/protocol:canbus_neolix_edu_protocol",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_library(
    name = "neolix_edu_controller",
    srcs = [
        "neolix_edu_controller.cc",
    ],
    hdrs = [
        "neolix_edu_controller.h",
    ],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle/neolix_edu",
    strip_include_prefix = "/brands/neolix_edu",
    deps = [
        ":neolix_edu_message_manager",
        "//brands:vehicle_controller_base",
        "//brands/neolix_edu/protocol:canbus_neolix_edu_protocol",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_test(
    name = "neolix_edu_controller_test",
    size = "small",
    srcs = ["neolix_edu_controller_test.cc"],
    data = ["//:test_data"],
    deps = [
        ":neolix_edu_controller",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "neolix_edu_message_manager_test",
    size = "small",
    srcs = ["neolix_edu_message_manager_test.cc"],
    deps = [
        "//brands/neolix_edu:neolix_edu_message_manager",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "neolix_edu_vehicle_factory_test",
    size = "small",
    srcs = ["neolix_edu_vehicle_factory_test.cc"],
    deps = [
        ":neolix_edu_vehicle_factory",
        "@com_google_googletest//:gtest_main",
    ],
)

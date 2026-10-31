load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "zhongyun_vehicle_factory",
    srcs = ["zhongyun_vehicle_factory.cc"],
    hdrs = ["zhongyun_vehicle_factory.h"],
    include_prefix = "modules/canbus/vehicle/zhongyun",
    strip_include_prefix = "/brands/zhongyun",
    deps = [
        ":zhongyun_controller",
        ":zhongyun_message_manager",
        "//brands:abstract_vehicle_factory",
        "//brands:vehicle_factory_core",
    ],
    alwayslink = True,
)

cc_library(
    name = "zhongyun_message_manager",
    srcs = ["zhongyun_message_manager.cc"],
    hdrs = ["zhongyun_message_manager.h"],
    include_prefix = "modules/canbus/vehicle/zhongyun",
    strip_include_prefix = "/brands/zhongyun",
    deps = [
        "//brands:chassis_extension_tools",
        "//brands/zhongyun/protocol:canbus_zhongyun_protocol",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_library(
    name = "zhongyun_controller",
    srcs = ["zhongyun_controller.cc"],
    hdrs = ["zhongyun_controller.h"],
    include_prefix = "modules/canbus/vehicle/zhongyun",
    strip_include_prefix = "/brands/zhongyun",
    deps = [
        ":zhongyun_message_manager",
        "//brands:chassis_extension_tools",
        "//brands:vehicle_controller_base",
        "//brands/zhongyun/protocol:canbus_zhongyun_protocol",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_test(
    name = "zhongyun_message_manager_test",
    size = "small",
    srcs = ["zhongyun_message_manager_test.cc"],
    data = ["//:test_data"],
    deps = [
        ":zhongyun_message_manager",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "zhongyun_vehicle_factory_test",
    size = "small",
    srcs = ["zhongyun_vehicle_factory_test.cc"],
    data = ["//:test_data"],
    deps = [
        ":zhongyun_vehicle_factory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "zhongyun_controller_test",
    size = "small",
    srcs = ["zhongyun_controller_test.cc"],
    data = ["//:test_data"],
    deps = [
        ":zhongyun_controller",
        "@com_google_googletest//:gtest_main",
        "@wheelos_common//modules/common/util",
    ],
)

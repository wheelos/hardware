load("@hardware.build//tools/hardware:defs.bzl", "VEHICLES", "vehicle_selection")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

exports_files(["chassis_extension_tools.h"])

CANBUS_COPTS = ["-DMODULE_NAME=\\\"canbus\\\""]

cc_library(
    name = "chassis_extension_tools",
    hdrs = ["chassis_extension_tools.h"],
    include_prefix = "modules/canbus/vehicle",
    strip_include_prefix = "/brands",
    deps = [
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_library(
    name = "vehicle_controller_base",
    srcs = ["vehicle_controller.cc"],
    hdrs = ["vehicle_controller.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle",
    strip_include_prefix = "/brands",
    deps = [
        "//proto:canbus_conf_cc_proto",
        "//proto:vehicle_parameter_cc_proto",
        "@hardware.canbus//can_client",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_common//modules/common/configs:vehicle_config_helper",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:error_code_cc_proto",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
        "@wheelos_msgs//wheelos_msgs/control_msgs:control_cmd_cc_proto",
    ],
)

cc_library(
    name = "abstract_vehicle_factory",
    srcs = ["abstract_vehicle_factory.cc"],
    hdrs = ["abstract_vehicle_factory.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle",
    strip_include_prefix = "/brands",
    deps = [
        "//brands:vehicle_controller_base",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_common//modules/common/util:registry",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_library(
    name = "vehicle_factory_core",
    srcs = ["vehicle_factory.cc"],
    hdrs = ["vehicle_factory.h"],
    copts = CANBUS_COPTS,
    include_prefix = "modules/canbus/vehicle",
    strip_include_prefix = "/brands",
    deps = [
        ":abstract_vehicle_factory",
        "@wheelos_common//modules/common/util:registry",
    ],
)

[vehicle_selection(name = "with_" + brand) for brand in VEHICLES]

cc_library(
    name = "vehicle_factory_plugins_all",
    include_prefix = "modules/canbus/vehicle",
    strip_include_prefix = "/brands",
    deps = select({
        ":with_ch": ["//brands/ch:ch_vehicle_factory"],
        ":with_devkit": ["//brands/devkit:devkit_vehicle_factory"],
        ":with_ge3": ["//brands/ge3:ge3_vehicle_factory"],
        ":with_gem": ["//brands/gem:gem_vehicle_factory"],
        ":with_lexus": ["//brands/lexus:lexus_vehicle_factory"],
        ":with_lincoln": ["//brands/lincoln:lincoln_vehicle_factory"],
        ":with_mk_mini": ["//brands/mk_mini:mk_mini_vehicle_factory"],
        ":with_neolix_edu": ["//brands/neolix_edu:neolix_edu_vehicle_factory"],
        ":with_transit": ["//brands/transit:transit_vehicle_factory"],
        ":with_wey": ["//brands/wey:wey_vehicle_factory"],
        ":with_yunle": ["//brands/yunle:yunle_vehicle_factory"],
        ":with_zhongyun": ["//brands/zhongyun:zhongyun_vehicle_factory"],
        "//conditions:default": [],
    }),
    alwayslink = True,
)

cc_library(
    name = "vehicle_factory",
    include_prefix = "modules/canbus/vehicle",
    strip_include_prefix = "/brands",
    deps = [
        ":vehicle_factory_core",
        ":vehicle_factory_plugins_all",
    ],
)

cc_test(
    name = "vehicle_selection_test",
    size = "small",
    srcs = ["vehicle_selection_test.cc"],
    target_compatible_with = select({
        ":with_devkit": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [
        ":vehicle_factory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "vehicle_factory_test",
    size = "small",
    srcs = ["vehicle_factory_test.cc"],
    deps = [
        ":vehicle_factory",
        "//brands/ch:ch_vehicle_factory",
        "//brands/devkit:devkit_vehicle_factory",
        "//brands/ge3:ge3_vehicle_factory",
        "//brands/gem:gem_vehicle_factory",
        "//brands/lincoln:lincoln_vehicle_factory",
        "//brands/neolix_edu:neolix_edu_vehicle_factory",
        "//brands/wey:wey_vehicle_factory",
        "//brands/zhongyun:zhongyun_vehicle_factory",
        "@com_google_googletest//:gtest_main",
    ],
)

load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "conf/*.conf",
        "dag/*.dag",
        "launch/*.launch",
    ]),
)

cc_binary(
    name = "libyg_radar_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [
        ":yg_radar_component_lib",
    ],
)

cc_library(
    name = "yg_radar_component_lib",
    srcs = [
        "yg_radar_component.cc",
        "yg_radar_message_manager.cc",
    ],
    hdrs = [
        "yg_radar_component.h",
        "yg_radar_message_manager.h",
    ],
    include_prefix = "modules/drivers/radar/yg_radar",
    strip_include_prefix = "/yg_radar",
    deps = [
        "//yg_radar/proto:yg_radar_cc_proto",
        "//yg_radar/proto:yg_radar_config_cc_proto",
        "//yg_radar/protocol:canbus_yg_radar_protocol",
        "@core//cyber",
        "@hardware.canbus//can_client",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_receiver",
        "@hardware.canbus//can_comm:message_manager_base",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/util:message_util",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:radar_cc_proto",
    ],
    alwayslink = True,
)

cpplint()

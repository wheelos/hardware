load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "dag/*.dag",
        "launch/*.launch",
    ]),
)

cc_binary(
    name = "libudas_ultrasonic_radar_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [
        ":udas_ultrasonic_radar_component_lib",
    ],
)

cc_library(
    name = "udas_ultrasonic_radar_component_lib",
    srcs = [
        "udas_ultrasonic_radar_component.cc",
        "udas_ultrasonic_radar_message_manager.cc",
    ],
    hdrs = [
        "udas_ultrasonic_radar_component.h",
        "udas_ultrasonic_radar_message_manager.h",
    ],
    include_prefix = "modules/drivers/radar/udas_ultrasonic_radar",
    strip_include_prefix = "/udas_ultrasonic_radar",
    deps = [
        "//udas_ultrasonic_radar/proto:udas_ultrasonic_radar_cc_proto",
        "//udas_ultrasonic_radar/proto:ultrasonic_radar_config_cc_proto",
        "//udas_ultrasonic_radar/protocol:canbus_udas_ultrasonic_radar_protocol",
        "@core//cyber",
        "@hardware.canbus//can_client",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_receiver",
        "@hardware.canbus//can_comm:message_manager_base",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/util:message_util",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:ultrasonic_radar_cc_proto",
    ],
    alwayslink = True,
)

cpplint()

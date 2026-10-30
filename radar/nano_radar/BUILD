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

cc_library(
    name = "nano_radar_message_manager",
    srcs = ["nano_radar_message_manager.cc"],
    hdrs = ["nano_radar_message_manager.h"],
    include_prefix = "modules/drivers/radar/nano_radar",
    strip_include_prefix = "/nano_radar",
    deps = [
        "//nano_radar/protocol:drivers_nano_radar_protocol",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@wheelos_common//modules/common/util:message_util",
    ],
)

cc_library(
    name = "nano_radar_lib",
    srcs = ["nano_radar_canbus_component.cc"],
    hdrs = ["nano_radar_canbus_component.h"],
    copts = ['-DMODULE_NAME=\\"nano_radar\\"'],
    include_prefix = "modules/drivers/radar/nano_radar",
    strip_include_prefix = "/nano_radar",
    deps = [
        ":nano_radar_message_manager",
        "//nano_radar/protocol:drivers_nano_radar_protocol",
        "@core//cyber",
        "@eigen",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_receiver",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//proto:sensor_canbus_conf_cc_proto",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/monitor_log",
    ],
    alwayslink = True,
)

cc_binary(
    name = "libnano_radar.so",
    linkshared = True,
    linkstatic = False,
    deps = [":nano_radar_lib"],
)

cpplint()

load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@hardware.build//tools/install:install.bzl", "install")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

install(
    name = "install",
    data = [
        ":runtime_data",
    ],
    data_dest = "drivers/addition_data/conti_radar",
    library_dest = "drivers/lib/conti_radar",
    targets = [
        ":libconti_radar.so",
    ],
)

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "dag/*.dag",
        "launch/*.launch",
    ]),
)

cc_library(
    name = "conti_radar_message_manager",
    srcs = ["conti_radar_message_manager.cc"],
    hdrs = ["conti_radar_message_manager.h"],
    include_prefix = "modules/drivers/radar/conti_radar",
    strip_include_prefix = "/conti_radar",
    deps = [
        "//conti_radar/protocol:drivers_conti_radar_protocol",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@wheelos_common//modules/common/util:util_tool",
    ],
)

cc_library(
    name = "conti_radar_canbus_lib",
    srcs = ["conti_radar_canbus_component.cc"],
    hdrs = ["conti_radar_canbus_component.h"],
    copts = ['-DMODULE_NAME=\\"conti_radar\\"'],
    include_prefix = "modules/drivers/radar/conti_radar",
    strip_include_prefix = "/conti_radar",
    deps = [
        ":conti_radar_message_manager",
        "//conti_radar/protocol:drivers_conti_radar_protocol",
        "@core//cyber",
        "@eigen",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_receiver",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//proto:sensor_canbus_conf_cc_proto",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/monitor_log",
        "@wheelos_msgs//wheelos_msgs/localization_msgs:localization_cc_proto",
        "@wheelos_msgs//wheelos_msgs/localization_msgs:pose_cc_proto",
    ],
    alwayslink = True,
)

cc_binary(
    name = "libconti_radar.so",
    linkshared = True,
    linkstatic = False,
    deps = [":conti_radar_canbus_lib"],
)

cpplint()

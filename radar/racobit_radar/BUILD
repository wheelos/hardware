load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@hardware.build//tools/install:install.bzl", "install")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

install(
    name = "install",
    data = [
        ":runtime_data",
    ],
    data_dest = "drivers/addition_data/racobit_radar",
    library_dest = "drivers/lib/racobit_radar",
    targets = [
        ":libracobit_radar_canbus_component.so",
    ],
)

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "conf/*.conf",
        "dag/*.dag",
        "launch/*.launch",
    ]),
)

cc_library(
    name = "racobit_radar_message_manager",
    srcs = ["racobit_radar_message_manager.cc"],
    hdrs = ["racobit_radar_message_manager.h"],
    include_prefix = "modules/drivers/radar/racobit_radar",
    strip_include_prefix = "/racobit_radar",
    deps = [
        "//racobit_radar/protocol:drivers_racobit_radar_protocol",
        "@hardware.canbus//:sensor_gflags",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@wheelos_common//modules/common/util:util_tool",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:racobit_radar_cc_proto",
    ],
)

# cc_test(
#     name = "racobit_radar_message_manager_test",
#     size = "small",
#     srcs = ["racobit_radar_message_manager_test.cc"],
#     deps = [
#         "racobit_radar_message_manager",
#         "@com_google_googletest//:gtest_main",
#     ],
# )

cc_library(
    name = "racobit_radar_canbus_lib",
    srcs = ["racobit_radar_canbus_component.cc"],
    hdrs = ["racobit_radar_canbus_component.h"],
    copts = ['-DMODULE_NAME=\\"racobit_radar_canbus\\"'],
    include_prefix = "modules/drivers/radar/racobit_radar",
    strip_include_prefix = "/racobit_radar",
    deps = [
        ":racobit_radar_message_manager",
        "//racobit_radar/protocol:drivers_racobit_radar_protocol",
        "@core//cyber",
        "@hardware.canbus//:sensor_gflags",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_receiver",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//proto:sensor_canbus_conf_cc_proto",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/monitor_log",
        "@wheelos_common//modules/common/status",
    ],
    alwayslink = True,
)

cc_binary(
    name = "libracobit_radar_canbus_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [":racobit_radar_canbus_lib"],
)

cpplint()

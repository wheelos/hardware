load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@hardware.build//tools/install:install.bzl", "install")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

ROBOSENSE_COPTS = ['-DMODULE_NAME=\\"rslidar\\"']

install(
    name = "install",
    data = [
        ":runtime_data",
    ],
    data_dest = "drivers/addition_data/lidar/rslidar",
    library_dest = "drivers/lib/lidar/rslidar",
    targets = [
        "librslidar_component.so",
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

cc_binary(
    name = "librslidar_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [
        ":rslidar_component_lib",
    ],
)

cc_library(
    name = "rslidar_component_lib",
    srcs = [
        "driver/rslidar_component.cpp",
    ],
    hdrs = [
        "driver/rslidar_component.h",
    ],
    copts = ROBOSENSE_COPTS,
    include_prefix = "modules/drivers/lidar/rslidar",
    linkopts = [
        "-lpcap",
    ],
    strip_include_prefix = "/rslidar",
    deps = [
        "//common:lidar_common",
        "//rslidar/proto:rslidar_cc_proto",
        "//rslidar/proto:rslidar_config_cc_proto",
        "@robosense-lidar-sdk//:rs_driver",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
    ],
    alwayslink = True,
)

cpplint()

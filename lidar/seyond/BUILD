load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

SEYOND_COPTS = ['-DMODULE_NAME=\\"seyond\\"']

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "dag/*.dag",
        "launch/*.launch",
    ]),
)

cc_binary(
    name = "libseyond_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [":seyond_component_lib"],
)

cc_library(
    name = "seyond_component_lib",
    srcs = [
        "src/seyond_component.cpp",
        "src/seyond_driver.cpp",
    ],
    hdrs = [
        "src/seyond_component.h",
        "src/seyond_driver.h",
    ],
    copts = SEYOND_COPTS + ["-std=c++17"],
    include_prefix = "modules/drivers/lidar/seyond",
    strip_include_prefix = "/seyond",
    deps = [
        "//common:lidar_common",
        "//seyond/proto:seyond_cc_proto",
        "//seyond/proto:seyond_config_cc_proto",
        "@core//cyber",
        "@inno-lidar-sdk",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
    ],
    alwayslink = True,
)

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

HESAI_COPTS = ['-DMODULE_NAME=\\"hesai\\"']

cc_binary(
    name = "libhesai_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [":hesai_component_lib"],
)

cc_library(
    name = "hesai_component_lib",
    srcs = [
        "driver/hesai_component.cc",
    ],
    hdrs = [
        #        "driver/driver.h",
        "driver/hesai_component.h",
    ],
    copts = HESAI_COPTS + [
        "-fopenmp",
    ],
    include_prefix = "modules/drivers/lidar/hesai",
    linkopts = [
        "-lgomp",
        "-lssl",
        "-pthread",
        "-lm",
    ],
    strip_include_prefix = "/hesai",
    deps = [
        "//common:lidar_common",
        "//hesai/proto:hesai_cc_proto",
        "//hesai/proto:hesai_config_cc_proto",
        "@core//cyber",
        "@hesai-lidar-sdk//:hesai_sdk_lib",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
    ],
    alwayslink = True,
)

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "dag/*.dag",
        "launch/*.launch",
        "correction/*/*.dat",
        "correction/*/*.csv",
    ]),
)

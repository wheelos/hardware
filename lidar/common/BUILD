load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lidar_common",
    srcs = [
        "util.cpp",
    ],
    hdrs = [
        "lidar_component_base.h",
        "lidar_component_base_impl.h",
        "sync_buffering.h",
        "sync_queue.h",
        "util.h",
    ],
    copts = [],
    include_prefix = "modules/drivers/lidar/common",
    strip_include_prefix = "/common",
    deps = [
        "//common/proto:lidar_config_base_cc_proto",
        "@boost.format",
        "@core//cyber",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
    ],
)

cpplint()

load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

LIDAR_COMPENSATOR_COPTS = ['-DMODULE_NAME=\\"lidar_compensator\\"']

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "dag/*.dag",
        "launch/*.launch",
    ]),
)

cc_binary(
    name = "liblidar_compensator_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [
        ":lidar_compensator_component_lib",
    ],
)

cc_library(
    name = "lidar_compensator_component_lib",
    srcs = [
        "compensator.cc",
        "lidar_compensator_component.cc",
    ],
    hdrs = [
        "compensator.h",
        "lidar_compensator_component.h",
    ],
    copts = LIDAR_COMPENSATOR_COPTS,
    include_prefix = "modules/drivers/lidar/compensator",
    strip_include_prefix = "/compensator",
    deps = [
        "//compensator/proto:lidar_compensator_config_cc_proto",
        "@core//cyber",
        "@eigen",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/latency_recorder",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
        "@wheelos_transform//modules/transform:buffer",
        "@wheelos_transform//modules/transform:transform_query",
    ],
    alwayslink = True,
)

cpplint()

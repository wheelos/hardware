load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@hardware.build//tools/install:install.bzl", "install")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

install(
    name = "install",
    library_dest = "drivers/lib/lidar/velodyne/compensator",
    targets = [
        ":libvelodyne_compensator_component.so",
    ],
)

cc_binary(
    name = "libvelodyne_compensator_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [":compensator_component_lib"],
)

cc_library(
    name = "compensator_component_lib",
    srcs = ["compensator_component.cc"],
    hdrs = ["compensator_component.h"],
    copts = ['-DMODULE_NAME=\\"velodyne\\"'],
    include_prefix = "modules/drivers/lidar/velodyne/compensator",
    strip_include_prefix = "/velodyne/compensator",
    deps = [
        "//proto:velodyne_cc_proto",
        "//velodyne/compensator:compensator_lib",
        "@core//cyber",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/latency_recorder",
    ],
    alwayslink = True,
)

cc_library(
    name = "compensator_lib",
    srcs = ["compensator.cc"],
    hdrs = ["compensator.h"],
    copts = ['-DMODULE_NAME=\\"velodyne\\"'],
    include_prefix = "modules/drivers/lidar/velodyne/compensator",
    strip_include_prefix = "/velodyne/compensator",
    deps = [
        "//proto:velodyne_config_cc_proto",
        "@eigen",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
        "@wheelos_transform//modules/transform:buffer",
        "@wheelos_transform//modules/transform:transform_query",
    ],
)

cpplint()

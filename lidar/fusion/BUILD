load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

LIDAR_FUSION_COPTS = ['-DMODULE_NAME=\\"lidar_fusion\\"']

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "dag/*.dag",
        "launch/*.launch",
    ]),
)

cc_binary(
    name = "liblidar_fusion_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [
        ":lidar_fusion_component_lib",
    ],
)

cc_library(
    name = "lidar_fusion_component_lib",
    srcs = [
        "lidar_fusion_component.cc",
    ],
    hdrs = [
        "lidar_fusion_component.h",
    ],
    copts = LIDAR_FUSION_COPTS,
    include_prefix = "modules/drivers/lidar/fusion",
    strip_include_prefix = "/fusion",
    deps = [
        "//fusion/proto:lidar_fusion_config_cc_proto",
        "@core//cyber",
        "@eigen",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
        "@wheelos_transform//modules/transform:buffer",
        "@wheelos_transform//modules/transform:transform_query",
    ],
    alwayslink = True,
)

cc_test(
    name = "lidar_extrinsic_test",
    size = "small",
    srcs = ["lidar_extrinsic_test.cc"],
    copts = ["-fno-access-control"],
    deps = [
        ":lidar_fusion_component_lib",
        "//velodyne/fusion:fusion_component_lib",
        "@com_google_googletest//:gtest_main",
        "@eigen",
        "@wheelos_transform//modules/transform:buffer",
        "@wheelos_transform//modules/transform:buffer_test_peer",
    ],
)

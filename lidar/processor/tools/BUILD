load("@rules_cc//cc:defs.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "lidar_fusion_baseline",
    srcs = ["lidar_fusion_baseline.cc"],
    deps = [
        "//processor:lidar_policy_lib",
        "//processor:lidar_unified_component_lib",
        "//proto:lidar_unified_component_config_cc_proto",
        "@core//cyber",
        "@core//cyber/record:record_reader",
        "@eigen",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
        "@wheelos_transform//modules/transform:buffer_interface",
    ],
)

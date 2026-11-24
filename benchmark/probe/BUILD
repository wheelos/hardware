load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "stats_accumulator",
    hdrs = [
        "arrival_sample.h",
        "stats_accumulator.h",
    ],
    include_prefix = "modules/drivers/benchmark",
    strip_include_prefix = "/probe",
)

cc_binary(
    name = "libwheelos_alloc_probe.so",
    srcs = ["alloc_probe.c"],
    linkopts = ["-ldl"],
    linkshared = True,
    linkstatic = False,
)

cc_binary(
    name = "cyber_channel_probe",
    srcs = ["cyber_channel_probe.cc"],
    deps = [
        ":stats_accumulator",
        "//proto:header_envelope_cc_proto",
        "@com_github_gflags_gflags//:gflags",
        "@core//cyber",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:imu_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:sensor_image_cc_proto",
    ],
)

cc_binary(
    name = "synthetic_sensor_pub",
    srcs = ["synthetic_sensor_pub.cc"],
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@core//cyber",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:sensor_image_cc_proto",
    ],
)

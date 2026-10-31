load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "raw_stream",
    srcs = ["raw_stream.cc"],
    hdrs = ["raw_stream.h"],
    include_prefix = "modules/drivers/gnss/stream",
    strip_include_prefix = "/stream",
    deps = [
        "//parser:data_parser",
        "//parser:rtcm_parser",
        "//proto:gnss_status_cc_proto",
        "@com_google_absl//absl/strings",
        "@core//cyber",
        "@hardware.hal//stream:stream_factory",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/util:util_tool",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:drive_state_cc_proto",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:vehicle_signal_cc_proto",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_cc_proto",
        "@wheelos_msgs//wheelos_msgs/config_msgs:vehicle_config_cc_proto",
        "@wheelos_msgs//wheelos_msgs/localization_msgs:imu_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:heading_cc_proto",
    ],
)

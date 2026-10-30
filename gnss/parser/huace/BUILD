load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "huace_parser",
    srcs = [
        "huace_parser.cc",
    ],
    hdrs = [
        "huace_messages.h",
        "huace_parser.h",
    ],
    include_prefix = "modules/drivers/gnss/parser/huace",
    strip_include_prefix = "/parser/huace",
    visibility = ["//visibility:public"],
    deps = [
        "//parser",
        "//proto:gnss_status_cc_proto",
        "//util",
        "//util:data_buffer",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@core//cyber",
        "@wheelos_common//modules/common/util",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:pnc_point_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:gnss_best_pose_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:heading_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:imu_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:ins_cc_proto",
    ],
)

cpplint()

load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "novatel_parser",
    srcs = ["novatel_parser.cc"],
    hdrs = [
        "novatel_messages.h",
        "novatel_parser.h",
    ],
    copts = ["-Ithird_party/rtklib"],
    include_prefix = "modules/drivers/gnss/parser/novatel",
    strip_include_prefix = "/parser/novatel",
    visibility = ["//visibility:public"],
    deps = [
        "//parser",
        "//parser:rtcm_decode",
        "//proto:config_cc_proto",
        "//third_party/rtklib",
        "//util",
        "@core//cyber",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/util:time_conversion",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:error_code_cc_proto",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:geometry_cc_proto",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:header_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:gnss_best_pose_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:gnss_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:gnss_raw_observation_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:heading_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:imu_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:ins_cc_proto",
    ],
)

cc_test(
    name = "novatel_parser_test",
    size = "small",
    srcs = ["novatel_parser_test.cc"],
    data = ["//:test_data_files"],
    deps = [
        ":novatel_parser",
        "@com_google_googletest//:gtest_main",
    ],
)

cpplint()

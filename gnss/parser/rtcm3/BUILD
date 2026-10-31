load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rtcm3_parser",
    srcs = [
        "rtcm3_parser.cc",
    ],
    hdrs = [
        "rtcm3_parser.h",
    ],
    include_prefix = "modules/drivers/gnss/parser/rtcm3",
    strip_include_prefix = "/parser/rtcm3",
    deps = [
        "//parser",
        "//parser:rtcm_decode",
        "//third_party/rtklib",
        "//util",
        "@core//cyber",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:gnss_raw_observation_cc_proto",
    ],
)

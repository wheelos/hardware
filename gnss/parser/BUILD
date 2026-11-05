load("@hardware.build//tools/hardware:defs.bzl", "enabled")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

GNSS_DEFINES = enabled(
    "with_gnss_forsense",
    [
        "GNSS_WITH_FORSENSE",
    ],
) + enabled(
    "with_gnss_huace",
    [
        "GNSS_WITH_HUACE",
    ],
) + enabled(
    "with_gnss_novatel",
    [
        "GNSS_WITH_NOVATEL",
    ],
)

cc_library(
    name = "parser",
    hdrs = [
        "parser.h",
    ],
    include_prefix = "modules/drivers/gnss/parser",
    strip_include_prefix = "/parser",
    deps = [
        "//proto:config_cc_proto",
        "//util:data_buffer",
        "@core//cyber",
    ],
)

cc_library(
    name = "rtcm_decode",
    hdrs = [
        "rtcm_decode.h",
    ],
    include_prefix = "modules/drivers/gnss/parser",
    strip_include_prefix = "/parser",
    deps = [
        "//third_party/rtklib",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:gnss_raw_observation_cc_proto",
    ],
)

cc_library(
    name = "parser_factory",
    srcs = ["parser_factory.cc"],
    hdrs = [
        "parser_factory.h",
    ],
    include_prefix = "modules/drivers/gnss/parser",
    local_defines = GNSS_DEFINES,
    strip_include_prefix = "/parser",
    deps = [
        ":parser",
        "//proto:config_cc_proto",
        "@core//cyber",
    ] + enabled(
        "with_gnss_forsense",
        [
            "//parser/forsense:forsense_parser",
        ],
    ) + enabled(
        "with_gnss_huace",
        [
            "//parser/huace:huace_parser",
        ],
    ) + enabled(
        "with_gnss_novatel",
        [
            "//parser/novatel:novatel_parser",
        ],
    ),
)

cc_test(
    name = "parser_factory_test",
    size = "small",
    srcs = ["parser_factory_test.cc"],
    local_defines = GNSS_DEFINES,
    deps = [
        ":parser_factory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "data_parser",
    srcs = [
        "data_parser.cc",
    ],
    hdrs = [
        "data_parser.h",
    ],
    include_prefix = "modules/drivers/gnss/parser",
    strip_include_prefix = "/parser",
    deps = [
        ":parser",
        ":parser_factory",
        "//proto:gnss_status_cc_proto",
        "//util",
        "@core//cyber",
        "@eigen",
        "@proj",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/util:message_util",
        "@wheelos_common//modules/common/util:proj_helper",
        "@wheelos_common//modules/common/util:time_conversion",
        "@wheelos_common//modules/common/util:util_tool",
        "@wheelos_msgs//wheelos_msgs/localization_msgs:gps_cc_proto",
        "@wheelos_msgs//wheelos_msgs/localization_msgs:imu_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:gnss_best_pose_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:gnss_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:gnss_raw_observation_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:heading_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:imu_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:ins_cc_proto",
    ],
)

cc_library(
    name = "rtcm_parser",
    srcs = [
        "rtcm_parser.cc",
    ],
    hdrs = [
        "parser.h",
        "rtcm_decode.h",
        "rtcm_parser.h",
    ],
    copts = ["-Ithird_party/rtklib"],
    include_prefix = "modules/drivers/gnss/parser",
    strip_include_prefix = "/parser",
    deps = [
        "//parser/rtcm3:rtcm3_parser",
        "//proto:config_cc_proto",
        "//third_party/rtklib",
        "//util",
        "@core//cyber",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:gnss_raw_observation_cc_proto",
    ],
)

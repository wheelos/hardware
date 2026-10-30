load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@hardware.build//tools/install:install.bzl", "install")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

install(
    name = "install",
    library_dest = "drivers/lib/lidar/velodyne/parser",
    targets = [
        ":libvelodyne_convert_component.so",
    ],
)

cc_binary(
    name = "libvelodyne_convert_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [":velodyne_convert_component_lib"],
)

cc_library(
    name = "velodyne_convert_component_lib",
    srcs = ["velodyne_convert_component.cc"],
    hdrs = ["velodyne_convert_component.h"],
    copts = ['-DMODULE_NAME=\\"velodyne\\"'],
    include_prefix = "modules/drivers/lidar/velodyne/parser",
    strip_include_prefix = "/velodyne/parser",
    deps = [
        "//velodyne/parser:convert",
        "@core//cyber",
    ],
    alwayslink = True,
)

cc_library(
    name = "convert",
    srcs = [
        "calibration.cc",
        "convert.cc",
        "online_calibration.cc",
        "util.cc",
        "velodyne128_parser.cc",
        "velodyne16_parser.cc",
        "velodyne32_parser.cc",
        "velodyne64_parser.cc",
        "velodyne_parser.cc",
    ],
    hdrs = [
        "calibration.h",
        "const_variables.h",
        "convert.h",
        "online_calibration.h",
        "util.h",
        "velodyne_parser.h",
    ],
    copts = ['-DMODULE_NAME=\\"velodyne\\"'],
    include_prefix = "modules/drivers/lidar/velodyne/parser",
    strip_include_prefix = "/velodyne/parser",
    deps = [
        "//proto:velodyne_config_cc_proto",
        "@boost.array",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@core//cyber",
        "@eigen",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
    ],
)

cpplint()

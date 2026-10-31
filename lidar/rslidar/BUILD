load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@rules_pkg//pkg:install.bzl", "pkg_install")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

ROBOSENSE_COPTS = ['-DMODULE_NAME=\\"rslidar\\"']

pkg_files(
    name = "package_files_data",
    srcs = [":runtime_data"],
    attributes = pkg_attributes(mode = "0644"),
    prefix = "drivers/addition_data/lidar/rslidar",
    strip_prefix = strip_prefix.from_root("rslidar"),
)

pkg_files(
    name = "package_files_libraries",
    srcs = ["librslidar_component.so"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "drivers/lib/lidar/rslidar",
    strip_prefix = strip_prefix.from_root("rslidar"),
)

pkg_filegroup(
    name = "package_files",
    srcs = [
        ":package_files_data",
        ":package_files_libraries",
    ],
)

pkg_install(
    name = "install_dir",
    srcs = [":package_files"],
    tags = ["manual"],
)

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "dag/*.dag",
        "launch/*.launch",
    ]),
)

cc_binary(
    name = "librslidar_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [
        ":rslidar_component_lib",
    ],
)

cc_library(
    name = "rslidar_component_lib",
    srcs = [
        "driver/rslidar_component.cpp",
    ],
    hdrs = [
        "driver/rslidar_component.h",
    ],
    copts = ROBOSENSE_COPTS,
    include_prefix = "modules/drivers/lidar/rslidar",
    linkopts = [
        "-lpcap",
    ],
    strip_include_prefix = "/rslidar",
    deps = [
        "//common:lidar_common",
        "//rslidar/proto:rslidar_cc_proto",
        "//rslidar/proto:rslidar_config_cc_proto",
        "@robosense-lidar-sdk//:rs_driver",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
    ],
    alwayslink = True,
)

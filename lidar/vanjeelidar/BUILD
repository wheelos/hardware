load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("@rules_pkg//pkg:install.bzl", "pkg_install")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

VANJEE_COPTS = ['-DMODULE_NAME=\\"vanjeelidar\\"']

pkg_files(
    name = "package_files_data",
    srcs = [":runtime_data"],
    attributes = pkg_attributes(mode = "0644"),
    prefix = "drivers/addition_data/lidar/vanjeelidar",
    strip_prefix = strip_prefix.from_root("vanjeelidar"),
)

pkg_files(
    name = "package_files_libraries",
    srcs = ["libvanjeelidar_component.so"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "drivers/lib/lidar/vanjeelidar",
    strip_prefix = strip_prefix.from_root("vanjeelidar"),
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
    name = "libvanjeelidar_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [
        ":vanjeelidar_component_lib",
    ],
)

cc_library(
    name = "measurement_conversion",
    srcs = ["src/measurement_conversion.cc"],
    hdrs = ["src/measurement_conversion.h"],
    include_prefix = "modules/drivers/lidar/vanjeelidar",
    strip_include_prefix = "/vanjeelidar",
    deps = [
        "//vanjeelidar/proto:vanjeelidar_cc_proto",
        "@vanjee-lidar-sdk//:vanjee_driver",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
    ],
)

cc_test(
    name = "measurement_conversion_test",
    size = "small",
    srcs = ["src/measurement_conversion_test.cc"],
    deps = [
        ":measurement_conversion",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "vanjeelidar_component_lib",
    srcs = [
        "src/vanjeelidar_component.cpp",
    ],
    hdrs = [
        "src/vanjeelidar_component.h",
    ],
    copts = VANJEE_COPTS,
    include_prefix = "modules/drivers/lidar/vanjeelidar",
    strip_include_prefix = "/vanjeelidar",
    deps = [
        ":measurement_conversion",
        "//common:lidar_common",
        "//vanjeelidar/proto:vanjeelidar_cc_proto",
        "//vanjeelidar/proto:vanjeelidar_config_cc_proto",
        "@vanjee-lidar-sdk//:vanjee_driver",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
    ],
    alwayslink = True,
)

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@rules_pkg//pkg:install.bzl", "pkg_install")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

pkg_files(
    name = "package_files_libraries",
    srcs = [":libvelodyne_compensator_component.so"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "drivers/lib/lidar/velodyne/compensator",
    strip_prefix = strip_prefix.from_root("velodyne/compensator"),
)

pkg_filegroup(
    name = "package_files",
    srcs = [":package_files_libraries"],
)

pkg_install(
    name = "install_dir",
    srcs = [":package_files"],
    tags = ["manual"],
)

cc_binary(
    name = "libvelodyne_compensator_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [":compensator_component_lib"],
)

cc_library(
    name = "compensator_component_lib",
    srcs = ["compensator_component.cc"],
    hdrs = ["compensator_component.h"],
    copts = ['-DMODULE_NAME=\\"velodyne\\"'],
    include_prefix = "modules/drivers/lidar/velodyne/compensator",
    strip_include_prefix = "/velodyne/compensator",
    deps = [
        "//proto:velodyne_cc_proto",
        "//velodyne/compensator:compensator_lib",
        "@core//cyber",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/latency_recorder",
    ],
    alwayslink = True,
)

cc_library(
    name = "compensator_lib",
    srcs = ["compensator.cc"],
    hdrs = ["compensator.h"],
    copts = ['-DMODULE_NAME=\\"velodyne\\"'],
    include_prefix = "modules/drivers/lidar/velodyne/compensator",
    strip_include_prefix = "/velodyne/compensator",
    deps = [
        "//proto:velodyne_config_cc_proto",
        "@eigen",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
        "@wheelos_transform//modules/transform:buffer",
        "@wheelos_transform//modules/transform:transform_query",
    ],
)

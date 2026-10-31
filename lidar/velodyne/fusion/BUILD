load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@rules_pkg//pkg:install.bzl", "pkg_install")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

pkg_files(
    name = "package_files_libraries",
    srcs = [":libvelodyne_fusion_component.so"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "drivers/lib/lidar/velodyne/fusion",
    strip_prefix = strip_prefix.from_root("velodyne/fusion"),
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
    name = "libvelodyne_fusion_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [":fusion_component_lib"],
)

cc_library(
    name = "fusion_component_lib",
    srcs = ["pri_sec_fusion_component.cc"],
    hdrs = ["pri_sec_fusion_component.h"],
    copts = ['-DMODULE_NAME=\\"velodyne\\"'],
    include_prefix = "modules/drivers/lidar/velodyne/fusion",
    strip_include_prefix = "/velodyne/fusion",
    deps = [
        "//proto:velodyne_config_cc_proto",
        "@core//cyber",
        "@eigen",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:pointcloud_cc_proto",
        "@wheelos_transform//modules/transform:buffer",
        "@wheelos_transform//modules/transform:transform_query",
    ],
    alwayslink = True,
)

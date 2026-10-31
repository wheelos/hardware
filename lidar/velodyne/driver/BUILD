load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@rules_pkg//pkg:install.bzl", "pkg_install")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

pkg_files(
    name = "package_files_libraries",
    srcs = [":libvelodyne_driver_component.so"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "drivers/lib/lidar/velodyne/driver",
    strip_prefix = strip_prefix.from_root("velodyne/driver"),
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
    name = "libvelodyne_driver_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [":velodyne_driver_component_lib"],
)

cc_library(
    name = "velodyne_driver_component_lib",
    srcs = ["velodyne_driver_component.cc"],
    hdrs = ["velodyne_driver_component.h"],
    copts = ['-DMODULE_NAME=\\"velodyne\\"'],
    include_prefix = "modules/drivers/lidar/velodyne/driver",
    strip_include_prefix = "/velodyne/driver",
    deps = [
        "//velodyne/driver",
        "@core//cyber",
        "@wheelos_common//modules/common/util:util_tool",
    ],
    alwayslink = True,
)

cc_library(
    name = "driver",
    srcs = [
        "driver.cc",
        "driver64.cc",
        "input.cc",
        "socket_input.cc",
    ],
    hdrs = [
        "driver.h",
        "input.h",
        "socket_input.h",
    ],
    copts = ['-DMODULE_NAME=\\"velodyne\\"'],
    include_prefix = "modules/drivers/lidar/velodyne/driver",
    strip_include_prefix = "/velodyne/driver",
    deps = [
        "//common/driver_factory:driver_base",
        "//proto:config_cc_proto",
        "//proto:velodyne_config_cc_proto",
        "@core//cyber",
        "@wheelos_common//modules/common/util:util_tool",
    ],
)

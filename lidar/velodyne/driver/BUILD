load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@hardware.build//tools/install:install.bzl", "install")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

install(
    name = "install",
    library_dest = "drivers/lib/lidar/velodyne/driver",
    targets = [
        ":libvelodyne_driver_component.so",
    ],
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

cpplint()

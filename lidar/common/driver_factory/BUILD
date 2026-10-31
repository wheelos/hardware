load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lidar_driver_factory",
    srcs = ["lidar_driver_factory.cc"],
    hdrs = ["lidar_driver_factory.h"],
    include_prefix = "modules/drivers/lidar/common/driver_factory",
    strip_include_prefix = "/common/driver_factory",
    deps = [
        ":driver_base",
        "//proto:config_cc_proto",
        "//proto:lidar_parameter_cc_proto",
        "//velodyne/driver",
        "@core//cyber",
        "@wheelos_common//modules/common/util:util_tool",
    ],
)

cc_library(
    name = "driver_base",
    hdrs = ["driver_base.h"],
    include_prefix = "modules/drivers/lidar/common/driver_factory",
    strip_include_prefix = "/common/driver_factory",
    deps = [
        "@wheelos_common//modules/common/util",
        "@wheelos_common//modules/common/util:util_tool",
    ],
)

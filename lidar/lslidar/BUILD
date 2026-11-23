load("@core//runtime:runtime.bzl", "runtime_module", "runtime_resources")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "dag/*.dag",
        "launch/*.launch",
        "params/*.yaml",
    ]),
)

runtime_resources(
    name = "runtime_resources",
    files = [":runtime_data"],
    module_name = "lslidar",
)

runtime_module(
    name = "module",
    components = [":liblslidar_component.so"],
    module_name = "lslidar",
    resources = [":runtime_resources"],
)

cc_binary(
    name = "liblslidar_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [
        ":lslidar_component_lib",
    ],
)

cc_library(
    name = "lslidar_component_lib",
    srcs = ["component/lslidar_component.cpp"],
    hdrs = ["component/lslidar_component.h"],
    copts = ['-DMODULE_NAME=\\"lslidar\\"'],
    include_prefix = "modules/drivers/lidar/lslidar",
    strip_include_prefix = "/lslidar",
    deps = [
        "//common:lidar_common",
        "//lslidar/driver",
        "//lslidar/parser:convert",
        "@core//cyber",
    ],
    alwayslink = True,
)

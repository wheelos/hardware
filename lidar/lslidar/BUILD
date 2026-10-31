load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

# install(
#     name = "install",
#     data = [
#         ":runtime_data",
#     ],
#     data_dest = "drivers/addition_data/lidar/lslidar",
#     library_dest = "drivers/lib/lidar/lslidar",
#     targets = [
#         "//lslidar/driver:liblslidar_driver_component.so",
#         "//lslidar/parser:liblslidar_convert_component.so",
#     ],
# )

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "dag/*.dag",
        "launch/*.launch",
        "params/*.yaml",
    ]),
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

load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "driver",
    srcs = [
        "driver.cc",
        "input.cc",
    ],
    hdrs = [
        "driver.h",
        "input.h",
    ],
    copts = ['-DMODULE_NAME=\\"lslidar\\"'],
    include_prefix = "modules/drivers/lidar/lslidar/driver",
    linkopts = [
        "-lpcap",
    ],
    strip_include_prefix = "/lslidar/driver",
    deps = [
        "//lslidar/proto:config_cc_proto",
        "@core//cyber",
        "@wheelos_common//modules/common/util:util_tool",
    ],
)

cpplint()

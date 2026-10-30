load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "data_buffer",
    srcs = [
        "data_buffer.cc",
    ],
    hdrs = [
        "data_buffer.h",
    ],
    include_prefix = "modules/drivers/gnss/util",
    strip_include_prefix = "/util",
    deps = [
        "@core//cyber",
    ],
)

cc_test(
    name = "data_buffer_test",
    srcs = ["data_buffer_test.cc"],
    deps = [
        ":data_buffer",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "util",
    hdrs = [
        "util.h",
    ],
    include_prefix = "modules/drivers/gnss/util",
    strip_include_prefix = "/util",
    deps = [
        "@wheelos_msgs//wheelos_msgs/basic_msgs:geometry_cc_proto",
    ],
)

cc_test(
    name = "util_test",
    srcs = [
        "util_test.cc",
    ],
    deps = [
        ":util",
        "@com_google_googletest//:gtest_main",
    ],
)

cpplint()

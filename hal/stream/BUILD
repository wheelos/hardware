load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "stream",
    hdrs = ["stream.h"],
    include_prefix = "modules/drivers/hal/stream",
    strip_include_prefix = "/stream",
    deps = [
        "@core//cyber",
    ],
)

cc_library(
    name = "can_stream",
    srcs = ["can_stream.cc"],
    hdrs = [
        "can_stream.h",
        "can_stream_utils.h",
    ],
    include_prefix = "modules/drivers/hal/stream",
    strip_include_prefix = "/stream",
    deps = [
        ":stream",
        "@core//cyber",
    ],
)

cc_library(
    name = "ntrip_stream",
    srcs = ["ntrip_stream.cc"],
    hdrs = ["ntrip_stream.h"],
    include_prefix = "modules/drivers/hal/stream",
    strip_include_prefix = "/stream",
    deps = [
        ":stream",
        ":tcp_stream",
        "@core//cyber",
        "@wheelos_common//modules/common/util",
    ],
)

cc_library(
    name = "serial_stream",
    srcs = ["serial_stream.cc"],
    hdrs = ["serial_stream.h"],
    include_prefix = "modules/drivers/hal/stream",
    strip_include_prefix = "/stream",
    deps = [
        ":stream",
        "@core//cyber",
    ],
)

cc_library(
    name = "tcp_stream",
    srcs = ["tcp_stream.cc"],
    hdrs = ["tcp_stream.h"],
    include_prefix = "modules/drivers/hal/stream",
    strip_include_prefix = "/stream",
    deps = [
        ":stream",
        "@core//cyber",
    ],
)

cc_library(
    name = "udp_stream",
    srcs = ["udp_stream.cc"],
    hdrs = ["udp_stream.h"],
    include_prefix = "modules/drivers/hal/stream",
    strip_include_prefix = "/stream",
    deps = [
        ":stream",
        "@core//cyber",
    ],
)

cc_library(
    name = "stream_factory",
    srcs = ["stream_factory.cc"],
    hdrs = ["stream_factory.h"],
    include_prefix = "modules/drivers/hal/stream",
    strip_include_prefix = "/stream",
    deps = [
        ":can_stream",
        ":ntrip_stream",
        ":serial_stream",
        ":stream",
        ":tcp_stream",
        ":udp_stream",
        "@core//cyber",
    ],
    alwayslink = 1,
)

cc_test(
    name = "can_stream_regression_test",
    srcs = ["can_stream_regression_test.cc"],
    deps = [
        ":can_stream",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cpplint()

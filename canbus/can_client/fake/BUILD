load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fake_can_client",
    srcs = ["fake_can_client.cc"],
    hdrs = ["fake_can_client.h"],
    include_prefix = "modules/drivers/canbus/can_client/fake",
    strip_include_prefix = "/can_client/fake",
    deps = [
        "//can_client",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:error_code_cc_proto",
    ],
)

cc_test(
    name = "fake_can_client_test",
    size = "small",
    srcs = ["fake_can_client_test.cc"],
    deps = [
        "//can_client/fake:fake_can_client",
        "//common:canbus_common",
        "@com_google_googletest//:gtest_main",
        "@core//cyber",
    ],
)

cpplint()

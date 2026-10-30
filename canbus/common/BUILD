load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@hardware.build//tools/install:install.bzl", "install")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

install(
    name = "install",
    library_dest = "drivers/lib",
    targets = [
        ":canbus_common",
    ],
)

cc_library(
    name = "canbus_common",
    srcs = [
        "byte.cc",
    ],
    hdrs = [
        "byte.h",
        "canbus_consts.h",
    ],
    include_prefix = "modules/drivers/canbus/common",
    strip_include_prefix = "/common",
    deps = [
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "byte_test",
    size = "small",
    srcs = ["byte_test.cc"],
    deps = [
        ":canbus_common",
        "@com_google_googletest//:gtest_main",
    ],
)

cpplint()

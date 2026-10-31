load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_pkg//pkg:install.bzl", "pkg_install")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

pkg_files(
    name = "package_files_archives",
    srcs = [":canbus_common"],
    attributes = pkg_attributes(mode = "0644"),
    prefix = "lib",
    strip_prefix = strip_prefix.from_root("common"),
)

pkg_filegroup(
    name = "package_files",
    srcs = [":package_files_archives"],
)

pkg_install(
    name = "install_dir",
    srcs = [":package_files"],
    tags = ["manual"],
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

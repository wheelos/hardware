load("@core//runtime:install.bzl", "pkg_install")
load("@hardware.build//tools/hardware:defs.bzl", "enabled")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

CAN_DEFINES = enabled(
    "with_canbus_fake",
    ["CANBUS_WITH_FAKE"],
) + enabled(
    "with_canbus_socket",
    ["CANBUS_WITH_SOCKET"],
)

pkg_files(
    name = "package_files_binaries",
    srcs = [":can_client_tool"],
    attributes = pkg_attributes(mode = "0755"),
    include_runfiles = True,
    prefix = "drivers/bin",
    strip_prefix = strip_prefix.from_root("can_client"),
)

pkg_filegroup(
    name = "package_files",
    srcs = [":package_files_binaries"],
)

pkg_install(
    name = "install_dir",
    srcs = [":package_files"],
    tags = ["manual"],
)

cc_library(
    name = "can_client_factory",
    srcs = ["can_client_factory.cc"],
    hdrs = ["can_client_factory.h"],
    include_prefix = "modules/drivers/canbus/can_client",
    local_defines = CAN_DEFINES,
    strip_include_prefix = "/can_client",
    deps = [
        "//can_client",
        "//common:canbus_common",
        "@core//cyber",
        "@wheelos_common//modules/common/util:util_tool",
        "@wheelos_msgs//wheelos_msgs/drivers_msgs:can_card_parameter_cc_proto",
    ] + enabled(
        "with_canbus_fake",
        [
            "//can_client/fake:fake_can_client",
        ],
    ) + enabled(
        "with_canbus_socket",
        [
            "//can_client/socket:socket_can_client_raw",
        ],
    ),
    alwayslink = True,
)

cc_library(
    name = "can_client",
    hdrs = ["can_client.h"],
    include_prefix = "modules/drivers/canbus/can_client",
    strip_include_prefix = "/can_client",
    deps = [
        "//common:canbus_common",
        "@wheelos_common//modules/common/util",
        "@wheelos_msgs//wheelos_msgs/drivers_msgs:can_card_parameter_cc_proto",
    ],
    alwayslink = True,
)

cc_binary(
    name = "can_client_tool",
    srcs = ["can_client_tool.cc"],
    deps = [
        "//can_client:can_client_factory",
        "@com_github_gflags_gflags//:gflags",
        "@core//cyber",
        "@wheelos_msgs//wheelos_msgs/drivers_msgs:can_card_parameter_cc_proto",
    ],
)

cc_test(
    name = "can_client_factory_test",
    size = "small",
    srcs = ["can_client_factory_test.cc"],
    local_defines = CAN_DEFINES,
    deps = [
        "//can_client:can_client_factory",
        "@com_google_googletest//:gtest_main",
        "@wheelos_msgs//wheelos_msgs/drivers_msgs:can_card_parameter_cc_proto",
    ],
)

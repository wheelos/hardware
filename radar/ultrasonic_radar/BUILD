load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@rules_pkg//pkg:install.bzl", "pkg_install")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

pkg_files(
    name = "package_files_data",
    srcs = [":runtime_data"],
    attributes = pkg_attributes(mode = "0644"),
    prefix = "drivers/addition_data/ultrasonic_radar",
    strip_prefix = strip_prefix.from_root("ultrasonic_radar"),
)

pkg_files(
    name = "package_files_libraries",
    srcs = [":libultrasonic_radar_canbus_component.so"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "drivers/lib/racobit_radar",
    strip_prefix = strip_prefix.from_root("ultrasonic_radar"),
)

pkg_filegroup(
    name = "package_files",
    srcs = [
        ":package_files_data",
        ":package_files_libraries",
    ],
)

pkg_install(
    name = "install_dir",
    srcs = [":package_files"],
    tags = ["manual"],
)

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "conf/*.conf",
        "dag/*.dag",
        "launch/*.launch",
    ]),
)

cc_library(
    name = "ultrasonic_radar_message_manager",
    srcs = ["ultrasonic_radar_message_manager.cc"],
    hdrs = ["ultrasonic_radar_message_manager.h"],
    include_prefix = "modules/drivers/radar/ultrasonic_radar",
    strip_include_prefix = "/ultrasonic_radar",
    deps = [
        "@hardware.canbus//:sensor_gflags",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@wheelos_common//modules/common/util:util_tool",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:header_cc_proto",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:ultrasonic_radar_cc_proto",
    ],
)

#  cc_test(
#    name = "ultrasonic_radar_message_manager_test",
#    size = "small",
#     srcs = ["ultrasonic_radar_message_manager_test.cc"],
#     deps = [
#         "ultrasonic_radar_message_manager",
#         "@com_google_googletest//:gtest_main",
#     ],
# )

cc_library(
    name = "ultrasonic_radar_canbus_lib",
    srcs = ["ultrasonic_radar_canbus.cc"],
    hdrs = ["ultrasonic_radar_canbus.h"],
    include_prefix = "modules/drivers/radar/ultrasonic_radar",
    strip_include_prefix = "/ultrasonic_radar",
    deps = [
        ":ultrasonic_radar_message_manager",
        "//ultrasonic_radar/proto:ultrasonic_radar_conf_cc_proto",
        "@hardware.canbus//:sensor_gflags",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_receiver",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//proto:sensor_canbus_conf_cc_proto",
        "@wheelos_common//modules/common/monitor_log",
        "@wheelos_common//modules/common/status",
    ],
)

# cc_test(
#     name = "ultrasonic_radar_canbus_test",
#     size = "small",
#     srcs = ["ultrasonic_radar_canbus_test.cc"],
#     deps = [
#         ":ultrasonic_radar_canbus_lib",
#         ":ultrasonic_radar_message_manager",
#         "@com_google_googletest//:gtest_main",
#     ],
# )

cc_library(
    name = "ultrasonic_radar_canbus_component_lib",
    srcs = ["ultrasonic_radar_canbus_component.cc"],
    hdrs = ["ultrasonic_radar_canbus_component.h"],
    copts = ['-DMODULE_NAME=\\"ultrasonic_radar_canbus\\"'],
    include_prefix = "modules/drivers/radar/ultrasonic_radar",
    strip_include_prefix = "/ultrasonic_radar",
    deps = [
        ":ultrasonic_radar_canbus_lib",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:ultrasonic_radar_cc_proto",
    ],
    alwayslink = True,
)

cc_binary(
    name = "libultrasonic_radar_canbus_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [":ultrasonic_radar_canbus_component_lib"],
)

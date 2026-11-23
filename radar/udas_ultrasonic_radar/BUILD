load("@core//runtime:install.bzl", "pkg_install")
load("@core//runtime:runtime.bzl", "runtime_module", "runtime_resources")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

pkg_files(
    name = "package_files_data",
    srcs = [":runtime_data"],
    attributes = pkg_attributes(mode = "0644"),
    prefix = "drivers/addition_data/udas_ultrasonic_radar",
    strip_prefix = strip_prefix.from_root("udas_ultrasonic_radar"),
)

pkg_files(
    name = "package_files_libraries",
    srcs = [":libudas_ultrasonic_radar_component.so"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "drivers/lib/udas_ultrasonic_radar",
    strip_prefix = strip_prefix.from_root("udas_ultrasonic_radar"),
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
        "dag/*.dag",
        "launch/*.launch",
    ]),
)

runtime_resources(
    name = "runtime_resources",
    files = [":runtime_data"],
    module_name = "udas_ultrasonic_radar",
)

runtime_module(
    name = "module",
    components = [":libudas_ultrasonic_radar_component.so"],
    module_name = "udas_ultrasonic_radar",
    resources = [":runtime_resources"],
)

cc_binary(
    name = "libudas_ultrasonic_radar_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [
        ":udas_ultrasonic_radar_component_lib",
    ],
)

cc_library(
    name = "udas_ultrasonic_radar_component_lib",
    srcs = [
        "udas_ultrasonic_radar_component.cc",
        "udas_ultrasonic_radar_message_manager.cc",
    ],
    hdrs = [
        "udas_ultrasonic_radar_component.h",
        "udas_ultrasonic_radar_message_manager.h",
    ],
    include_prefix = "modules/drivers/radar/udas_ultrasonic_radar",
    strip_include_prefix = "/udas_ultrasonic_radar",
    deps = [
        "//udas_ultrasonic_radar/proto:udas_ultrasonic_radar_cc_proto",
        "//udas_ultrasonic_radar/proto:ultrasonic_radar_config_cc_proto",
        "//udas_ultrasonic_radar/protocol:canbus_udas_ultrasonic_radar_protocol",
        "@core//cyber",
        "@hardware.canbus//can_client",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_receiver",
        "@hardware.canbus//can_comm:message_manager_base",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/util:message_util",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:ultrasonic_radar_cc_proto",
    ],
    alwayslink = True,
)

load("@core//runtime:install.bzl", "pkg_install")
load("@core//runtime:runtime.bzl", "runtime_module", "runtime_resources")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

pkg_files(
    name = "package_files_data",
    srcs = [":runtime_data"],
    attributes = pkg_attributes(mode = "0644"),
    prefix = "drivers/addition_data/nano_radar",
    strip_prefix = strip_prefix.from_root("nano_radar"),
)

pkg_files(
    name = "package_files_libraries",
    srcs = [":libnano_radar.so"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "drivers/lib/nano_radar",
    strip_prefix = strip_prefix.from_root("nano_radar"),
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
    module_name = "nano_radar",
)

runtime_module(
    name = "module",
    components = [":libnano_radar.so"],
    module_name = "nano_radar",
    resources = [":runtime_resources"],
)

cc_library(
    name = "nano_radar_message_manager",
    srcs = ["nano_radar_message_manager.cc"],
    hdrs = ["nano_radar_message_manager.h"],
    include_prefix = "modules/drivers/radar/nano_radar",
    strip_include_prefix = "/nano_radar",
    deps = [
        "//nano_radar/protocol:drivers_nano_radar_protocol",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_sender",
        "@hardware.canbus//can_comm:message_manager_base",
        "@wheelos_common//modules/common/util:message_util",
    ],
)

cc_library(
    name = "nano_radar_lib",
    srcs = ["nano_radar_canbus_component.cc"],
    hdrs = ["nano_radar_canbus_component.h"],
    copts = ['-DMODULE_NAME=\\"nano_radar\\"'],
    include_prefix = "modules/drivers/radar/nano_radar",
    strip_include_prefix = "/nano_radar",
    deps = [
        ":nano_radar_message_manager",
        "//nano_radar/protocol:drivers_nano_radar_protocol",
        "@core//cyber",
        "@eigen",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_receiver",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//proto:sensor_canbus_conf_cc_proto",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/monitor_log",
    ],
    alwayslink = True,
)

cc_binary(
    name = "libnano_radar.so",
    linkshared = True,
    linkstatic = False,
    deps = [":nano_radar_lib"],
)

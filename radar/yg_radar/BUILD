load("@core//runtime:install.bzl", "pkg_install")
load("@core//runtime:runtime.bzl", "runtime_module", "runtime_resources")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

pkg_files(
    name = "package_files_data",
    srcs = [":runtime_data"],
    attributes = pkg_attributes(mode = "0644"),
    prefix = "drivers/addition_data/yg_radar",
    strip_prefix = strip_prefix.from_root("yg_radar"),
)

pkg_files(
    name = "package_files_libraries",
    srcs = [":libyg_radar_component.so"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "drivers/lib/yg_radar",
    strip_prefix = strip_prefix.from_root("yg_radar"),
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

runtime_resources(
    name = "runtime_resources",
    files = [":runtime_data"],
    module_name = "yg_radar",
)

runtime_module(
    name = "module",
    components = [":libyg_radar_component.so"],
    module_name = "yg_radar",
    resources = [":runtime_resources"],
)

cc_binary(
    name = "libyg_radar_component.so",
    linkshared = True,
    linkstatic = False,
    deps = [
        ":yg_radar_component_lib",
    ],
)

cc_library(
    name = "yg_radar_component_lib",
    srcs = [
        "yg_radar_component.cc",
        "yg_radar_message_manager.cc",
    ],
    hdrs = [
        "yg_radar_component.h",
        "yg_radar_message_manager.h",
    ],
    include_prefix = "modules/drivers/radar/yg_radar",
    strip_include_prefix = "/yg_radar",
    deps = [
        "//yg_radar/proto:yg_radar_cc_proto",
        "//yg_radar/proto:yg_radar_config_cc_proto",
        "//yg_radar/protocol:canbus_yg_radar_protocol",
        "@core//cyber",
        "@hardware.canbus//can_client",
        "@hardware.canbus//can_client:can_client_factory",
        "@hardware.canbus//can_comm:can_receiver",
        "@hardware.canbus//can_comm:message_manager_base",
        "@wheelos_common//modules/common/adapters:adapter_gflags",
        "@wheelos_common//modules/common/util:message_util",
        "@wheelos_msgs//wheelos_msgs/sensor_msgs:radar_cc_proto",
    ],
    alwayslink = True,
)

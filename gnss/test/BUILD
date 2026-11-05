load("@core//runtime:install.bzl", "pkg_install")
load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

pkg_files(
    name = "package_files_binaries",
    srcs = [":parser_cli"],
    attributes = pkg_attributes(mode = "0755"),
    include_runfiles = True,
    prefix = "drivers/bin",
    strip_prefix = strip_prefix.from_root("test"),
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

cc_binary(
    name = "parser_cli",
    srcs = ["parser_cli.cc"],
    deps = [
        "//parser:data_parser",
        "//proto:config_cc_proto",
        "//proto:gnss_status_cc_proto",
        "@core//cyber",
        "@hardware.hal//stream",
    ],
)

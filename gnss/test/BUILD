load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@hardware.build//tools/install:install.bzl", "install")
load("@rules_cc//cc:defs.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

install(
    name = "install",
    runtime_dest = "drivers/bin",
    targets = [
        ":parser_cli",
    ],
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

cpplint()

load("@com_google_protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@com_google_protobuf//bazel:proto_library.bzl", "proto_library")

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "equipment_config_proto",
    srcs = ["equipment_config.proto"],
    import_prefix = "modules/drivers/equipment/proto",
    strip_import_prefix = "/proto",
    deps = ["@com_google_protobuf//:any_proto"],
)

cc_proto_library(
    name = "equipment_config_cc_proto",
    deps = [":equipment_config_proto"],
)

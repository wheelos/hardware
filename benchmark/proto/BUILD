load("@com_google_protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@com_google_protobuf//bazel:proto_library.bzl", "proto_library")

package(default_visibility = ["//visibility:public"])

proto_library(
    name = "header_envelope_proto",
    srcs = ["header_envelope.proto"],
    import_prefix = "modules/drivers/benchmark",
    strip_import_prefix = "/proto",
    deps = [
        "@wheelos_msgs//wheelos_msgs/basic_msgs:header_proto",
    ],
)

cc_proto_library(
    name = "header_envelope_cc_proto",
    deps = [":header_envelope_proto"],
)

load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@hardware.build//tools/install:install.bzl", "install")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

package(default_visibility = ["//visibility:public"])

install(
    name = "install",
    runtime_dest = "drivers/bin",
    targets = [
        ":video2jpg",
    ],
)

cc_binary(
    name = "video2jpg",
    srcs = ["video2jpg.cc"],
    deps = [
        ":frame_processor",
        "@com_github_gflags_gflags//:gflags",
        "@ffmpeg//:avutil",
    ],
)

cc_library(
    name = "frame_processor",
    srcs = ["frame_processor.cc"],
    hdrs = ["frame_processor.h"],
    include_prefix = "modules/drivers/video/tools/decode_video",
    strip_include_prefix = "/tools/decode_video",
    deps = [
        ":h265_decoder",
        "@com_google_absl//absl/strings",
        "@wheelos_common//modules/common/util",
    ],
)

cc_library(
    name = "h265_decoder",
    srcs = ["h265_decoder.cc"],
    hdrs = ["h265_decoder.h"],
    include_prefix = "modules/drivers/video/tools/decode_video",
    strip_include_prefix = "/tools/decode_video",
    deps = [
        "@core//cyber",
        "@ffmpeg//:avcodec",
        "@ffmpeg//:avformat",
        "@ffmpeg//:swscale",
    ],
)

cpplint()

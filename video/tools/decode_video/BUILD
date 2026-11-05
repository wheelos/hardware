load("@core//runtime:install.bzl", "pkg_install")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

pkg_files(
    name = "package_files_binaries",
    srcs = [":video2jpg"],
    attributes = pkg_attributes(mode = "0755"),
    include_runfiles = True,
    prefix = "drivers/bin",
    strip_prefix = strip_prefix.from_root("tools/decode_video"),
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

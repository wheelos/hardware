load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "canbus_yg_radar_protocol",
    srcs = [
        "radar_object_info_540.cc",
        "radar_object_list_560.cc",
    ],
    hdrs = [
        "radar_object_info_540.h",
        "radar_object_list_560.h",
    ],
    include_prefix = "modules/drivers/radar/yg_radar/protocol",
    strip_include_prefix = "/yg_radar/protocol",
    deps = [
        "//yg_radar/proto:yg_radar_cc_proto",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
    ],
)

cpplint()

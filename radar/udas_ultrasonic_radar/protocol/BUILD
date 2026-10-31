load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "canbus_udas_ultrasonic_radar_protocol",
    srcs = [
        "sensor_dist_f_5c9.cc",
        "sensor_dist_r_5c8.cc",
    ],
    hdrs = [
        "sensor_dist_f_5c9.h",
        "sensor_dist_r_5c8.h",
    ],
    include_prefix = "modules/drivers/radar/udas_ultrasonic_radar/protocol",
    strip_include_prefix = "/udas_ultrasonic_radar/protocol",
    deps = [
        "//udas_ultrasonic_radar/proto:udas_ultrasonic_radar_cc_proto",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
    ],
)

load("@hardware.build//tools:cpplint.bzl", "cpplint")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "canbus_mk_mini_protocol",
    srcs = [
        "bms_flag_infor_18c4e2ef.cc",
        "bms_infor_18c4e1ef.cc",
        "ctrl_cmd_18c4d2d0.cc",
        "ctrl_fb_18c4d2ef.cc",
        "io_cmd_18c4d7d0.cc",
        "io_fb_18c4daef.cc",
        "lr_wheel_fb_18c4d7ef.cc",
        "odo_fb_18c4deef.cc",
        "rr_wheel_fb_18c4d8ef.cc",
        "sensor_reset_18ffffff.cc",
        "ultrasonic_1_fb_18c4e8ef.cc",
        "ultrasonic_2_fb_18c4e9ef.cc",
        "veh_fb_diag_18c4eaef.cc",
    ],
    hdrs = [
        "bms_flag_infor_18c4e2ef.h",
        "bms_infor_18c4e1ef.h",
        "ctrl_cmd_18c4d2d0.h",
        "ctrl_fb_18c4d2ef.h",
        "io_cmd_18c4d7d0.h",
        "io_fb_18c4daef.h",
        "lr_wheel_fb_18c4d7ef.h",
        "odo_fb_18c4deef.h",
        "rr_wheel_fb_18c4d8ef.h",
        "sensor_reset_18ffffff.h",
        "ultrasonic_1_fb_18c4e8ef.h",
        "ultrasonic_2_fb_18c4e9ef.h",
        "veh_fb_diag_18c4eaef.h",
    ],
    include_prefix = "modules/canbus/vehicle/mk_mini/protocol",
    strip_include_prefix = "/brands/mk_mini/protocol",
    deps = [
        "//brands:chassis_extension_tools",
        "//brands/mk_mini/proto:mk_mini_cc_proto",
        "@hardware.canbus//can_comm:message_manager_base",
        "@hardware.canbus//common:canbus_common",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cpplint()

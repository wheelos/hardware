load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "can_receiver",
    hdrs = ["can_receiver.h"],
    include_prefix = "modules/drivers/canbus/can_comm",
    strip_include_prefix = "/can_comm",
    deps = [
        "//can_client",
        "//can_comm:message_manager_base",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:error_code_cc_proto",
    ],
)

cc_library(
    name = "can_sender",
    hdrs = ["can_sender.h"],
    include_prefix = "modules/drivers/canbus/can_comm",
    strip_include_prefix = "/can_comm",
    deps = [
        "//can_client",
        "//can_comm:message_manager_base",
        "@com_google_googletest//:gtest",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:error_code_cc_proto",
    ],
)

cc_library(
    name = "message_manager_base",
    hdrs = [
        "message_manager.h",
        "protocol_data.h",
    ],
    include_prefix = "modules/drivers/canbus/can_comm",
    strip_include_prefix = "/can_comm",
    deps = [
        "//common:canbus_common",
        "@core//cyber",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:error_code_cc_proto",
    ],
)

cc_test(
    name = "can_sender_test",
    size = "small",
    srcs = ["can_sender_test.cc"],
    deps = [
        "//can_client/fake:fake_can_client",
        "//can_comm:can_sender",
        "//common:canbus_common",
        "@com_google_googletest//:gtest_main",
        "@core//cyber",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_cc_proto",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_test(
    name = "can_receiver_test",
    size = "small",
    srcs = ["can_receiver_test.cc"],
    linkstatic = True,
    deps = [
        "//can_client/fake:fake_can_client",
        "//can_comm:can_receiver",
        "//common:canbus_common",
        "@com_google_googletest//:gtest_main",
        "@core//cyber",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:drive_state_cc_proto",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:header_cc_proto",
        "@wheelos_msgs//wheelos_msgs/basic_msgs:vehicle_signal_cc_proto",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_cc_proto",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
        "@wheelos_msgs//wheelos_msgs/config_msgs:vehicle_config_cc_proto",
    ],
)

cc_test(
    name = "protocol_data_test",
    size = "small",
    srcs = ["protocol_data_test.cc"],
    linkstatic = True,
    deps = [
        "//can_comm:message_manager_base",
        "@com_google_googletest//:gtest_main",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_cc_proto",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

cc_test(
    name = "message_manager_test",
    size = "small",
    srcs = ["message_manager_test.cc"],
    deps = [
        "//can_comm:message_manager_base",
        "@com_google_googletest//:gtest_main",
        "@core//cyber",
        "@wheelos_msgs//wheelos_msgs/chassis_msgs:chassis_detail_cc_proto",
    ],
)

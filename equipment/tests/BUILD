load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fake_stream",
    testonly = True,
    hdrs = ["fake_stream.h"],
    deps = ["//:runtime"],
)

cc_test(
    name = "channel_test",
    size = "small",
    srcs = ["channel_test.cc"],
    deps = [
        ":fake_stream",
        "//:binary_devices",
        "//:can_signal_backend",
        "//:runtime",
        "@com_google_googletest//:gtest_main",
        "@hardware.hal//stream:can_stream",
    ],
)

cc_test(
    name = "simulation_test",
    size = "small",
    srcs = ["simulation_test.cc"],
    data = ["//:simulation_config"],
    deps = [
        ":fake_stream",
        "//:runtime",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "equipment_runtime_test",
    size = "small",
    srcs = ["equipment_runtime_test.cc"],
    deps = [
        ":fake_stream",
        "//:runtime",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "equipment_stream_test",
    size = "small",
    srcs = ["equipment_stream_test.cc"],
    deps = [
        ":fake_stream",
        "//:binary_devices",
        "//:runtime",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "charger_device_test",
    size = "small",
    srcs = ["charger_device_test.cc"],
    deps = [
        ":fake_stream",
        "//:charger",
        "//:runtime",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "equipment_service_test",
    size = "medium",
    srcs = ["equipment_service_test.cc"],
    deps = [
        ":fake_stream",
        "//:client",
        "//:service",
        "@com_google_googletest//:gtest",
        "@core//cyber",
    ],
)

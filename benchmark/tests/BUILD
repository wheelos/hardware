load("@rules_cc//cc:defs.bzl", "cc_test")
load("@rules_python//python:defs.bzl", "py_test")

package(default_visibility = ["//visibility:public"])

cc_test(
    name = "stats_accumulator_test",
    size = "small",
    srcs = ["stats_accumulator_test.cc"],
    deps = [
        "//probe:stats_accumulator",
        "@com_google_googletest//:gtest_main",
    ],
)

py_test(
    name = "bench_engine_test",
    size = "medium",
    srcs = ["bench_engine_test.py"],
    data = [
        "//probe:cyber_channel_probe",
        "//probe:libwheelos_alloc_probe.so",
        "//probe:synthetic_sensor_pub",
    ],
    imports = [".."],
    deps = [
        "//cli:bench_lib",
        "//cli:vanjee-10hz-bench",
        "//cli:wheelos-sensor-bench",
    ],
)

load("@rules_python//python:defs.bzl", "py_binary", "py_library")

package(default_visibility = ["//visibility:public"])

py_library(
    name = "bench_lib",
    srcs = [
        "__init__.py",
        "collectors.py",
        "report.py",
        "scoring.py",
    ],
    imports = [".."],
)

py_binary(
    name = "wheelos-sensor-bench",
    srcs = ["wheelos_sensor_bench.py"],
    data = [
        "//probe:cyber_channel_probe",
        "//probe:libwheelos_alloc_probe.so",
    ],
    imports = [".."],
    main = "wheelos_sensor_bench.py",
    deps = [":bench_lib"],
)

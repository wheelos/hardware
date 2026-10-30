load("@bazel_skylib//lib:selects.bzl", "selects")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag", "string_flag")

package(default_visibility = ["//visibility:public"])

string_flag(
    name = "profile",
    build_setting_default = "custom",
)

string_flag(
    name = "camera",
    build_setting_default = "none",
    values = [
        "none",
        "camera",
        "camera_gst",
    ],
)

string_flag(
    name = "canbus",
    build_setting_default = "all",
    values = [
        "none",
        "all",
        "socket",
        "fake",
    ],
)

string_flag(
    name = "gnss",
    build_setting_default = "all",
    values = [
        "none",
        "all",
        "huace",
        "novatel",
        "forsense",
    ],
)

string_flag(
    name = "vehicle",
    build_setting_default = "legacy",
    values = [
        "legacy",
        "none",
        "ch",
        "devkit",
        "ge3",
        "gem",
        "lexus",
        "lincoln",
        "mk_mini",
        "neolix_edu",
        "transit",
        "wey",
        "yunle",
        "zhongyun",
    ],
)

[
    bool_flag(
        name = "lidar_" + brand,
        build_setting_default = False,
    )
    for brand in [
        "hesai",
        "livox",
        "lslidar",
        "rslidar",
        "seyond",
        "vanjeelidar",
        "velodyne",
    ]
]

[
    config_setting(
        name = "with_lidar_" + brand,
        flag_values = {":lidar_" + brand: "true"},
    )
    for brand in [
        "hesai",
        "livox",
        "lslidar",
        "rslidar",
        "seyond",
        "vanjeelidar",
        "velodyne",
    ]
]

[
    config_setting(
        name = dimension + "_" + value,
        flag_values = {":" + dimension: value},
    )
    for dimension, values in {
        "camera": [
            "none",
            "camera",
            "camera_gst",
        ],
        "canbus": [
            "none",
            "all",
            "socket",
            "fake",
        ],
        "gnss": [
            "none",
            "all",
            "huace",
            "novatel",
            "forsense",
        ],
    }.items()
    for value in values
]

[
    selects.config_setting_group(
        name = "with_gnss_" + brand,
        match_any = [
            ":gnss_all",
            ":gnss_" + brand,
        ],
    )
    for brand in [
        "huace",
        "novatel",
        "forsense",
    ]
]

[
    selects.config_setting_group(
        name = "with_canbus_" + backend,
        match_any = [
            ":canbus_all",
            ":canbus_" + backend,
        ],
    )
    for backend in [
        "socket",
        "fake",
    ]
]

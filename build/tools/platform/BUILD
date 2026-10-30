package(default_visibility = ["//visibility:public"])

constraint_setting(
    name = "camera_gst_platform",
)

constraint_value(
    name = "jetson_orin",
    constraint_setting = ":camera_gst_platform",
)

config_setting(
    name = "is_jetson_orin",
    constraint_values = [":jetson_orin"],
)

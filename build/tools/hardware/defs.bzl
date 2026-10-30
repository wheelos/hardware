"""Hardware build-setting helpers used by hardware-owned targets."""

load("@bazel_skylib//lib:selects.bzl", "selects")

VEHICLES = [
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
]

def enabled(name, targets):
    return select({
        Label("//tools/hardware:" + name): targets,
        "//conditions:default": [],
    })

def vehicle_selection(name):
    brand = name.removeprefix("with_")
    native.config_setting(
        name = name + "_legacy",
        define_values = {"vehicle": brand},
        flag_values = {str(Label("//tools/hardware:vehicle")): "legacy"},
    )
    native.config_setting(
        name = name + "_profile",
        flag_values = {str(Label("//tools/hardware:vehicle")): brand},
    )
    selects.config_setting_group(
        name = name,
        match_any = [":" + name + "_legacy", ":" + name + "_profile"],
    )

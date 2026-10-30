"""Compatibility helper for legacy driver installation targets."""

def if_jetson_orin(if_true, if_false = []):
    return select({
        Label("//tools/platform:is_jetson_orin"): if_true,
        "//conditions:default": if_false,
    })

load("@hardware.build//tools/install:install.bzl", "install")

package(default_visibility = ["//visibility:public"])

install(
    name = "install",
    data = [
        ":runtime_data",
    ],
    data_dest = "drivers/addition_data/lidar/velodyne",
    library_dest = "drivers/lib/lidar/velodyne",
    targets = [
        "//velodyne/compensator:libvelodyne_compensator_component.so",
        "//velodyne/driver:libvelodyne_driver_component.so",
        "//velodyne/fusion:libvelodyne_fusion_component.so",
        "//velodyne/parser:libvelodyne_convert_component.so",
    ],
)

filegroup(
    name = "runtime_data",
    srcs = glob([
        "conf/*.txt",
        "dag/*.dag",
        "launch/*.launch",
        "params/*.yaml",
    ]),
)

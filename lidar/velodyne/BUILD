load("@rules_pkg//pkg:install.bzl", "pkg_install")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files", "strip_prefix")

package(default_visibility = ["//visibility:public"])

pkg_files(
    name = "package_files_data",
    srcs = [":runtime_data"],
    attributes = pkg_attributes(mode = "0644"),
    prefix = "drivers/addition_data/lidar/velodyne",
    strip_prefix = strip_prefix.from_root("velodyne"),
)

pkg_files(
    name = "package_files_libraries",
    srcs = [
        "//velodyne/compensator:libvelodyne_compensator_component.so",
        "//velodyne/driver:libvelodyne_driver_component.so",
        "//velodyne/fusion:libvelodyne_fusion_component.so",
        "//velodyne/parser:libvelodyne_convert_component.so",
    ],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "drivers/lib/lidar/velodyne",
    strip_prefix = strip_prefix.from_pkg(),
)

pkg_filegroup(
    name = "package_files",
    srcs = [
        ":package_files_data",
        ":package_files_libraries",
    ],
)

pkg_install(
    name = "install_dir",
    srcs = [":package_files"],
    tags = ["manual"],
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

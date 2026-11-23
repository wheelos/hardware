# Architecture

## When

Read this when changing module boundaries, dependencies, or runtime assembly.

## Rules / Facts

- **Multi-module Monorepo**: The repository root (`pkgs/hardware/`) is a Git
  container, not a Bazel module. Each of the 13 top-level directories is an
  independent Bzlmod module:
  - Build/platform support: `hardware.build` (`build/`, Starlark-only).
  - Transport & CAN foundation: `hardware.hal` (`hal/`), `hardware.canbus` (`canbus/`).
  - Sensors & perception-input drivers: `hardware.camera` (`camera/`),
    `hardware.camera_gst` (`camera_gst/`), `hardware.image_decompress`
    (`image_decompress/`), `hardware.audio` (`audio/`), `hardware.video`
    (`video/`), `hardware.gnss` (`gnss/`), `hardware.lidar` (`lidar/`),
    `hardware.radar` (`radar/`).
  - Vehicle & equipment control: `hardware.vehicle` (`vehicle/`),
    `hardware.equipment` (`equipment/`).
- **Why `build/` is a subdirectory**: Keeping `hardware.build` under `build/`
  prevents root-workspace package collisions across the 12 functional modules,
  allows clean BCR `strip_prefix` source slicing (`hardware-<ver>/build`), and
  matches symmetric `--override_module` overrides (`../build`).
- **Dependency isolation**:
  - `hardware.build` has no dependency on `wheelos_core`, `wheelos_transform`,
    or any functional hardware module.
  - `hardware.vehicle` and `hardware.radar` depend on `hardware.canbus`.
  - `hardware.gnss` and `hardware.equipment` depend on `hardware.hal`.
  - `hardware.camera` does not resolve `hardware.camera_gst`,
    `hardware.image_decompress`, `hardware.hal`, `hardware.canbus`,
    `hardware.vehicle`, `hardware.lidar`, or `wheelos_transform`.
  - Driver SDKs are owned by the module that uses them (`camera_gst` owns
    GStreamer, `gnss` owns RTKLIB, `lidar` owns LiDAR vendor SDKs,
    `camera`/`video` own local FFmpeg extensions).
- **Separated transport boundaries (preserved)**: `hardware.hal` (`Stream` /
  `CanStream`) and `hardware.canbus` (`CanClient`) are intentionally separate
  contracts and are not merged.
- **Core-owned runtime assembly**: Every runtime-capable module exposes
  `//:module` via `@core//runtime:runtime.bzl` (`runtime_module` and
  `runtime_resources`) so consumers and product builds can assemble relocatable
  runtimes with `runtime_product`. Legacy `:package_files` / `:install_dir`
  targets remain for product packaging compatibility.

## Sources

- `README.md`
- `build/MODULE.bazel`, `build/tools/hardware/BUILD`, `build/tools/platform/BUILD`
- `*/MODULE.bazel`, `*/BUILD.bazel`

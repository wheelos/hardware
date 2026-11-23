# Copilot instructions for `wheelos/hardware`

## Repository paradigm

- This repository is a **multi-module Bzlmod Monorepo**, not a single umbrella
  Bazel workspace. Each top-level module directory (`audio/`, `build/`,
  `camera/`, `camera_gst/`, `canbus/`, `equipment/`, `gnss/`, `hal/`,
  `image_decompress/`, `lidar/`, `radar/`, `vehicle/`, `video/`) owns its own
  `MODULE.bazel`, `.bazelrc`, `.bazelrc.dev`, and `.bazelversion`.
- `build/` (`hardware.build`) is a Starlark-only support module for shared build
  flags and platform constraints. Keep it in `build/` and keep it free of C++
  sources and `CPPLINT.cfg`.
- Sources live directly under each module root without an extra
  `modules/drivers/...` physical hierarchy. Preserve legacy C++ `#include` and
  Proto `import` paths with Bazel `include_prefix` / `strip_include_prefix` and
  `import_prefix` / `strip_import_prefix`.

## Build, test, and validation

- Always run Bazel builds/tests in the managed dev container as the mapped
  non-root user with `--config=dev`.
- Structural consistency check:
  ```bash
  python3 scripts/validate_modules.py
  ```
- Isolated downstream SDK consumer smoke test (inside dev container):
  ```bash
  bash scripts/validate_downstream_consumer.sh
  ```
- Single-module build/test (from `<module>/` inside dev container, via `bazelisk`
  for Bazel 8.8.1):
  ```bash
  bazelisk test --config=dev //...
  ```
- Camera/Video modules use the container FFmpeg sysroot by default
  (`APOLLO_SYSROOT_DIR=/opt/apollo/sysroot`).

## Public module contract

- Align shared toolchain/core dependency versions across all `MODULE.bazel`
  files (`rules_cc 0.2.26`, `rules_pkg 1.3.0`, `platforms 1.1.0`,
  `bazel_skylib 1.9.2`, `protobuf 36.2`, `abseil-cpp 20260107.1`,
  `googletest 1.17.0.bcr.2`, `gflags 2.3.1`, `yaml-cpp 0.9.0.bcr.1`).
- Component modules export:
  - `//:component` (loadable `.so` component)
  - `//:module` (`@core//runtime:runtime.bzl` `runtime_module` for unified Core
    runtime assembly)
  - `//:package_files` & `//:install_dir` (`rules_pkg` / `@core//runtime:install.bzl`)
  - Explicit C++ SDK library targets (e.g. `:client`, `:image_decoder`,
    `//stream:stream_factory`, `//brands/<brand>:<brand>_vehicle_factory`) that
    can be consumed without forcing global `@hardware.build` profile flags.

## References

- Agent rules: `AGENTS.md`
- Durable knowledge: `.agents/knowledge/README.md`
- Task skills: `.agents/skills/README.md`
- Project docs: `docs/README.md`

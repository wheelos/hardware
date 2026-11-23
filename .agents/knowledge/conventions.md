# Conventions

## When

Read this when editing `MODULE.bazel`, `BUILD.bazel`/`BUILD`, `.bazelrc*`, or
adding new driver targets/brands.

## Rules / Facts

- **Aligned Bzlmod core/toolchain dependency matrix** (synchronized with
  `wheelos_core`, `apollo-lite`, and `equipment`):
  - `bazel_skylib`: `1.9.2`
  - `rules_cc`: `0.2.26`
  - `rules_pkg`: `1.3.0`
  - `platforms`: `1.1.0`
  - `protobuf`: `36.2` (`repo_name = "com_google_protobuf"`)
  - `abseil-cpp`: `20260107.1` (`repo_name = "com_google_absl"`)
  - `googletest`: `1.17.0.bcr.2` (`repo_name = "com_google_googletest"`)
  - `gflags`: `2.3.1` (`repo_name = "com_github_gflags_gflags"`)
  - `yaml-cpp`: `0.9.0.bcr.1` (`repo_name = "com_github_jbeder_yaml_cpp"`)
  - `wheelos_core`: `1.0.6` (`repo_name = "core"`), `wheelos_common`: `0.1.3`,
    `wheelos_msgs`: `0.1.6`, `wheelos_transform`: `0.1.0`.
- **Module workspace scaffolding**:
  - Every module directory carries `.bazelversion` (`8.8.1`), `.bazelrc`, and
    `.bazelrc.dev` (with relative `--override_module` for sibling hardware
    dependencies and `../../core`, `../../common`, `../../wheelos_msgs`,
    `../../transform` as needed).
  - C++ modules carry `CPPLINT.cfg`; Starlark-only `build/` intentionally omits
    `CPPLINT.cfg`.
- **Shallow physical layout + logical include/import prefixes**:
  - Keep sources at the module root or meaningful domain subdirs (`brands/`,
    `parser/`, `stream/`, etc.) without `modules/drivers/...` nesting.
  - Preserve legacy C++ `#include` paths using `include_prefix` and
    `strip_include_prefix`, and Proto imports using `import_prefix` and
    `strip_import_prefix`.
- **Standard public target contract**:
  - Component modules export `//:component` (loadable `.so`), `//:module`
    (`runtime_module`), `//:runtime_resources` (or subpackage `:module`), and
    `//:package_files` / `//:install_dir`.
  - Multi-vendor modules (`lidar`, `radar`) expose root aliases (`:hesai`,
    `:livox`, `:conti`, `:nano`, etc.) plus per-brand `//<brand>:module` targets
    so external consumers can depend on a specific brand without configuring
    global `@hardware.build` flags.
  - SDK library modules (`hal`, `canbus`, `equipment`, `image_decompress`,
    `vehicle/brands/*`) expose explicit `cc_library` targets (`:stream_factory`,
    `:client`, `:image_decoder`, `//brands/devkit:devkit_vehicle_factory`)
    usable without global profile flags.
- **Standalone vs. aggregated DAG paths**:
  - Modules that contribute resources directly via their own `//:module`
    (such as `equipment/dag/equipment.dag`) use `lib/components/<mod>/...` and
    `conf/<mod>/...`.
  - Modules currently assembled under Apollo-Lite `//tools/hardware`
    (`module_name = "hardware"`) keep `:runtime_data` / `:profile_runtime_data`
    with `conf/hardware/<mod>/...` until product migration completes.

## Sources

- `*/MODULE.bazel`
- `*/BUILD.bazel`
- `scripts/validate_modules.py`

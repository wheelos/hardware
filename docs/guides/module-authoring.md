# Module Authoring Guide

Follow these conventions when adding a new hardware module or vendor driver.

## 1. Directory and Workspace Files

Every top-level module directory `<mod>/` must provide:

- `MODULE.bazel`: `name = "hardware.<mod>"` using the aligned dependency
  versions checked by `scripts/validate_modules.py`.
- `BUILD.bazel`: public targets (`:component`, `:module`, `:package_files`,
  `:install_dir`, and explicit C++ SDK libraries where applicable).
- `.bazelversion` (`8.8.1`), `.bazelrc`, `.bazelrc.dev`, and `CPPLINT.cfg`
  (omitted only for Starlark-only `build/`).

## 2. Logical Include and Proto Prefixes

Place C++ and `.proto` files directly in shallow module subdirectories and
preserve canonical include/import paths with Bazel native attributes:

```starlark
cc_library(
    name = "my_driver_lib",
    srcs = ["my_driver.cc"],
    hdrs = ["my_driver.h"],
    include_prefix = "modules/drivers/my_driver",
    strip_include_prefix = "/",
    ...
)
```

## 3. Standard Exports

- **Component library & shared object**: `lib<name>_component.so` + alias
  `name = "component"`.
- **Unified Core runtime**: `runtime_resources` + `runtime_module(name = "module", ...)`.
- **Legacy package files**: `pkg_files` + `pkg_filegroup(name = "package_files")`
  + `pkg_install(name = "install_dir")`.
- **Driver-specific SDKs**: Keep third-party wrappers inside `<mod>/third_party/`
  or declare them only in `<mod>/MODULE.bazel`.

## 4. Validate Before Submitting

```bash
python3 scripts/validate_modules.py
# Inside managed dev container, from <mod>/:
bazelisk test --config=dev //...
```

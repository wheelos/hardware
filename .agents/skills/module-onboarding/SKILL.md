---
name: module-onboarding
description: Add a new hardware.* Bzlmod module or a new brand/device target while preserving dependency isolation and unified SDK conventions.
---

# Module onboarding

## Checklist

1. Create `<module>/` at repository root with shallow layout (no `modules/drivers/` nesting).
2. Add `MODULE.bazel` using the aligned dependency matrix in
   `.agents/knowledge/conventions.md` (`rules_cc 0.2.26`, `protobuf 36.2`,
   `googletest 1.17.0.bcr.2`, `wheelos_core 1.0.6`, etc.). Declare only minimal
   required `bazel_dep`s; keep vendor SDK extensions inside `<module>/third_party/`.
3. Add `.bazelversion` (`8.8.1`), `.bazelrc`, `.bazelrc.dev`, and `CPPLINT.cfg`.
4. In `BUILD.bazel`:
   - Set `include_prefix` / `strip_include_prefix` (and `import_prefix` /
     `strip_import_prefix` in `proto/BUILD`) to preserve logical include paths.
   - Export public C++ SDK libraries, `//:component` (`.so`), `//:runtime_resources`,
     `//:module` (`@core//runtime:runtime.bzl`), and `//:package_files` /
     `//:install_dir`.
5. Ensure DAGs consumed via `//:module` use `lib/components/<module>/...` and
   `conf/<module>/...` rather than legacy `drivers/lib/...` or `/apollo/...` paths.
6. Run `python3 scripts/validate_modules.py` and `bazelisk test --config=dev //...`.

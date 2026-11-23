---
name: review
description: Review changes in wheelos/hardware for module boundary leaks, build-matrix drift, public target regressions, and runtime path mismatches.
---

# Review

## Checklist

1. **Boundary & dependency isolation**: Verify `MODULE.bazel` edits do not pull
   heavy third-party SDKs, `wheelos_transform`, or sibling hardware modules into
   unrelated consumers.
2. **Version alignment**: Verify shared deps (`rules_cc`, `protobuf`, `abseil-cpp`,
   `googletest`, `gflags`, `yaml-cpp`, `platforms`, `bazel_skylib`, `rules_pkg`)
   match `.agents/knowledge/conventions.md`.
3. **Build & runtime exports**: Check that `//:component`, `//:module`,
   `//:package_files`, and brand/SDK aliases remain intact and tested.
4. **Path contracts**: Verify DAGs contain software-root-relative paths (no
   `/apollo/...` or `drivers/lib/...` in standalone `//:module` DAGs) and C++
   headers keep `include_prefix` stability.
5. **Validation evidence**: Require `python3 scripts/validate_modules.py` and
   relevant container `bazelisk test --config=dev` results.

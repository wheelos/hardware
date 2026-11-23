## Change and ownership

- Affected module(s) (`hardware.<name>`):
- Public API / Bazel target / `MODULE.bazel` impact:

## Checklist

- [ ] `python3 scripts/validate_modules.py` passes.
- [ ] Affected module(s) build and pass `bazelisk test --config=dev //...` in the
      managed container.
- [ ] Dependency isolation preserved (no unintended heavy SDKs or
      `wheelos_transform` pulled into unrelated modules).
- [ ] Public exports (`:component`, `:module`, `:package_files`, C++ SDK libs)
      and documentation (`docs/`, `README.md`) remain synchronized.
- [ ] If releasing to BCR, source tag and `strip_prefix` topology are verified
      without `--override_module`.

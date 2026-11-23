---
name: build-and-test
description: Validate multi-module structure, run module Bazel builds/tests in the managed dev container, and run isolated downstream consumer verification.
---

# Build and test

## Steps

1. Run the fast structural & version-alignment check (works on host or container):
   ```bash
   python3 scripts/validate_modules.py
   ```
2. Run Bazel 8.8.1 (`bazelisk`) builds and tests inside the managed dev
   container as the mapped non-root user (never build on the host):
   ```bash
   # Single module test (from /apollo/pkgs/hardware/<module>)
   bazelisk test --config=dev //...
   ```
3. For FFmpeg-backed modules (`camera`, `video`), rely on the container sysroot
   (`/opt/apollo/sysroot`, default in extension) or pass `--repo_env=APOLLO_SYSROOT_DIR=/opt/apollo/sysroot`.
4. Verify external SDK consumption and dependency isolation:
   ```bash
   bash scripts/validate_downstream_consumer.sh
   ```
5. When integrated inside `apollo-lite`, also verify the product hardware profile
   suite from `/apollo`:
   ```bash
   python3 scripts/bzlmod.py dev test --config=cpu --config=opt --config=hw_x86_devkit //tools/hardware:tests
   ```

# BCR Release and Source Packaging Guide

`wheelos/hardware` is published as **independent Bzlmod source modules** in
the Bazel Central Registry (`bcr.wheelos.cn`), backed by a **single immutable
Git release archive**.

## Why Single Git Repo + Per-Module BCR `strip_prefix`

- Consumers declare `bazel_dep(name = "hardware.camera")` without downloading or
  configuring all 13 modules.
- Maintainers keep one Git repository (`wheelos/hardware`) and cut coordinated
  releases without splitting into 13 separate Git repositories.
- Because `hardware.build` lives under `build/`, its BCR archive slice
  (`strip_prefix: "hardware-<ver>/build"`) never includes the functional
  driver directories.

## Release Workflow

1. **Pre-release validation**:
   ```bash
   python3 scripts/validate_modules.py
   bash scripts/validate_downstream_consumer.sh
   ```
2. **Verify upstream dependencies**: Ensure the exact versions declared in each
   `MODULE.bazel` (`wheelos_core`, `wheelos_common`, `wheelos_msgs`,
   `wheelos_transform`, `rules_cc`, `protobuf`, `abseil-cpp`, `googletest`, etc.)
   are already published in the target BCR.
3. **Tag the source revision**: Create an immutable Git tag (for example `v0.1.0`)
   so GitHub serves `https://github.com/wheelos/hardware/archive/refs/tags/v0.1.0.tar.gz`.
4. **Publish BCR metadata in topological order**:
   - **Stage 1 (Foundations)**: `hardware.build`, `hardware.hal`
   - **Stage 2 (Mid-layer)**: `hardware.canbus`
   - **Stage 3 (Drivers & Equipment)**: `hardware.audio`, `hardware.camera`,
     `hardware.camera_gst`, `hardware.equipment`, `hardware.gnss`,
     `hardware.image_decompress`, `hardware.lidar`, `hardware.radar`,
     `hardware.vehicle`, `hardware.video`

   Each BCR `source.json` points to the same tarball with its own `strip_prefix`:
   ```json
   {
     "url": "https://github.com/wheelos/hardware/archive/refs/tags/v0.1.0.tar.gz",
     "integrity": "sha256-...",
     "strip_prefix": "hardware-0.1.0/camera"
   }
   ```
5. **Verify registry-only builds**: Test a clean downstream consumer against the
   registry **without** `--override_module` or `--lockfile_mode=off`.
6. **Offline / air-gapped delivery**: Use `bazel vendor` in the consumer or
   product workspace to generate a self-contained vendor archive instead of
   copying raw checkout directories.

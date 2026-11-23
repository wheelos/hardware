# BCR Publishing

## When

Read this when preparing `hardware.*` modules for Bazel Central Registry
(`bcr.wheelos.cn`) publication or validating registry builds.

## Rules / Facts

- **Single Git release, multiple BCR entries**:
  - All 13 modules share one immutable Git tag/archive from
    `https://github.com/wheelos/hardware/archive/refs/tags/<tag>.tar.gz`.
  - Each module (`hardware.build`, `hardware.hal`, ..., `hardware.equipment`)
    has its own BCR module directory whose `source.json` points to that archive
    with `"strip_prefix": "hardware-<ver>/<dir>"`.
- **Publish in topological dependency order**:
  1. Upstream foundations: `wheelos_msgs`, `wheelos_core`, `wheelos_common`,
     `wheelos_transform`.
  2. Hardware leaves: `hardware.build`, `hardware.hal`.
  3. Hardware mid-layer: `hardware.canbus`.
  4. Hardware leaf-consumers: `hardware.audio`, `hardware.camera`,
     `hardware.camera_gst`, `hardware.equipment`, `hardware.gnss`,
     `hardware.image_decompress`, `hardware.lidar`, `hardware.radar`,
     `hardware.vehicle`, `hardware.video`.
- **Verification**: Validate downstream builds against the registry **without**
  `--override_module` or `--lockfile_mode=off` before declaring a release
  complete.

## Sources

- `README.md`
- `docs/guides/bcr-release.md`
- `scripts/validate_downstream_consumer.sh`

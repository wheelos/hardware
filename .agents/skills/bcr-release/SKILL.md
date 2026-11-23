---
name: bcr-release
description: Publish wheelos/hardware modules to the Bazel Central Registry using a single Git release archive and per-module strip_prefix entries.
---

# BCR release

## Steps

1. Confirm all `MODULE.bazel` versions and dependency versions pass
   `python3 scripts/validate_modules.py`.
2. Ensure upstream `wheelos_msgs`, `wheelos_core`, `wheelos_common`, and
   `wheelos_transform` versions declared in `MODULE.bazel` are already published
   in the target registry.
3. Create and verify an immutable Git tag (for example `v0.1.0`) on `wheelos/hardware`.
4. Register BCR module directories in topological order:
   - Wave 1: `hardware.build`, `hardware.hal`
   - Wave 2: `hardware.canbus`
   - Wave 3: `hardware.audio`, `hardware.camera`, `hardware.camera_gst`,
     `hardware.equipment`, `hardware.gnss`, `hardware.image_decompress`,
     `hardware.lidar`, `hardware.radar`, `hardware.vehicle`, `hardware.video`
   Each `source.json` points to the same Git tag archive with
   `"strip_prefix": "hardware-<ver>/<module_dir>"`.
5. Validate downstream consumer builds against the registry **without**
   `--override_module`.

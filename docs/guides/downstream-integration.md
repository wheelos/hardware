# Downstream SDK Integration

External projects can consume `wheelos/hardware` selectively via Bzlmod without
cloning the full repository or pulling unrelated drivers.

## 1. Select Only Needed Modules in `MODULE.bazel`

### Example A: Camera + Image Decoder Only

```starlark
module(name = "my_camera_app", version = "0.1.0")

bazel_dep(name = "wheelos_core", version = "1.0.6", repo_name = "core")
bazel_dep(name = "hardware.camera", version = "0.1.0")
bazel_dep(name = "hardware.image_decompress", version = "0.1.0")
```

This resolves `hardware.camera`, `hardware.image_decompress`, and `hardware.build`
without resolving `hardware.lidar`, `hardware.vehicle`, `hardware.canbus`, or
`wheelos_transform`.

### Example B: Equipment Control Client + Vehicle Brand SDK

```starlark
bazel_dep(name = "wheelos_core", version = "1.0.6", repo_name = "core")
bazel_dep(name = "hardware.equipment", version = "0.1.0")
bazel_dep(name = "hardware.vehicle", version = "0.1.0")
```

## 2. Choose Your Consumption Surface

1. **C++ SDK Libraries** (compile & link directly, no global `@hardware.build`
   profile flags required):
   - `@hardware.hal//:stream_factory`, `@hardware.hal//:can_stream`
   - `@hardware.canbus//:client`, `@hardware.canbus//:factory`
   - `@hardware.equipment//:client`, `@hardware.equipment//:charger`
   - `@hardware.image_decompress//:image_decoder`
   - `@hardware.vehicle//brands/devkit:devkit_vehicle_factory` (or other brand)
2. **Cyber RT Loadable Components** (`.so` plugins):
   - `@hardware.camera//:component`
   - `@hardware.equipment//:component`
   - `@hardware.lidar//:hesai` (or `:livox`, `:rslidar`, `:velodyne`, etc.)
   - `@hardware.radar//:conti` (or `:nano`, `:yg`, etc.)
3. **Unified Core Runtime Assembly** (`runtime_product` / `runtime_module`):
   ```starlark
   load("@core//runtime:product.bzl", "runtime_product")

   runtime_product(
       name = "my_robot_runtime",
       modules = [
           "@hardware.equipment//:module",
           "@hardware.lidar//hesai:module",
           "@hardware.radar//conti_radar:module",
       ],
       default_dag = "dag/equipment/equipment.dag",
   )
   ```
   - Per-brand targets like `@hardware.lidar//hesai:module` and
     `@hardware.radar//conti_radar:module` work without setting `@hardware.build`
     profile flags.
   - Aggregate targets (`@hardware.lidar//:module`, `@hardware.vehicle//:component`)
     honor `@hardware.build//tools/hardware:*` flags when profile-based pruning
     is desired.

## 3. Local Unreleased Development (`--override_module`)

When testing against a local checkout before BCR publication, configure your
`.bazelrc.dev` (or command line) with explicit `--override_module` entries for
each consumed `hardware.*` module and its transitive local dependencies:

```text
common:dev --lockfile_mode=off
common:dev --override_module=hardware.build=/path/to/hardware/build
common:dev --override_module=hardware.hal=/path/to/hardware/hal
common:dev --override_module=hardware.equipment=/path/to/hardware/equipment
```

Run `bash scripts/validate_downstream_consumer.sh` inside the dev container to
verify isolated downstream resolution and build behavior.

# WheelOS Hardware SDK Architecture

`wheelos/hardware` is the unified autonomous-driving hardware integration and
device-protocol SDK for WheelOS / Cyber RT. It is structured as a **multi-module
Bzlmod Monorepo** aligned with `wheelos_core` and `apollo-lite`.

## Design Principles

1. **Minimal-dependency Bzlmod modularity**: Each functional domain is an
   independent Bzlmod root (`hardware.<name>`). External consumers declare only
   the modules they use without pulling unrelated vendor SDKs or algorithm
   dependencies (`wheelos_transform`, LiDAR SDKs, GStreamer, etc.).
2. **Shallow physical layout with stable logical contracts**: Source files live
   directly under each module root without deep `modules/drivers/...` nesting.
   Bazel `include_prefix`/`strip_include_prefix` and `import_prefix` preserve
   existing `#include` and `.proto` import paths.
3. **Core-owned runtime assembly**: Runtime-capable modules export `//:module`
   using `@core//runtime:runtime.bzl` (`runtime_module` and
   `runtime_resources`), enabling downstream products to assemble relocatable
   bundles via `runtime_product`.
4. **Owned third-party driver SDKs**: Hardware-only SDKs live inside the
   owning module (`camera_gst/third_party/gstreamer`, `gnss/third_party/rtklib`,
   `camera/third_party/ffmpeg`, `lidar` BCR SDK dependencies).

## Module Catalog

| Module | Directory | Role & Primary Exports |
| --- | --- | --- |
| `hardware.build` | `build/` | Starlark-only build settings and platform constraints (`@hardware.build//tools/hardware`, `@hardware.build//tools/platform`). Kept in `build/` to isolate from functional module roots. |
| `hardware.hal` | `hal/` | Serial, TCP, UDP, NTRIP, and CAN stream abstractions (`:stream`, `:stream_factory`, `:can_stream`). |
| `hardware.canbus` | `canbus/` | Frame-oriented CAN clients, sender/receiver, and CLI tool (`:client`, `:factory`, `:sensor_canbus`, `:tool`, `:module`). |
| `hardware.audio` | `audio/` | Microphone driver component (`:component`, `:module`). |
| `hardware.camera` | `camera/` | V4L2 camera and compression component (`:component`, `:module`). |
| `hardware.camera_gst` | `camera_gst/` | Jetson Orin GStreamer camera backend (`:component`, `:frame_extractor`, `:module`). |
| `hardware.image_decompress` | `image_decompress/` | Standalone image decompression library and component (`:image_decoder`, `:component`, `:module`). |
| `hardware.video` | `video/` | H.265 video stream driver (`:component`, `:module`). |
| `hardware.gnss` | `gnss/` | GNSS stream/parser drivers and RTKLIB (`:component`, `:module`). |
| `hardware.lidar` | `lidar/` | Multi-vendor LiDAR drivers & processor (`:component`, `:hesai`, `:livox`, `:lslidar`, `:rslidar`, `:seyond`, `:vanjeelidar`, `:velodyne`, `:processor`, `:compensator`, `:fusion`, `//<brand>:module`, `:module`). |
| `hardware.radar` | `radar/` | Radar CAN drivers (`:component`, `:conti`, `:nano`, `:racobit`, `:udas_ultrasonic`, `:ultrasonic`, `:yg`, `//<brand>:module`, `:module`). |
| `hardware.vehicle` | `vehicle/` | Vehicle chassis component, factory, and brand protocols (`:component`, `:module`, `//brands/<brand>:<brand>_vehicle_factory`). |
| `hardware.equipment` | `equipment/` | Device-first equipment control RPC service & typed clients on `hardware.hal` (`:component`, `:client`, `:charger`, `:module`). |

> **Transport contract note**: `hardware.hal` (`Stream` / `CanStream`) and
> `hardware.canbus` (`CanClient`) serve distinct contracts and remain separate.

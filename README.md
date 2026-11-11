# WheelOS Hardware

One Git checkout contains independent Bzlmod modules, not aliases of one
umbrella module. Each directory below is a module root with its own
`MODULE.bazel` and direct dependency declarations.

| Module/root | Responsibility | Public example |
| --- | --- | --- |
| `hardware.camera` / `camera/` | V4L2 camera and compression | `@hardware.camera//:component` |
| `hardware.camera_gst` / `camera_gst/` | GStreamer/Orin camera backend | `@hardware.camera_gst//:component` |
| `hardware.image_decompress` / `image_decompress/` | Image-message decoder/component, independent of camera devices | `@hardware.image_decompress//:component`, `:image_decoder` |
| `hardware.canbus` / `canbus/` | CAN transport, clients and communication primitives | `@hardware.canbus//:client`, `:factory`, `:tool` |
| `hardware.vehicle` / `vehicle/` | Vehicle component, factory and brand protocols | `@hardware.vehicle//:component` |
| `hardware.equipment` / `equipment/` | Independent device capabilities and RPC operation lifecycle on HAL Streams | `@hardware.equipment//:component`, `:client` |
| `hardware.lidar` / `lidar/` | LiDAR implementations and SDK adapters | `@hardware.lidar//:hesai` |
| `hardware.radar` / `radar/` | Radar implementations | `@hardware.radar//:conti` |
| `hardware.gnss` / `gnss/` | GNSS streams, parsers and RTKLib | `@hardware.gnss//:component` |
| `hardware.hal` / `hal/` | Existing serial/TCP/UDP/NTRIP/CAN stream abstraction | `@hardware.hal//stream:stream_factory` |
| `hardware.audio` / `audio/` | Microphone | `@hardware.audio//:component` |
| `hardware.video` / `video/` | Video driver | `@hardware.video//:component` |
| `hardware.build` / `build/` | Shared rules, settings, installation provider/templates; no runtime driver code | `@hardware.build//tools/hardware:vehicle` |

Vehicle and CAN-based radar depend on `hardware.canbus`. GNSS uses the shared
`hardware.hal` module. Shared helpers/settings do not depend on functional
hardware modules. Camera does not resolve GStreamer, image_decompress, HAL,
vehicle, CAN, GNSS or LiDAR SDK modules.
The product may declare all modules for its full inventory; that is distinct
from an independent consumer declaring only `hardware.camera`.

Driver SDK and system-library adapters belong to the hardware module that uses
them: `hardware.camera_gst` owns the GStreamer header/library wrapper under
`camera_gst/third_party/gstreamer`, and `hardware.gnss` owns RTKLIB under
`gnss/third_party/rtklib`. The root product repository does not carry a second
copy of these hardware-only third-party packages.

```starlark
bazel_dep(name = "hardware.camera", version = "0.1.0")
bazel_dep(name = "hardware.vehicle", version = "0.1.0")
bazel_dep(name = "hardware.image_decompress", version = "0.1.0")
bazel_dep(name = "hardware.hal", version = "0.1.0")
```

Source files live directly in their module roots, not another copy of the old
product directory hierarchy:

```text
hardware/
  audio/microphone_component.cc
  camera/camera_component.cc
  camera/backend/
  camera_gst/camera_gst_component.cc
  canbus/can_client/
  canbus/can_comm/
  equipment/devices/
  equipment/protocols/can_signal/
  gnss/parser/
  hal/stream/
  image_decompress/image_decompress.cc
  lidar/hesai/
  radar/conti_radar/
  vehicle/canbus_component.cc
  vehicle/brands/devkit/
  video/video_driver_component.cc
  build/tools/
```

Each top-level directory has its own `MODULE.bazel` and `BUILD.bazel`.
No extra `src/` layer is needed for these existing small module roots.
Existing implementation subdirectories remain where they express a meaningful
boundary. `audio` is the capability and microphone its implementation;
`camera_gst` names the backend; `vehicle/brands` avoids `vehicle/vehicle`.
HAL is specifically the existing stream layer, not a new universal hardware
framework. `image_decompress` is a runtime component/library, not an executable
maintenance tool, so it no longer belongs under `tools/`.

Bazel BUILD labels use the new shallow packages. C++ include paths and Proto
import names retain their existing logical identity using native
`include_prefix`/`strip_include_prefix` and
`import_prefix`/`strip_import_prefix` attributes. This preserves current
consumers without keeping deep physical directories or source symlinks.
Vehicle messages and component classes are not renamed.
The product separately maps built files back to stable DAG/deployment paths;
source layout and runtime bundle layout are distinct contracts.

`build/` replaces the ambiguous `common/` directory. It is intentionally a
Starlark-only support module: it owns shared settings and platform constraints.
It has no
dependency on transform, any functional hardware module, or the product.
Camera-only consumers therefore do not resolve `wheelos_transform`.

The product's runtime installation uses Bazel-maintained `rules_pkg`:
`pkg_install` installs a directory explicitly and `pkg_tar` produces an archive.
Modules declare `pkg_files`/`pkg_filegroup` mappings and explicit `install_dir`
targets directly from `rules_pkg`, while product runtime assembly uses Core's
`//runtime` rules to validate and materialize `bin/`, `lib/`, `conf/`, `dag/`,
and `launch/` layouts with software-root-relative references. The legacy
`install()` interface, custom provider, templates and forwarding layers have
been removed. Runtime dependency collection belongs to product runtime assembly,
not module SDK file mappings.
C++ lint is an explicit command/review operation, not a Bazel build target;
consumers retain their own lint configuration.

The repository is a WheelOS/Cyber hardware-integration and device-protocol
library, not a middleware-independent HAL or a complete driving product.
Generic driver examples and sensor-level decoding/filtering/deskew/fusion fit
here; product sensor inventory, site calibration, startup orchestration,
whole-vehicle safety decisions and release assembly belong to the consumer.
The existing HAL CAN stream and frame-oriented CAN client have different
contracts and are not merged as part of this ownership cleanup.

These are standard Bazel mechanisms, not a claim that one directory convention
is mandatory for all C++ projects:
[C++ include paths](https://bazel.build/reference/be/c-cpp#cc_library.include_prefix)
and [Proto import paths](https://bazel.build/reference/be/protocol-buffer#proto_library.import_prefix).

Profiles use one shared settings identity under
`@hardware.build//tools/hardware`; helpers anchor labels to their defining
module, not the calling BUILD's repository. Product runtime layout and binary
packaging remain in Apollo-Lite `//tools/hardware`. Building its `:hardware`
target does not package or install; `:package` archives the same runtime.

Source is maintained at `git@github.com:wheelos/hardware.git`.
Local development uses the product's `.bazelrc.dev` with one override per
module root. The product's `wheelos.repos` imports this checkout into
`pkgs/hardware`; `pkgs/` remains ignored by the product repository.
Source publication does not register module versions in the Bazel registry.

Publication requires an authorized immutable source revision and separate
registry entries for each module, with source archives rooted at the matching
directory (for example via `strip_prefix`). They may share one Git release
initially; independent modules do not require independent Git repositories or
release cadences. Publish shared dependencies before consumers and validate
registry builds without local overrides. No registry metadata or release
revision is fabricated here.

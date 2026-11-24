# Driver Benchmark Profiler (`hardware.benchmark`)

`hardware.benchmark` provides `wheelos-sensor-bench`, an isolated CLI, OS/kernel
sidecar collector, Cyber RT channel latency/throughput probe, and `LD_PRELOAD`
copy-amplification probe for benchmarking sensor drivers and comparing hardware
candidates objectively.

## 1. Profile a Single Sensor Driver

Inside the dev container or an installed Core runtime:

```bash
bazelisk run --config=dev //:wheelos-sensor-bench -- run \
  --device hesai_at128p \
  --dag /path/to/hesai_at128p_pcap.dag \
  --channel /apollo/sensor/hesai/AT128P/PointCloud2 \
  --msg-type pointcloud \
  --duration 60s \
  --warmup 10s \
  --bind-cores 4-7 \
  --nic eth0 \
  --out /tmp/bench/hesai_at128p
```

You can also target any custom driver binary (`--cmd "..."`) or attach to an
already running driver process (`--pid <pid>`).

Use the [report template](../benchmarks/report-template.md) to record the
hardware, software versions, workload, commands, results, and measurement
limitations. See the [VanJee 720-16 baseline](../benchmarks/vanjee-720-16-orin-2026-11-24.md)
for an actual device-rate example.

### VanJee 720-16 live 10 Hz baseline

Use the managed container as the mapped user. Use a fresh output directory and
a free host UDP port. Check the addresses, port ownership and model before
running; the command starts a driver and the SDK sends a read-only calibration
query. It does not configure the device. On ARM, the extra Ninja toolchain flag
below resolves the toolchain using the existing Bazel cache.

```bash
cd /apollo/pkgs/hardware/lidar
bazelisk build --config=dev -c opt \
  --extra_toolchains=@@rules_foreign_cc+//toolchains:built_ninja_toolchain \
  //vanjeelidar:libvanjeelidar_component.so @core//cyber/mainboard:mainboard
lidar_bin="$(bazelisk info --config=dev -c opt bazel-bin)"

# Build PROJ data matching the linked library if the container sysroot is incompatible.
cd /apollo/pkgs/hardware/gnss
bazelisk build --config=dev \
  --extra_toolchains=@@rules_foreign_cc+//toolchains:built_ninja_toolchain \
  @proj//:proj_runtime_data
export PROJ_DATA="$(bazelisk info --config=dev bazel-bin)/external/proj+/data"
export CYBER_DOMAIN_ID=183 CYBER_IP=127.0.0.1 CYBER_PATH=/apollo/pkgs/core/cyber

cd /apollo/pkgs/hardware/benchmark
bazelisk run --config=dev \
  --extra_toolchains=@@rules_foreign_cc+//toolchains:built_ninja_toolchain \
  //:vanjee-10hz-bench -- \
  --mainboard "$lidar_bin/external/wheelos_core+/cyber/mainboard/mainboard" \
  --component "$lidar_bin/vanjeelidar/libvanjeelidar_component.so" \
  --lidar-address 192.168.10.6 --host-address 192.168.10.3 \
  --interface eth1 --out /tmp/bench/vanjee-live \
  --driver-version "$(git -C /apollo/pkgs/hardware rev-parse HEAD)" \
  --warmup 20s --duration 30m --no-perf
```

The default 720-16 configuration enables IMU and uses the host clock; IMU output
and hardware clock synchronization are not verified. The measured process
includes mainboard and Cyber runtime, not the separate probes or system network
processing. Shared NIC counters include unrelated traffic.
The report records the OS visible to the process (the container OS when
containerized), the host kernel and, when available, the hardware board model.
Pass the actual driver revision with `--driver-version` and record host OS,
dependency overrides, build flags and dirty source state separately.

The optional passive packet observer requires `CAP_NET_RAW`. Without it the
result is N/A; do not grant capabilities globally. An authorized administrator
can instead run the following observer in a separate container process before
starting the mapped-user benchmark (the three paths must be fresh and visible
to both processes):

```bash
python3 /apollo/pkgs/hardware/benchmark/cli/vanjee_packets.py \
  --interface eth1 --source 192.168.10.6 --destination 192.168.10.3 \
  --window-file /tmp/vanjee-window --out /tmp/vanjee-packets.json \
  --ready-file /tmp/vanjee-observer-ready --timeout 1900
```

Once ready, add `--window-file /tmp/vanjee-window --packet-result
/tmp/vanjee-packets.json` to the benchmark command. Capture is non-promiscuous
and filters the selected IPv4 UDP flow and 720 single-return packet format.
Sequence gaps are estimates at the observer, not proof of driver socket loss;
observer socket drops span its entire lifetime, including warmup. Investigate
reordering or capture drops before interpreting gaps as network loss.

The callback-entry latency is from the driver's published header to subscriber
callback entry and includes Cyber transport and deserialization; it is not
network or hardware-to-driver latency. Legacy reports without the timestamp
marker are not scored for latency. Large-copy counts are limited to intercepted
`memcpy`/`memmove` calls of at least 4 KiB in the measurement window, whereas
total call/byte and heap delta counters cover the whole process lifetime.
Inline and other copies are invisible, and a configured transport mode is not
proof that transport or end-to-end zero-copy occurred.

## 2. Metrics Collected

- **CPU**: Core-normalized process CPU%, per-thread breakdown, user/sys split,
  and per-core `NET_RX`/`TIMER`/`SCHED` SoftIRQ deltas.
- **Memory & Zero-Copy**:
  - Steady-state RSS/PSS, RSS drift slope, minor/major page faults.
  - Optional `perf stat` `cache-misses` / `LLC-load-misses` (auto-degrades to
    `N/A` in containers without `CAP_PERFMON`).
  - Window-aligned intercepted large-copy amplification
    (`large_memcpy_bytes / app_payload_bytes`) and configured Cyber transport
    mode (`SHM` / `HYBRID` / `RTPS` / `INTRA` / `ICEORYX`); neither establishes
    end-to-end zero-copy.
- **I/O & Latency**:
  - NIC peak/mean RX Mbps, packet drops, and `/proc/net/udp` `RcvbufErrors`.
  - Segmented latency percentiles (P50/P95/P99/Max): `T_driver_to_app_ms`
    (host transport/serialization) and `T_hw_to_driver_ms` (when clocks are
    synchronized).

## 3. Multi-Candidate Selection Report

```bash
bazelisk run --config=dev //:wheelos-sensor-bench -- compare \
  /tmp/bench/hesai_at128p/baseline.json \
  /tmp/bench/rslidar_m1/baseline.json \
  /tmp/bench/seyond_falcon/baseline.json \
  --profile x86_ipc \
  --out /tmp/bench/lidar_selection.html
```

Outputs `baseline.json`, `report.md`, and an offline self-contained
`report.html` with normalized compute-cost scores (0-100), SVG charts, and
red-line bottleneck alerts.

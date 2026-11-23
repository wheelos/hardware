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

## 2. Metrics Collected

- **CPU**: Core-normalized process CPU%, per-thread breakdown, user/sys split,
  and per-core `NET_RX`/`TIMER`/`SCHED` SoftIRQ deltas.
- **Memory & Zero-Copy**:
  - Steady-state RSS/PSS, RSS drift slope, minor/major page faults.
  - Optional `perf stat` `cache-misses` / `LLC-load-misses` (auto-degrades to
    `N/A` in containers without `CAP_PERFMON`).
  - Two-stage zero-copy check: `memcpy`/`memmove` copy-amplification ratio
    (`large_memcpy_bytes / app_payload_bytes`) and Cyber transport mode
    (`SHM` / `HYBRID` / `RTPS` / `INTRA` / `ICEORYX`).
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

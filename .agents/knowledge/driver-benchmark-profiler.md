# Driver Benchmark Profiler

## When

Read this when extending `hardware.benchmark` (`benchmark/`), interpreting
sensor driver benchmark metrics, or validating zero-copy and latency baselines.

## Rules / Facts

- **Module isolation (`hardware.benchmark`)**: The profiler lives in its own
  Bzlmod root (`benchmark/`) and must remain optional. Functional driver
  modules (`camera`, `lidar`, `radar`, etc.) must **not** depend on
  `hardware.benchmark`.
- **Two-stage zero-copy verification**:
  1. *Device -> Driver payload stage*: Measured via `libwheelos_alloc_probe.so`
     (`LD_PRELOAD` interposing `memcpy`/`memmove` >= 4 KiB and `mallinfo2` heap
     deltas) to compute the **copy amplification ratio**
     (`large_memcpy_bytes / app_payload_bytes`).
  2. *Driver -> Cyber RT transport stage*: Measured by `cyber_channel_probe`,
     reporting configured Cyber `communication_mode` (`SHM` / `HYBRID` /
     `RTPS` / `INTRA` / `ICEORYX`) and shared-memory segment activity.
- **Separated timestamp domains**:
  - `T_driver_to_app_ms`: `T_recv_unix_ns - header.timestamp_sec * 1e9`
    (host-local transport/serialization/callback latency; valid without
    external PTP sync).
  - `T_hw_to_driver_ms`: `header.timestamp_sec * 1e9 - measurement_time_ns`
    (meaningful only when sensor and host clocks are synchronized, e.g.
    PTP/gPTP or host `CLOCK_MONOTONIC`/`REALTIME`).
- **OS & Kernel metrics**:
  - CPU & threads: `/proc/<pid>/stat` and `/proc/<pid>/task/*/stat` (`utime`,
    `stime`, core-normalized utilization, `taskset` core pinning).
  - SoftIRQs: per-core delta of `/proc/softirqs` (`NET_RX`, `TIMER`, `SCHED`)
    on `--bind-cores` (or all cores if unbound).
  - Memory: `/proc/<pid>/smaps_rollup` (`Rss`, `Pss`, RSS drift slope) and
    minor/major page faults.
  - Cache Miss Rate: optional `perf stat` (`cache-references`, `cache-misses`,
    `LLC-loads`, `LLC-load-misses`); gracefully degrades to `null` / `N/A` in
    unprivileged containers without `CAP_PERFMON`.
  - Network/UDP I/O: `/sys/class/net/<nic>/statistics/{rx_bytes,rx_packets,rx_dropped,rx_missed_errors}`
    and `/proc/net/udp` socket `drops`.
- **Normalized selection scoring (0-100)**: Combines throughput-normalized CPU
  cost (`CPU_per_100Mbps`), `T_driver_to_app` P99 latency, frame/packet drop
  rate, zero-copy amplification/SHM status, RSS footprint/drift, and `NET_RX`
  SoftIRQ rate, with configurable profile weights (`x86_ipc`, `orin_edge`).
- **Outputs**: Machine-readable `baseline.json`, markdown summary `report.md`,
  and self-contained offline `report.html` (embedded SVG comparison charts).

## Sources

- `benchmark/cli/wheelos_sensor_bench.py`
- `benchmark/cli/collectors.py`
- `benchmark/cli/scoring.py`
- `benchmark/cli/report.py`
- `benchmark/probe/cyber_channel_probe.cc`
- `benchmark/probe/alloc_probe.c`

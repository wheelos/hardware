---
name: driver-benchmark
description: Run wheelos-sensor-bench to profile hardware driver CPU, memory/zero-copy, I/O, and latency baselines, and generate multi-sensor selection reports.
---

# Driver Benchmark

## Steps

1. Run unit and end-to-end profiler tests inside the managed dev container:
   ```bash
   cd /apollo/pkgs/hardware/benchmark
   bazelisk test --config=dev //...
   ```
2. Profile a driver DAG (or driver command / running PID):
   ```bash
   bazelisk run --config=dev //:wheelos-sensor-bench -- run \
     --device lidar_at128 \
     --dag /path/to/hesai_at128p.dag \
     --channel /apollo/sensor/hesai/AT128P/PointCloud2 \
     --msg-type pointcloud \
     --duration 60s \
     --warmup 10s \
     --bind-cores 4-7 \
     --nic eth0 \
     --out /tmp/bench/lidar_at128
   ```
   Use `--no-alloc-probe` only when debugging an external binary incompatible
   with `LD_PRELOAD`; keep it enabled for two-stage zero-copy amplification
   measurements.
3. Compare multiple sensors for hardware selection:
   ```bash
   bazelisk run --config=dev //:wheelos-sensor-bench -- compare \
     /tmp/bench/hesai_at128/baseline.json \
     /tmp/bench/rslidar_m1/baseline.json \
     /tmp/bench/seyond_falcon/baseline.json \
     --profile x86_ipc \
     --out /tmp/bench/lidar_selection.html
   ```
4. Inspect `baseline.json` (for CI regression diffs), `report.md`, and
   `report.html` (offline visual comparison & red-line warnings).

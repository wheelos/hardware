# Hardware benchmark report

Copy this template for a normal device-rate run. Use `N/A` when a metric was
not measured; do not infer zero-copy or device packet loss from missing data.

## Environment

- Date / device / firmware:
- Hardware / CPU governor / affinity:
- Host OS / kernel / container OS:
- Driver repository revision / SDK version / build mode:
- Other dependency revisions or local overrides:
- Workload / interface / device rate / warmup / measurement duration:
- Probes enabled / measured process / host load:

## Results

| Metric | Value | Measurement scope |
| --- | --- | --- |
| Output count / rate / throughput | | |
| Process CPU mean / peak (% of one core) | | |
| RSS mean / peak (MiB) | | |
| Header-to-callback latency P50 / P95 / P99 / max (ms) | | |
| Intercepted large-copy bytes / payload bytes | | |
| Channel sequence gaps | | |
| Device packet gaps / observer socket drops | | |

## Reproduction and limitations

- Build and run commands, configuration, and raw `baseline.json`:
- Clock source, workload differences, unavailable metrics, and known issues:

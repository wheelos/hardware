"""Normalized driver compute-cost scoring and bottleneck red-line diagnostics."""

from __future__ import annotations

from typing import Any

PROFILES: dict[str, dict[str, Any]] = {
    "x86_ipc": {
        "weights": {
            "cpu": 0.30,
            "latency": 0.20,
            "drop": 0.20,
            "zero_copy": 0.15,
            "memory": 0.10,
            "softirq": 0.05,
        },
        "ceilings": {
            "cpu_per_100mbps": 200.0,
            "cpu_raw_pct": 400.0,
            "p99_latency_ms": 50.0,
            "drop_rate_pct": 1.0,
            "copy_amplification": 2.0,
            "rss_mb": 1024.0,
            "net_rx_softirq_per_sec": 50000.0,
        },
    },
    "orin_edge": {
        "weights": {
            "cpu": 0.35,
            "latency": 0.20,
            "drop": 0.15,
            "zero_copy": 0.15,
            "memory": 0.10,
            "softirq": 0.05,
        },
        "ceilings": {
            "cpu_per_100mbps": 120.0,
            "cpu_raw_pct": 200.0,
            "p99_latency_ms": 30.0,
            "drop_rate_pct": 0.5,
            "copy_amplification": 1.5,
            "rss_mb": 512.0,
            "net_rx_softirq_per_sec": 30000.0,
        },
    },
}


def _clamp01(value: float) -> float:
    return max(0.0, min(1.0, value))


def _inverse_linear_score(value: float, ceiling: float) -> float:
    if ceiling <= 0.0:
        return 1.0
    return _clamp01(1.0 - (max(0.0, value) / ceiling))


def evaluate_baseline(
    baseline: dict[str, Any], profile_name: str = "x86_ipc"
) -> dict[str, Any]:
    """Compute normalized metrics, 0-100 compute-cost score, and red-line alerts."""
    profile = PROFILES.get(profile_name, PROFILES["x86_ipc"])
    weights = profile["weights"]
    ceilings = profile["ceilings"]

    os_metrics = baseline.get("os_metrics", {})
    cpu_info = os_metrics.get("cpu", {})
    mem_info = os_metrics.get("memory", {})
    io_info = os_metrics.get("io", {})
    softirq_info = os_metrics.get("softirqs", {})
    channel = baseline.get("channel_metrics") or {}
    alloc = baseline.get("alloc_probe") or {}

    cpu_mean = float(cpu_info.get("total_core_pct_mean", 0.0))
    app_mbps = float(channel.get("mean_mbps", 0.0))
    nic_mbps = float(io_info.get("nic_rx_mbps_mean", 0.0))
    effective_mbps = app_mbps if app_mbps > 0.0 else nic_mbps

    cpu_per_100mbps = (
        (cpu_mean / (effective_mbps / 100.0)) if effective_mbps >= 1.0 else None
    )

    msg_count = int(channel.get("message_count", 0))
    seq_drops = int(channel.get("dropped_sequence_count", 0))
    seq_drop_pct = (
        (seq_drops / (msg_count + seq_drops) * 100.0)
        if (msg_count + seq_drops) > 0
        else 0.0
    )
    nic_drop_pct = float(io_info.get("nic_drop_rate_pct", 0.0))
    udp_drops = int(io_info.get("udp_rcvbuf_drops_delta", 0))
    combined_drop_pct = max(seq_drop_pct, nic_drop_pct)

    d2a = channel.get("driver_to_app_ms") or {}
    has_latency = (
        msg_count > 0 and bool(d2a) and d2a.get("count", 1) > 0
        and channel.get("latency_timestamp") == "callback_entry"
    )
    p99_ms = float(d2a.get("p99_ms", 0.0))
    p50_ms = float(d2a.get("p50_ms", 0.0))
    jitter_ms = max(0.0, p99_ms - p50_ms)

    total_payload = int(channel.get("total_payload_bytes", 0))
    large_memcpy = int(alloc.get("large_memcpy_bytes", 0))
    threshold = int(alloc.get("large_copy_threshold_bytes", 4096))
    small_messages = msg_count > 0 and total_payload / msg_count < threshold
    copy_amp = (
        (large_memcpy / total_payload)
        if (alloc.get("available") and alloc.get("measurement_window_aligned")
            and total_payload > 0 and not small_messages)
        else None
    )
    transport_mode = str(channel.get("transport_mode", "UNKNOWN"))
    shm_transport = transport_mode in ("SHM", "HYBRID", "INTRA", "ICEORYX")

    zc_status = "copy_observed" if copy_amp is not None and copy_amp > 0 else "unknown"

    rss_mean = float(mem_info.get("rss_mb_mean", 0.0))
    rss_drift = float(mem_info.get("rss_drift_mb_per_min", 0.0))
    net_rx_irq = float((softirq_info.get("per_sec") or {}).get("NET_RX", 0.0))

    s_cpu = (
        _inverse_linear_score(cpu_per_100mbps, ceilings["cpu_per_100mbps"])
        if cpu_per_100mbps is not None
        else _inverse_linear_score(cpu_mean, ceilings["cpu_raw_pct"])
    )
    s_lat = _inverse_linear_score(p99_ms, ceilings["p99_latency_ms"])
    s_drop = _inverse_linear_score(combined_drop_pct, ceilings["drop_rate_pct"])
    if udp_drops > 0:
        s_drop = min(s_drop, 0.5)
    if copy_amp is not None:
        s_zc = _inverse_linear_score(copy_amp, ceilings["copy_amplification"])
        if not shm_transport:
            s_zc *= 0.6
    else:
        s_zc = 0.8 if shm_transport else 0.3

    s_mem = _inverse_linear_score(rss_mean, ceilings["rss_mb"])
    if rss_drift > 5.0:
        s_mem *= 0.6
    s_irq = _inverse_linear_score(net_rx_irq, ceilings["net_rx_softirq_per_sec"])

    weighted = (
        weights["cpu"] * s_cpu
        + weights["latency"] * s_lat
        + weights["drop"] * s_drop
        + weights["zero_copy"] * s_zc
        + weights["memory"] * s_mem
        + weights["softirq"] * s_irq
    )
    has_copy_score = copy_amp is not None and copy_amp > 0
    total_score = round(100.0 * weighted, 1) if has_latency and has_copy_score else None

    red_flags: list[str] = []
    if msg_count == 0:
        red_flags.append(
            "No channel messages observed; selection score, latency, sequence drops and zero-copy cannot be established."
        )
    elif not has_latency:
        red_flags.append(
            "No valid callback-entry driver-to-app latency samples; selection score and latency cannot be established."
        )
    if msg_count > 0 and copy_amp is None:
        red_flags.append(
            "Copy amplification is unavailable; selection score and zero-copy cannot be established."
        )
    elif copy_amp == 0:
        red_flags.append(
            "No large copies were intercepted; selection score and zero-copy cannot be established."
        )
    if copy_amp is not None:
        red_flags.append(
            "Intercepted large-copy bytes are process-wide; configured transport and this ratio do not prove end-to-end zero-copy."
        )
    if combined_drop_pct > 0.1 or udp_drops > 0:
        red_flags.append(
            f"Packet/frame drops detected (seq={seq_drop_pct:.3f}%, nic={nic_drop_pct:.3f}%, udp_drops={udp_drops}); consider raising net.core.rmem_max or NIC ring buffer."
        )
    if has_latency and p99_ms > ceilings["p99_latency_ms"]:
        red_flags.append(
            f"High driver-to-app P99 latency ({p99_ms:.2f} ms > {ceilings['p99_latency_ms']:.1f} ms)."
        )
    if copy_amp is not None and copy_amp >= 0.9:
        red_flags.append(
            f"High payload copy amplification ({copy_amp:.2f}x memcpy/payload); verify DMA-BUF / loaned SHM path."
        )
    if rss_drift > 10.0:
        red_flags.append(
            f"Sustained RSS growth ({rss_drift:.2f} MB/min); check buffer pool reuse."
        )

    return {
        "profile": profile_name,
        "total_score": total_score,
        "subscores": {
            "cpu": round(s_cpu * 100.0, 1),
            "latency": round(s_lat * 100.0, 1) if has_latency else None,
            "drop": round(s_drop * 100.0, 1) if msg_count > 0 else None,
            "zero_copy": round(s_zc * 100.0, 1) if has_copy_score else None,
            "memory": round(s_mem * 100.0, 1),
            "softirq": round(s_irq * 100.0, 1),
        },
        "normalized": {
            "effective_mbps": round(effective_mbps, 3),
            "cpu_per_100mbps": round(cpu_per_100mbps, 2)
            if cpu_per_100mbps is not None
            else None,
            "seq_drop_rate_pct": round(seq_drop_pct, 4) if msg_count > 0 else None,
            "combined_drop_rate_pct": round(combined_drop_pct, 4) if msg_count > 0 else None,
            "copy_amplification_ratio": round(copy_amp, 3)
            if copy_amp is not None
            else None,
            "zero_copy_status": zc_status,
            "p99_latency_ms": round(p99_ms, 3) if has_latency else None,
            "jitter_ms": round(jitter_ms, 3) if has_latency else None,
        },
        "red_flags": red_flags,
    }

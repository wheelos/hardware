"""Baseline JSON, Markdown, and offline SVG/HTML report generators."""

from __future__ import annotations

import html
import json
from pathlib import Path
from typing import Any


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def _fmt(val: Any, suffix: str = "") -> str:
    if val is None:
        return "N/A"
    if isinstance(val, float):
        return f"{val:.2f}{suffix}"
    return f"{val}{suffix}"


def render_markdown(items: list[dict[str, Any]]) -> str:
    lines = [
        "# WheelOS Driver Benchmark Report",
        "",
        "| Device | Score | CPU% (Mean) | CPU / 100Mbps | RSS (MB) | Copy Amp | Zero-Copy Status | P50 / P99 Latency (ms) | Drop% | NET_RX IRQ/s |",
        "| --- | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: |",
    ]
    for item in items:
        ev = item.get("evaluation", {})
        norm = ev.get("normalized", {})
        os_m = item.get("os_metrics", {})
        cpu = os_m.get("cpu", {})
        mem = os_m.get("memory", {})
        irq = (os_m.get("softirqs", {}) or {}).get("per_sec", {})
        chan = item.get("channel_metrics") or {}
        d2a = chan.get("driver_to_app_ms") or {}
        lat_str = f"{_fmt(d2a.get('p50_ms'))} / {_fmt(d2a.get('p99_ms'))}"
        copy_amp = (
            f"{norm['copy_amplification_ratio']:.2f}x"
            if norm.get("copy_amplification_ratio") is not None
            else "N/A"
        )
        lines.append(
            f"| {item.get('device', 'unknown')} | "
            f"**{ev.get('total_score', 0.0):.1f}** | "
            f"{_fmt(cpu.get('total_core_pct_mean'), '%')} | "
            f"{_fmt(norm.get('cpu_per_100mbps'), '%')} | "
            f"{_fmt(mem.get('rss_mb_mean'))} | "
            f"{copy_amp} | "
            f"{norm.get('zero_copy_status', 'unknown')} | "
            f"{lat_str} | "
            f"{_fmt(norm.get('combined_drop_rate_pct'), '%')} | "
            f"{_fmt(irq.get('NET_RX'))} |"
        )

    lines.append("")
    lines.append("## Diagnostics and Red Flags")
    lines.append("")
    any_flag = False
    for item in items:
        flags = (item.get("evaluation") or {}).get("red_flags") or []
        if flags:
            any_flag = True
            lines.append(f"### {item.get('device', 'unknown')}")
            for flag in flags:
                lines.append(f"- {flag}")
            lines.append("")
    if not any_flag:
        lines.append("- No red-line threshold violations detected.")
        lines.append("")
    return "\n".join(lines)


def _svg_bar_chart(
    title: str, rows: list[tuple[str, float, str]], max_val: float | None = None
) -> str:
    width = 620
    bar_h = 22
    gap = 10
    top = 34
    height = top + len(rows) * (bar_h + gap) + 16
    effective_max = max_val or max((v for _, v, _ in rows), default=1.0)
    if effective_max <= 0.0:
        effective_max = 1.0

    parts = [
        f'<svg viewBox="0 0 {width} {height}" width="100%" height="{height}" '
        'xmlns="http://www.w3.org/2000/svg" style="background:#fff;border:1px solid #e5e7eb;border-radius:8px;">',
        f'<text x="16" y="22" font-size="14" font-weight="600" fill="#111827">{html.escape(title)}</text>',
    ]
    label_w = 170
    chart_w = width - label_w - 95
    for idx, (label, val, color) in enumerate(rows):
        y = top + idx * (bar_h + gap)
        w = max(2.0, min(chart_w, (max(0.0, val) / effective_max) * chart_w))
        parts.append(
            f'<text x="16" y="{y + 15}" font-size="12" fill="#374151">{html.escape(label[:22])}</text>'
        )
        parts.append(
            f'<rect x="{label_w}" y="{y}" width="{w:.1f}" height="{bar_h}" rx="4" fill="{color}"/>'
        )
        parts.append(
            f'<text x="{label_w + w + 8:.1f}" y="{y + 15}" font-size="12" fill="#111827">{val:.2f}</text>'
        )
    parts.append("</svg>")
    return "\n".join(parts)


def render_html(items: list[dict[str, Any]]) -> str:
    score_rows = [
        (
            item.get("device", "unknown"),
            float((item.get("evaluation") or {}).get("total_score", 0.0)),
            "#2563eb",
        )
        for item in items
    ]
    cpu_rows = [
        (
            item.get("device", "unknown"),
            float(
                ((item.get("evaluation") or {}).get("normalized") or {}).get(
                    "cpu_per_100mbps"
                )
                or ((item.get("os_metrics") or {}).get("cpu") or {}).get(
                    "total_core_pct_mean", 0.0
                )
            ),
            "#059669",
        )
        for item in items
    ]
    lat_rows = [
        (
            item.get("device", "unknown"),
            float(
                ((item.get("evaluation") or {}).get("normalized") or {}).get(
                    "p99_latency_ms", 0.0
                )
            ),
            "#d97706",
        )
        for item in items
    ]

    table_rows = []
    for item in items:
        ev = item.get("evaluation", {})
        norm = ev.get("normalized", {})
        os_m = item.get("os_metrics", {})
        cpu = os_m.get("cpu", {})
        mem = os_m.get("memory", {})
        perf = item.get("perf_cache") or {}
        chan = item.get("channel_metrics") or {}
        d2a = chan.get("driver_to_app_ms") or {}
        flags = ev.get("red_flags") or []
        flag_html = (
            "<br/>".join(html.escape(f) for f in flags)
            if flags
            else '<span style="color:#059669;">OK</span>'
        )
        table_rows.append(
            "<tr>"
            f"<td><strong>{html.escape(str(item.get('device', 'unknown')))}</strong></td>"
            f"<td><strong>{ev.get('total_score', 0.0):.1f}</strong></td>"
            f"<td>{_fmt(cpu.get('total_core_pct_mean'), '%')} "
            f"(u:{_fmt(cpu.get('user_core_pct_mean'))}/s:{_fmt(cpu.get('sys_core_pct_mean'))})</td>"
            f"<td>{_fmt(norm.get('cpu_per_100mbps'), '%')}</td>"
            f"<td>{_fmt(mem.get('rss_mb_mean'), ' MB')}</td>"
            f"<td>{_fmt(perf.get('cache_miss_rate_pct'), '%')}</td>"
            f"<td>{_fmt(norm.get('copy_amplification_ratio'), 'x')} ({html.escape(str(norm.get('zero_copy_status', 'N/A')))})</td>"
            f"<td>{_fmt(norm.get('effective_mbps'), ' Mbps')}</td>"
            f"<td>{_fmt(d2a.get('p50_ms'))} / {_fmt(d2a.get('p99_ms'))} ms</td>"
            f"<td>{_fmt(norm.get('combined_drop_rate_pct'), '%')}</td>"
            f"<td>{flag_html}</td>"
            "</tr>"
        )

    return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <title>WheelOS Driver Benchmark Report</title>
  <style>
    body {{ font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; margin: 24px; background: #f9fafb; color: #111827; }}
    h1 {{ margin-bottom: 6px; }}
    .sub {{ color: #6b7280; margin-bottom: 20px; }}
    .charts {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(460px, 1fr)); gap: 16px; margin-bottom: 24px; }}
    table {{ width: 100%; border-collapse: collapse; background: #fff; border-radius: 8px; overflow: hidden; box-shadow: 0 1px 2px rgba(0,0,0,.06); }}
    th, td {{ padding: 10px 12px; border-bottom: 1px solid #e5e7eb; text-align: left; font-size: 13px; vertical-align: top; }}
    th {{ background: #f3f4f6; font-weight: 600; }}
  </style>
</head>
<body>
  <h1>WheelOS Driver Benchmark &amp; Selection Report</h1>
  <div class="sub">Standardized CPU, Zero-Copy Memory, I/O Throughput, and End-to-End Latency Baseline</div>
  <div class="charts">
    {_svg_bar_chart("Compute-Cost Composite Score (0-100, Higher is Better)", score_rows, 100.0)}
    {_svg_bar_chart("Normalized CPU% per 100 Mbps (Lower is Better)", cpu_rows)}
    {_svg_bar_chart("Driver-to-App P99 Latency ms (Lower is Better)", lat_rows)}
  </div>
  <table>
    <thead>
      <tr>
        <th>Device</th>
        <th>Score</th>
        <th>CPU% (User/Sys)</th>
        <th>CPU / 100Mbps</th>
        <th>RSS</th>
        <th>Cache Miss</th>
        <th>Copy Amp / Zero-Copy</th>
        <th>Throughput</th>
        <th>P50 / P99 Latency</th>
        <th>Drop%</th>
        <th>Diagnostics</th>
      </tr>
    </thead>
    <tbody>
      {''.join(table_rows)}
    </tbody>
  </table>
</body>
</html>
"""

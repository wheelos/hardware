"""Unit and end-to-end tests for wheelos-sensor-bench."""

from __future__ import annotations

import json
import os
from pathlib import Path
import tempfile
import unittest

from cli.collectors import parse_core_list, parse_perf_stat_csv
from cli.report import render_html, render_markdown
from cli.scoring import evaluate_baseline
from cli.wheelos_sensor_bench import _find_asset, main as bench_main


class BenchmarkUnitTest(unittest.TestCase):
    def test_parse_core_list(self) -> None:
        self.assertEqual(parse_core_list("4-7,2,6"), [2, 4, 5, 6, 7])
        self.assertIsNone(parse_core_list(None))

    def test_parse_perf_stat_csv(self) -> None:
        sample = (
            "1000000,,cache-references,100000000,100.00,,\n"
            "25000,,cache-misses,100000000,100.00,2.50,of all cache refs\n"
            "500000,,LLC-loads,100000000,100.00,,\n"
            "10000,,LLC-load-misses,100000000,100.00,2.00,\n"
        )
        parsed = parse_perf_stat_csv(sample)
        self.assertTrue(parsed["available"])
        self.assertEqual(parsed["cache_miss_rate_pct"], 2.5)
        self.assertEqual(parsed["llc_miss_rate_pct"], 2.0)

        denied = parse_perf_stat_csv("<not supported>,,cache-misses")
        self.assertFalse(denied["available"])

    def test_scoring_and_reports(self) -> None:
        good = {
            "device": "hesai_at128p",
            "os_metrics": {
                "cpu": {"total_core_pct_mean": 45.0, "user_core_pct_mean": 35.0, "sys_core_pct_mean": 10.0},
                "memory": {"rss_mb_mean": 180.0, "rss_drift_mb_per_min": 0.1},
                "softirqs": {"per_sec": {"NET_RX": 4200.0}},
                "io": {"nic_rx_mbps_mean": 120.0, "nic_drop_rate_pct": 0.0, "udp_rcvbuf_drops_delta": 0},
            },
            "channel_metrics": {
                "mean_mbps": 120.0,
                "message_count": 200,
                "dropped_sequence_count": 0,
                "total_payload_bytes": 10_000_000,
                "transport_mode": "SHM",
                "driver_to_app_ms": {"p50_ms": 1.2, "p99_ms": 3.5},
            },
            "alloc_probe": {"available": True, "large_memcpy_bytes": 500_000},
        }
        bad = {
            "device": "copied_cam",
            "os_metrics": {
                "cpu": {"total_core_pct_mean": 180.0, "user_core_pct_mean": 120.0, "sys_core_pct_mean": 60.0},
                "memory": {"rss_mb_mean": 600.0, "rss_drift_mb_per_min": 15.0},
                "softirqs": {"per_sec": {"NET_RX": 18000.0}},
                "io": {"nic_rx_mbps_mean": 100.0, "nic_drop_rate_pct": 0.5, "udp_rcvbuf_drops_delta": 12},
            },
            "channel_metrics": {
                "mean_mbps": 100.0,
                "message_count": 200,
                "dropped_sequence_count": 2,
                "total_payload_bytes": 10_000_000,
                "transport_mode": "RTPS",
                "driver_to_app_ms": {"p50_ms": 8.0, "p99_ms": 65.0},
            },
            "alloc_probe": {"available": True, "large_memcpy_bytes": 20_000_000},
        }
        good["evaluation"] = evaluate_baseline(good, "x86_ipc")
        bad["evaluation"] = evaluate_baseline(bad, "x86_ipc")
        self.assertGreater(good["evaluation"]["total_score"], bad["evaluation"]["total_score"])
        self.assertEqual(good["evaluation"]["normalized"]["zero_copy_status"], "end_to_end_zero_copy")
        self.assertGreaterEqual(len(bad["evaluation"]["red_flags"]), 3)

        md = render_markdown([good, bad])
        html_doc = render_html([good, bad])
        self.assertIn("hesai_at128p", md)
        self.assertIn("<svg", html_doc)

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            p1 = tmp_path / "good.json"
            p2 = tmp_path / "bad.json"
            p1.write_text(json.dumps(good), encoding="utf-8")
            p2.write_text(json.dumps(bad), encoding="utf-8")
            out_html = tmp_path / "compare.html"
            rc = bench_main(["compare", str(p1), str(p2), "--out", str(out_html)])
            self.assertEqual(rc, 0)
            self.assertTrue(out_html.is_file())
            self.assertTrue((tmp_path / "report.md").is_file())

    def test_end_to_end_cyber_run_with_alloc_probe(self) -> None:
        pub_bin = _find_asset(None, ["probe/synthetic_sensor_pub"])
        probe_bin = _find_asset(None, ["probe/cyber_channel_probe"])
        alloc_so = _find_asset(None, ["probe/libwheelos_alloc_probe.so"])
        self.assertIsNotNone(pub_bin, "synthetic_sensor_pub missing from runfiles")
        self.assertIsNotNone(probe_bin, "cyber_channel_probe missing from runfiles")
        self.assertIsNotNone(alloc_so, "libwheelos_alloc_probe.so missing from runfiles")

        channel = f"/apollo/sensor/bench/test_{os.getpid()}"
        with tempfile.TemporaryDirectory() as tmp:
            out_dir = Path(tmp) / "run_out"
            cmd = (
                f"{pub_bin} --channel={channel} --hz=25 --payload_bytes=65536 "
                f"--duration_sec=8 --simulate_copy=true"
            )
            rc = bench_main(
                [
                    "run",
                    "--device",
                    "synthetic_cam_e2e",
                    "--cmd",
                    cmd,
                    "--channel",
                    channel,
                    "--msg-type",
                    "image",
                    "--warmup",
                    "0.5s",
                    "--duration",
                    "2.5s",
                    "--no-perf",
                    "--channel-probe-bin",
                    str(probe_bin),
                    "--alloc-probe-so",
                    str(alloc_so),
                    "--out",
                    str(out_dir),
                ]
            )
            self.assertEqual(rc, 0)
            baseline = json.loads((out_dir / "baseline.json").read_text(encoding="utf-8"))
            self.assertTrue((out_dir / "report.md").is_file())
            self.assertTrue((out_dir / "report.html").is_file())
            self.assertGreater(baseline["os_metrics"]["memory"]["rss_mb_mean"], 0.0)
            self.assertGreater(baseline["channel_metrics"]["message_count"], 5)
            self.assertTrue(baseline["alloc_probe"]["available"])
            self.assertGreater(baseline["alloc_probe"]["large_memcpy_bytes"], 65536 * 5)


if __name__ == "__main__":
    unittest.main()

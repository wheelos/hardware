"""Unit and end-to-end tests for wheelos-sensor-bench."""

from __future__ import annotations

import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

from cli.collectors import collect_preflight, parse_core_list, parse_perf_stat_csv
from cli.report import render_html, render_markdown
from cli.scoring import evaluate_baseline
from cli.vanjee_packets import PacketSequence
from cli.vanjee_10hz import main as vanjee_main
from cli.wheelos_sensor_bench import _find_asset, main as bench_main


class BenchmarkUnitTest(unittest.TestCase):
    def test_preflight_records_board_and_runtime_os(self) -> None:
        def fake_read(path: Path) -> str | None:
            if str(path) == "/proc/device-tree/model":
                return "Jetson Orin\0"
            return None

        with mock.patch("cli.collectors._read_text", side_effect=fake_read), \
             mock.patch("cli.collectors.platform.freedesktop_os_release",
                        return_value={"PRETTY_NAME": "Ubuntu 22.04"}):
            result = collect_preflight(None, None)
        self.assertEqual("Jetson Orin", result["board"])
        self.assertEqual("Ubuntu 22.04", result["userspace_os"])

    def test_report_contains_environment_and_driver_version(self) -> None:
        baseline = {
            "device": "example",
            "preflight": {"board": "Jetson Orin", "userspace_os": "Ubuntu 22.04",
                          "kernel": "5.15"},
            "target": {"driver_version": "rev-123", "warmup_sec": 20,
                       "duration_sec": 1800},
            "channel_metrics": {"message_count": 12, "hz": 10.0},
            "os_metrics": {"cpu": {"total_core_pct_peak": 15.0},
                           "memory": {"rss_mb_max": 50.0}},
        }
        for report in (render_markdown([baseline]), render_html([baseline])):
            for value in ("Jetson Orin", "Ubuntu 22.04", "5.15", "rev-123", "1800", "15.00%", "50.00"):
                self.assertIn(value, report)

    def test_vanjee_run_rejects_empty_channel_and_reports_packet_observation(self) -> None:
        for count, expected in ((10, 0), (0, 1)):
            with self.subTest(count=count), tempfile.TemporaryDirectory() as tmp:
                out = Path(tmp) / "output"
                packets = {
                    "available": True, "packets": 100, "missing_sequence_estimate": 0,
                    "duplicates": 0, "out_of_order": 0, "capture_socket_drops": 0,
                }

                def fake_bench(_args: list[str]) -> int:
                    (out / "baseline.json").write_text(json.dumps({
                        "device": "test", "channel_metrics": {"message_count": count},
                    }), encoding="utf-8")
                    (out / "report.md").write_text("# Test\n", encoding="utf-8")
                    return 0

                with mock.patch.dict(os.environ, {"CYBER_DOMAIN_ID": "183", "CYBER_PATH": "/tmp"}), \
                     mock.patch("cli.vanjee_10hz.socket.socket"), \
                     mock.patch("cli.vanjee_10hz.PacketObserver") as observer, \
                     mock.patch("cli.vanjee_10hz.bench_main", side_effect=fake_bench):
                    observer.return_value.finish.return_value = packets
                    result = vanjee_main([
                        "--mainboard", sys.executable, "--component", sys.executable,
                        "--host-address", "127.0.0.1", "--lidar-address", "127.0.0.2",
                        "--interface", "lo", "--out", str(out),
                    ])
                self.assertEqual(expected, result)
                self.assertEqual(packets, json.loads((out / "baseline.json").read_text())["vanjee_packet_observation"])
                self.assertIn("Observed 100 packets", (out / "report.md").read_text())
                self.assertIn("Observed 100 packets", (out / "report.html").read_text())

    def test_vanjee_packet_sequence_wrap_and_filtering(self) -> None:
        sequence = PacketSequence("192.0.2.6", "192.0.2.3", 3333, 3001)

        def packet(seq: int, port: int = 3001) -> bytes:
            frame = bytearray(42 + 1253)
            frame[12:14] = b"\x08\x00"
            frame[14] = 0x45
            frame[23] = 17
            frame[26:30] = bytes([192, 0, 2, 6])
            frame[30:34] = bytes([192, 0, 2, 3])
            struct.pack_into("!HHHH", frame, 34, 3333, port, 1261, 0)
            frame[44:47] = b"\x10\x01\x12"
            struct.pack_into("!H", frame, 42 + 1249, seq)
            return bytes(frame)

        for value in (65534, 65535, 0, 2, 2, 1):
            sequence.record(packet(value))
        sequence.record(packet(3, port=3002))
        self.assertEqual(sequence.result(), {
            "packets": 6, "missing_sequence_estimate": 1,
            "duplicates": 1, "out_of_order": 1,
        })

    def test_asset_search_preserves_runfiles_symlink(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "source.py"
            source.touch()
            main = root / "runfiles" / "_main"
            (main / "cli").mkdir(parents=True)
            (main / "probe").mkdir()
            script = main / "cli" / "wheelos_sensor_bench.py"
            script.symlink_to(source)
            probe = main / "probe" / "libwheelos_alloc_probe.so"
            probe.touch()
            with mock.patch("cli.wheelos_sensor_bench.__file__", str(script)):
                self.assertEqual(str(probe), _find_asset(None, ["probe/libwheelos_alloc_probe.so"]))

    def test_missing_allocation_probe_is_not_silently_disabled(self) -> None:
        with tempfile.TemporaryDirectory() as tmp, mock.patch(
            "cli.wheelos_sensor_bench._find_asset", return_value=None
        ):
            with self.assertRaisesRegex(SystemExit, "Allocation probe not found"):
                bench_main([
                    "run", "--device", "missing", "--cmd", "true",
                    "--warmup", "0s", "--duration", "1s", "--out", tmp,
                ])

    def test_explicit_missing_probe_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(SystemExit, "Allocation probe not found"):
                bench_main([
                    "run", "--device", "missing", "--cmd", "true",
                    "--alloc-probe-so", str(Path(tmp) / "missing.so"),
                    "--warmup", "0s", "--duration", "1s", "--out", tmp,
                ])
            self.assertFalse((Path(tmp) / "baseline.json").exists())

    def test_missing_pid_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(SystemExit, "is not running"):
                bench_main([
                    "run", "--device", "missing", "--pid", "2147483647",
                    "--warmup", "0s", "--duration", "1s", "--no-perf", "--out", tmp,
                ])
            self.assertFalse((Path(tmp) / "baseline.json").exists())

    def test_empty_channel_is_not_scored_as_success(self) -> None:
        baseline = {
            "device": "no_signal",
            "channel_metrics": {"message_count": 0, "transport_mode": "SHM",
                                "driver_to_app_ms": {"p50_ms": 0.0, "p99_ms": 0.0}},
        }
        baseline["evaluation"] = evaluate_baseline(baseline)
        self.assertIsNone(baseline["evaluation"]["total_score"])
        self.assertIsNone(baseline["evaluation"]["normalized"]["p99_latency_ms"])
        self.assertEqual(baseline["evaluation"]["normalized"]["zero_copy_status"], "unknown")
        self.assertIn("No channel messages", baseline["evaluation"]["red_flags"][0])
        self.assertIn("N/A", render_markdown([baseline]))
        self.assertIn("N/A", render_html([baseline]))
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "baseline.json"
            path.write_text(json.dumps(baseline), encoding="utf-8")
            self.assertEqual(0, bench_main(["compare", str(path), "--out", str(Path(tmp) / "comparison")]))

    def test_missing_latency_samples_are_not_scored(self) -> None:
        baseline = {
            "device": "no_latency",
            "channel_metrics": {"message_count": 10, "driver_to_app_ms": {
                "count": 0, "p50_ms": 0.0, "p99_ms": 0.0,
            }},
        }
        baseline["evaluation"] = evaluate_baseline(baseline)
        self.assertIsNone(baseline["evaluation"]["total_score"])
        self.assertIsNone(baseline["evaluation"]["subscores"]["latency"])
        self.assertIn("No valid callback-entry", baseline["evaluation"]["red_flags"][0])
        self.assertIn("N/A / N/A", render_markdown([baseline]))
        self.assertIn("N/A / N/A", render_html([baseline]))

    def test_legacy_latency_does_not_trigger_threshold_warning(self) -> None:
        result = evaluate_baseline({
            "channel_metrics": {"message_count": 1, "driver_to_app_ms": {
                "count": 1, "p50_ms": 100.0, "p99_ms": 500.0,
            }},
        })
        self.assertIsNone(result["subscores"]["latency"])
        self.assertFalse(any("High driver-to-app" in flag for flag in result["red_flags"]))

    def test_no_intercepted_copies_do_not_prove_zero_copy(self) -> None:
        result = evaluate_baseline({
            "channel_metrics": {
                "message_count": 1, "total_payload_bytes": 10000,
                "transport_mode": "SHM", "latency_timestamp": "callback_entry",
                "driver_to_app_ms": {"count": 1, "p50_ms": 1.0, "p99_ms": 2.0},
            },
            "alloc_probe": {"available": True, "measurement_window_aligned": True,
                            "large_memcpy_bytes": 0},
        })
        self.assertEqual("unknown", result["normalized"]["zero_copy_status"])
        self.assertIsNone(result["subscores"]["zero_copy"])
        self.assertIsNone(result["total_score"])

    def test_small_gnss_messages_do_not_imply_zero_copy(self) -> None:
        baseline = {
            "channel_metrics": {
                "message_count": 10, "total_payload_bytes": 1000,
                "transport_mode": "SHM",
                "driver_to_app_ms": {"count": 10, "p50_ms": 1.0, "p99_ms": 2.0},
            },
            "alloc_probe": {"available": True, "large_copy_threshold_bytes": 4096,
                            "large_memcpy_bytes": 0},
        }
        result = evaluate_baseline(baseline)
        self.assertIsNone(result["total_score"])
        self.assertIsNone(result["normalized"]["copy_amplification_ratio"])
        self.assertEqual("unknown", result["normalized"]["zero_copy_status"])
        self.assertIsNone(result["subscores"]["zero_copy"])

    def test_invalid_timing_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            for option, value in (("--duration", "0s"), ("--duration", "nan"),
                                  ("--warmup", "-1s"), ("--sample-interval", "0")):
                with self.subTest(option=option, value=value):
                    with self.assertRaises(SystemExit):
                        bench_main(["run", "--device", "invalid", "--cmd", "true",
                                    "--out", tmp, f"{option}={value}"])

    def test_stale_channel_results_cannot_mask_probe_failure(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            (out / "channel_probe.json").write_text('{"message_count": 123}', encoding="utf-8")
            probe = out / "empty_probe"
            probe.write_text(
                "#!/bin/sh\nfor arg in \"$@\"; do case \"$arg\" in "
                "--ready_file=*) touch \"${arg#--ready_file=}\";; esac; done\n"
                "sleep 0.7\n", encoding="utf-8"
            )
            probe.chmod(0o755)
            command = f"{shlex.quote(sys.executable)} -c {shlex.quote('import time; time.sleep(5)')}"
            with self.assertRaisesRegex(SystemExit, "Channel probe did not produce valid results"):
                bench_main([
                    "run", "--device", "failed", "--cmd", command,
                    "--channel", "/test", "--channel-probe-bin", str(probe),
                    "--warmup", "0.1s", "--duration", "0.5s", "--no-perf",
                    "--no-alloc-probe", "--out", tmp,
                ])
            self.assertFalse((out / "baseline.json").exists())

    def test_channel_probe_exit_aborts_measurement(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            probe = out / "early_probe"
            probe.write_text(
                "#!/bin/sh\nfor arg in \"$@\"; do case \"$arg\" in "
                "--ready_file=*) touch \"${arg#--ready_file=}\";; esac; done\n"
                "sleep 0.3\nexit 7\n", encoding="utf-8"
            )
            probe.chmod(0o755)
            command = f"{shlex.quote(sys.executable)} -c {shlex.quote('import time; time.sleep(10)')}"
            start = time.monotonic()
            with self.assertRaisesRegex(SystemExit, "Channel probe exited before measurement completed"):
                bench_main([
                    "run", "--device", "failed", "--cmd", command,
                    "--channel", "/test", "--channel-probe-bin", str(probe),
                    "--warmup", "0s", "--duration", "5s", "--no-perf",
                    "--no-alloc-probe", "--out", tmp,
                ])
            self.assertLess(time.monotonic() - start, 4)
            self.assertFalse((out / "baseline.json").exists())

    def test_early_target_exit_does_not_create_baseline(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            command = f"{shlex.quote(sys.executable)} -c {shlex.quote('import time; time.sleep(0.2)')}"
            with self.assertRaisesRegex(SystemExit, "Target exited before measurement completed"):
                bench_main([
                    "run", "--device", "failed", "--cmd", command,
                    "--warmup", "0s", "--duration", "1s", "--no-perf",
                    "--no-alloc-probe", "--out", tmp,
                ])
            self.assertFalse((Path(tmp) / "baseline.json").exists())

    def test_target_exit_during_warmup_does_not_create_baseline(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            command = f"{shlex.quote(sys.executable)} -c {shlex.quote('raise SystemExit(7)')}"
            with self.assertRaisesRegex(SystemExit, "Target exited during warmup"):
                bench_main([
                    "run", "--device", "failed", "--cmd", command,
                    "--warmup", "0.2s", "--duration", "1s", "--no-perf",
                    "--no-alloc-probe", "--out", tmp,
                ])
            self.assertFalse((Path(tmp) / "baseline.json").exists())

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
                "latency_timestamp": "callback_entry",
                "driver_to_app_ms": {"p50_ms": 1.2, "p99_ms": 3.5},
            },
            "alloc_probe": {"available": True, "measurement_window_aligned": True,
                            "large_memcpy_bytes": 500_000},
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
                "latency_timestamp": "callback_entry",
                "driver_to_app_ms": {"p50_ms": 8.0, "p99_ms": 65.0},
            },
            "alloc_probe": {"available": True, "measurement_window_aligned": True,
                            "large_memcpy_bytes": 20_000_000},
        }
        good["evaluation"] = evaluate_baseline(good, "x86_ipc")
        bad["evaluation"] = evaluate_baseline(bad, "x86_ipc")
        self.assertGreater(good["evaluation"]["total_score"], bad["evaluation"]["total_score"])
        self.assertEqual(good["evaluation"]["normalized"]["zero_copy_status"], "copy_observed")
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
            self.assertTrue(baseline["alloc_probe"]["measurement_window_aligned"])
            self.assertEqual(baseline["channel_metrics"]["latency_timestamp"], "callback_entry")
            self.assertGreater(baseline["alloc_probe"]["large_memcpy_bytes"], 65536 * 5)

    def test_old_unaligned_measurements_are_not_scored(self) -> None:
        baseline = {
            "channel_metrics": {
                "message_count": 10, "total_payload_bytes": 100000,
                "transport_mode": "SHM",
                "driver_to_app_ms": {"count": 10, "p50_ms": 1.0, "p99_ms": 2.0},
            },
            "alloc_probe": {"available": True, "large_memcpy_bytes": 5000},
        }
        result = evaluate_baseline(baseline)
        self.assertIsNone(result["total_score"])
        self.assertIsNone(result["normalized"]["copy_amplification_ratio"])
        self.assertIsNone(result["normalized"]["p99_latency_ms"])
        self.assertEqual(result["normalized"]["zero_copy_status"], "unknown")

    def test_alloc_probe_excludes_warmup_and_shutdown(self) -> None:
        alloc_so = _find_asset(None, ["probe/libwheelos_alloc_probe.so"])
        self.assertIsNotNone(alloc_so)
        child = """
import ctypes, sys
lib = ctypes.CDLL(None)
copy = lib.memcpy
copy.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
src = ctypes.create_string_buffer(1048576)
dst = ctypes.create_string_buffer(1048576)
def copies(n):
    for _ in range(n):
        copy(dst, src, 1048576)
copies(4)
print("warmup", flush=True)
sys.stdin.readline()
copies(1)
print("measurement", flush=True)
sys.stdin.readline()
copies(4)
"""
        with tempfile.TemporaryDirectory() as tmp:
            window = Path(tmp) / "window"
            window.write_bytes(b"\0" * 16)
            output = Path(tmp) / "alloc.json"
            env = dict(os.environ, LD_PRELOAD=str(alloc_so),
                       WHEELOS_ALLOC_PROBE_OUT=str(output),
                       WHEELOS_BENCH_WINDOW=str(window))
            proc = subprocess.Popen([sys.executable, "-u", "-c", child], env=env,
                                    stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                    text=True)
            try:
                self.assertEqual(proc.stdout.readline().strip(), "warmup")
                with window.open("r+b", buffering=0) as control:
                    now = time.monotonic_ns()
                    control.seek(8)
                    control.write(struct.pack("=Q", now + 20_000_000_000))
                    control.seek(0)
                    control.write(struct.pack("=Q", now))
                    proc.stdin.write("go\n")
                    proc.stdin.flush()
                    self.assertEqual(proc.stdout.readline().strip(), "measurement")
                    control.seek(8)
                    control.write(struct.pack("=Q", time.monotonic_ns()))
                proc.stdin.write("go\n")
                proc.stdin.flush()
                self.assertEqual(proc.wait(timeout=10), 0)
            finally:
                if proc.poll() is None:
                    proc.kill()
                    proc.wait()
                proc.stdin.close()
                proc.stdout.close()
            result = json.loads(output.read_text(encoding="utf-8"))
            self.assertTrue(result["measurement_window_aligned"])
            self.assertGreaterEqual(result["large_memcpy_bytes"], 1048576)
            self.assertLess(result["large_memcpy_bytes"], 2 * 1048576)


if __name__ == "__main__":
    unittest.main()

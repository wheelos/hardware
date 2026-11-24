"""wheelos-sensor-bench: One-click driver benchmark & selection profiler."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
import math
import os
from pathlib import Path
import shlex
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import time
from typing import Any

from cli.collectors import (
    PerfStatSession, ProcSampler, collect_preflight, parse_core_list, read_proc_stat,
)
from cli.report import render_html, render_markdown, write_json
from cli.scoring import evaluate_baseline


def parse_duration(value: str) -> float:
    text = value.strip().lower()
    if text.endswith("ms"):
        return float(text[:-2]) / 1000.0
    if text.endswith("s"):
        return float(text[:-1])
    if text.endswith("m"):
        return float(text[:-1]) * 60.0
    return float(text)


def _find_asset(
    explicit: str | None, rel_candidates: list[str], bin_name: str | None = None,
    executable: bool = False,
) -> str | None:
    if explicit:
        access = os.X_OK if executable else os.R_OK
        return explicit if Path(explicit).is_file() and os.access(explicit, access) else None
    here = Path(__file__).absolute()
    resolved = here.resolve()
    search_roots = [here.parents[1], here.parents[2], resolved.parents[1], resolved.parents[2]]
    for env_key in ("RUNFILES_DIR", "TEST_SRCDIR"):
        val = os.environ.get(env_key)
        if val:
            search_roots.append(Path(val))
    for root in search_roots:
        for rel in rel_candidates:
            candidate = root / rel
            if candidate.exists():
                return str(candidate)
            for match in root.glob(f"*/{rel}"):
                if match.exists():
                    return str(match)
    if bin_name:
        return shutil.which(bin_name)
    return None


def _terminate_proc(proc: subprocess.Popen[Any], timeout_sec: float = 5.0) -> None:
    if proc.poll() is not None:
        return
    try:
        proc.send_signal(signal.SIGINT)
        proc.wait(timeout=timeout_sec)
        return
    except subprocess.TimeoutExpired:
        pass
    try:
        proc.terminate()
        proc.wait(timeout=2.0)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


def _load_json(path: Path) -> dict[str, Any] | None:
    if not path.is_file():
        return None
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None


def cmd_run(args: argparse.Namespace) -> int:
    duration_sec = parse_duration(args.duration)
    warmup_sec = parse_duration(args.warmup)
    if not math.isfinite(duration_sec) or duration_sec <= 0:
        raise SystemExit("--duration must be finite and greater than zero.")
    if not math.isfinite(warmup_sec) or warmup_sec < 0:
        raise SystemExit("--warmup must be finite and nonnegative.")
    if not math.isfinite(args.sample_interval) or args.sample_interval <= 0:
        raise SystemExit("--sample-interval must be finite and greater than zero.")
    bind_cores = parse_core_list(args.bind_cores)
    out_dir = Path(args.out).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".bench-probes-", dir=out_dir) as tmp:
        return _run(args, duration_sec, warmup_sec, bind_cores, out_dir, Path(tmp))


def _run(
    args: argparse.Namespace,
    duration_sec: float,
    warmup_sec: float,
    bind_cores: list[int] | None,
    out_dir: Path,
    probe_dir: Path,
) -> int:
    target_sources = [bool(args.dag), bool(args.cmd), args.pid is not None]
    if sum(target_sources) != 1:
        raise SystemExit("Specify exactly one of --dag, --cmd, or --pid.")

    alloc_probe_so = None
    alloc_json_path = probe_dir / "alloc_probe.json"
    window_path = Path(args.window_file) if args.window_file else probe_dir / "measurement-window"
    with window_path.open("xb") as window:
        window.write(b"\0" * 16)
    if not args.no_alloc_probe and args.pid is None:
        alloc_probe_so = _find_asset(
            args.alloc_probe_so,
            [
                "probe/libwheelos_alloc_probe.so",
                "share/benchmark/libwheelos_alloc_probe.so",
            ],
        )
        if not alloc_probe_so:
            raise SystemExit(
                "Allocation probe not found; pass --alloc-probe-so or explicitly disable it with --no-alloc-probe."
            )

    channel_probe_bin = None
    channel_json_path = probe_dir / "channel_probe.json"
    if args.channel:
        channel_probe_bin = _find_asset(
            args.channel_probe_bin,
            [
                "probe/cyber_channel_probe",
                "bin/modules/benchmark/cyber_channel_probe",
            ],
            bin_name="cyber_channel_probe",
            executable=True,
        )
        if not channel_probe_bin:
            raise SystemExit("cyber_channel_probe binary not found.")

    driver_proc: subprocess.Popen[Any] | None = None
    probe_proc: subprocess.Popen[Any] | None = None
    preflight = collect_preflight(args.nic, bind_cores)

    env = os.environ.copy()
    if alloc_probe_so:
        existing_preload = env.get("LD_PRELOAD", "")
        env["LD_PRELOAD"] = (
            f"{alloc_probe_so}:{existing_preload}" if existing_preload else alloc_probe_so
        )
        env["WHEELOS_ALLOC_PROBE_OUT"] = str(alloc_json_path)
        env["WHEELOS_BENCH_WINDOW"] = str(window_path)

    try:
        if args.pid is not None:
            target_pid = args.pid
        else:
            if args.dag:
                mainboard_bin = _find_asset(
                    args.mainboard_bin,
                    [
                        "cyber/mainboard/mainboard",
                        "bin/commands/mainboard",
                    ],
                    bin_name="mainboard",
                    executable=True,
                )
                if not mainboard_bin:
                    raise SystemExit("mainboard binary not found; pass --mainboard-bin.")
                launch_cmd = [mainboard_bin, "-d", args.dag]
            else:
                launch_cmd = shlex.split(args.cmd)

            if args.bind_cores:
                launch_cmd = ["taskset", "-c", args.bind_cores, *launch_cmd]

            driver_proc = subprocess.Popen(launch_cmd, env=env)
            target_pid = driver_proc.pid

        if channel_probe_bin and args.channel:
            ready_path = probe_dir / "channel-ready"
            probe_cmd = [
                channel_probe_bin,
                f"--channel={args.channel}",
                f"--msg_type={args.msg_type}",
                f"--warmup_sec={warmup_sec}",
                f"--duration_sec={duration_sec}",
                f"--output_json={channel_json_path}",
                f"--window_file={window_path}",
                f"--ready_file={ready_path}",
            ]
            probe_proc = subprocess.Popen(probe_cmd)
            ready_deadline = time.monotonic() + 10.0
            while not ready_path.exists():
                if probe_proc.poll() is not None:
                    raise SystemExit(
                        f"Channel probe exited before becoming ready (exit code {probe_proc.returncode})."
                    )
                if driver_proc is not None and driver_proc.poll() is not None:
                    raise SystemExit(
                        f"Target exited before channel probe became ready (exit code {driver_proc.returncode})."
                    )
                if time.monotonic() >= ready_deadline:
                    raise SystemExit("Channel probe did not become ready within 10 seconds.")
                time.sleep(0.02)

        if warmup_sec > 0.0:
            time.sleep(warmup_sec)

        if driver_proc is not None and driver_proc.poll() is not None:
            raise SystemExit(
                f"Target exited during warmup (exit code {driver_proc.returncode}); no new baseline recorded."
            )
        if args.pid is not None and read_proc_stat(target_pid) is None:
            raise SystemExit(f"Target PID {target_pid} is not running; no new baseline recorded.")
        if probe_proc is not None and probe_proc.poll() is not None:
            raise SystemExit(
                f"Channel probe exited during warmup (exit code {probe_proc.returncode}); no new baseline recorded."
            )

        sampler = ProcSampler(
            pid=target_pid,
            nic=args.nic,
            bind_cores=bind_cores,
            interval_sec=args.sample_interval,
        )
        perf_session = PerfStatSession(target_pid) if not args.no_perf else None

        sampler.start()
        if perf_session:
            perf_session.start()

        start_ns = time.monotonic_ns()
        end_ns = start_ns + int(duration_sec * 1_000_000_000)
        with window_path.open("r+b", buffering=0) as window:
            window.seek(8)
            window.write(struct.pack("=Q", end_ns))
            window.seek(0)
            window.write(struct.pack("=Q", start_ns))

        target_exited = False
        probe_exited_early = False
        deadline = end_ns / 1e9
        while time.monotonic() < deadline:
            if driver_proc is not None and driver_proc.poll() is not None:
                target_exited = True
                break
            if driver_proc is None and read_proc_stat(target_pid) is None:
                target_exited = True
                break
            if probe_proc is not None and probe_proc.poll() is not None:
                probe_exited_early = True
                break
            time.sleep(min(0.1, max(0.0, deadline - time.monotonic())))

        perf_metrics = (
            perf_session.stop()
            if perf_session
            else {"available": False, "reason": "disabled by --no-perf"}
        )
        os_metrics = sampler.stop()
        if target_exited:
            detail = (
                f"exit code {driver_proc.returncode}"
                if driver_proc is not None else f"PID {target_pid} disappeared"
            )
            raise SystemExit(
                f"Target exited before measurement completed ({detail}); no new baseline recorded."
            )
        if probe_exited_early:
            raise SystemExit(
                f"Channel probe exited before measurement completed "
                f"(exit code {probe_proc.returncode}); no new baseline recorded."
            )
    finally:
        if probe_proc is not None:
            _terminate_proc(probe_proc, timeout_sec=30.0)
        if driver_proc is not None:
            _terminate_proc(driver_proc, timeout_sec=30.0)

    alloc_raw = _load_json(alloc_json_path) if alloc_probe_so else None
    alloc_summary = (
        {"available": True, **alloc_raw}
        if alloc_raw is not None
        else {"available": False}
    )
    channel_metrics = _load_json(channel_json_path) if args.channel else None
    if args.channel and channel_metrics is None:
        raise SystemExit("Channel probe did not produce valid results; no new baseline recorded.")
    if probe_proc is not None and probe_proc.returncode != 0:
        raise SystemExit(
            f"Channel probe failed (exit code {probe_proc.returncode}); no new baseline recorded."
        )

    if alloc_raw is not None:
        write_json(out_dir / "alloc_probe.json", alloc_raw)
    if channel_metrics is not None:
        write_json(out_dir / "channel_probe.json", channel_metrics)

    baseline: dict[str, Any] = {
        "schema_version": "1.0",
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "device": args.device,
        "target": {
            "driver_version": args.driver_version,
            "dag": args.dag,
            "cmd": args.cmd,
            "pid": args.pid,
            "channel": args.channel,
            "msg_type": args.msg_type,
            "warmup_sec": warmup_sec,
            "duration_sec": duration_sec,
        },
        "preflight": preflight,
        "os_metrics": os_metrics,
        "perf_cache": perf_metrics,
        "alloc_probe": alloc_summary,
        "channel_metrics": channel_metrics,
    }
    baseline["evaluation"] = evaluate_baseline(baseline, profile_name=args.profile)

    baseline_path = out_dir / "baseline.json"
    md_path = out_dir / "report.md"
    html_path = out_dir / "report.html"
    write_json(baseline_path, baseline)
    md_text = render_markdown([baseline])
    md_path.write_text(md_text, encoding="utf-8")
    html_path.write_text(render_html([baseline]), encoding="utf-8")

    print(md_text)
    print(f"Wrote baseline: {baseline_path}")
    print(f"Wrote report:   {html_path}")
    return 0


def cmd_compare(args: argparse.Namespace) -> int:
    items: list[dict[str, Any]] = []
    for raw_path in args.baselines:
        data = _load_json(Path(raw_path))
        if data is None:
            raise SystemExit(f"Failed to read baseline JSON: {raw_path}")
        data["evaluation"] = evaluate_baseline(data, profile_name=args.profile)
        items.append(data)

    def score(item: dict[str, Any]) -> float:
        value = (item.get("evaluation") or {}).get("total_score")
        return float(value) if value is not None else -1.0

    items.sort(key=score, reverse=True)

    out_path = Path(args.out).resolve()
    out_dir = out_path.parent if out_path.suffix else out_path
    out_dir.mkdir(parents=True, exist_ok=True)
    html_path = out_path if out_path.suffix == ".html" else out_dir / "report.html"
    md_path = out_dir / "report.md"
    json_path = out_dir / "comparison_baseline.json"

    md_text = render_markdown(items)
    md_path.write_text(md_text, encoding="utf-8")
    html_path.write_text(render_html(items), encoding="utf-8")
    write_json(json_path, {"profile": args.profile, "devices": items})

    print(md_text)
    print(f"Wrote comparison HTML: {html_path}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="wheelos-sensor-bench",
        description="WheelOS Hardware Driver Benchmark Profiler & Selection Suite.",
    )
    sub = parser.add_subparsers(dest="subcommand", required=True)

    run_p = sub.add_parser("run", help="Profile a single sensor driver.")
    run_p.add_argument("--device", required=True, help="Logical device label (e.g. lidar_at128).")
    run_p.add_argument("--driver-version", help="Driver source revision or release used for this run.")
    run_p.add_argument("--dag", help="Cyber DAG file to launch via mainboard.")
    run_p.add_argument("--cmd", help="Explicit driver/replay command to launch.")
    run_p.add_argument("--pid", type=int, help="Attach to an already running driver PID.")
    run_p.add_argument("--channel", help="Cyber channel to subscribe for throughput/latency.")
    run_p.add_argument(
        "--msg-type",
        default="auto",
        choices=["auto", "raw", "pointcloud", "image", "compressed_image", "imu"],
    )
    run_p.add_argument("--duration", default="30s", help="Measurement duration (e.g. 60s, 5000ms).")
    run_p.add_argument("--warmup", default="5s", help="Warmup duration before sampling.")
    run_p.add_argument("--bind-cores", help="CPU core list for taskset/SoftIRQ filtering (e.g. 4-7).")
    run_p.add_argument("--nic", help="Network interface for bandwidth/drop telemetry (e.g. eth0).")
    run_p.add_argument("--sample-interval", type=float, default=0.5, help="Proc poll interval sec.")
    run_p.add_argument("--profile", default="x86_ipc", choices=["x86_ipc", "orin_edge"])
    run_p.add_argument("--out", default="bench_out", help="Output directory.")
    run_p.add_argument("--no-alloc-probe", action="store_true", help="Disable LD_PRELOAD copy probe.")
    run_p.add_argument("--no-perf", action="store_true", help="Disable perf stat cache-miss sampling.")
    run_p.add_argument("--mainboard-bin", help="Explicit path to Cyber mainboard binary.")
    run_p.add_argument("--channel-probe-bin", help="Explicit path to cyber_channel_probe.")
    run_p.add_argument("--alloc-probe-so", help="Explicit path to libwheelos_alloc_probe.so.")
    run_p.add_argument("--window-file", help=argparse.SUPPRESS)
    run_p.set_defaults(func=cmd_run)

    cmp_p = sub.add_parser("compare", help="Compare multiple baseline.json files.")
    cmp_p.add_argument("baselines", nargs="+", help="Paths to baseline.json files.")
    cmp_p.add_argument("--profile", default="x86_ipc", choices=["x86_ipc", "orin_edge"])
    cmp_p.add_argument("--out", default="bench_compare/report.html", help="Output HTML path or dir.")
    cmp_p.set_defaults(func=cmd_compare)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return int(args.func(args))


if __name__ == "__main__":
    sys.exit(main())

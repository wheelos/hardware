"""OS, kernel, NIC, and perf_event collectors for wheelos-sensor-bench."""

from __future__ import annotations

import os
from pathlib import Path
import platform
import shutil
import signal
import subprocess
import threading
import time
from typing import Any

CLK_TCK = os.sysconf("SC_CLK_TCK")


def parse_core_list(spec: str | None) -> list[int] | None:
    """Parse a CPU core specification such as '0-3,6' into sorted core IDs."""
    if not spec:
        return None
    cores: set[int] = set()
    for part in spec.split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part:
            lo_s, hi_s = part.split("-", 1)
            lo, hi = int(lo_s), int(hi_s)
            if hi < lo:
                raise ValueError(f"Invalid core range: {part}")
            cores.update(range(lo, hi + 1))
        else:
            cores.add(int(part))
    return sorted(cores)


def _read_text(path: Path) -> str | None:
    try:
        return path.read_text(encoding="utf-8", errors="replace").strip()
    except OSError:
        return None


def collect_preflight(nic: str | None, bind_cores: list[int] | None) -> dict[str, Any]:
    """Capture host environment fingerprint before benchmarking."""
    cpu_model = "unknown"
    cpuinfo = _read_text(Path("/proc/cpuinfo"))
    if cpuinfo:
        for line in cpuinfo.splitlines():
            if line.lower().startswith("model name") and ":" in line:
                cpu_model = line.split(":", 1)[1].strip()
                break

    governor = _read_text(
        Path("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")
    )
    paranoid = _read_text(Path("/proc/sys/kernel/perf_event_paranoid"))
    rmem_max = _read_text(Path("/proc/sys/net/core/rmem_max"))

    nic_info: dict[str, Any] = {}
    if nic:
        speed = _read_text(Path(f"/sys/class/net/{nic}/speed"))
        mtu = _read_text(Path(f"/sys/class/net/{nic}/mtu"))
        operstate = _read_text(Path(f"/sys/class/net/{nic}/operstate"))
        nic_info = {
            "name": nic,
            "speed_mbps": int(speed) if speed and speed.lstrip("-").isdigit() else None,
            "mtu": int(mtu) if mtu and mtu.isdigit() else None,
            "operstate": operstate,
        }

    return {
        "hostname": platform.node(),
        "kernel": platform.release(),
        "arch": platform.machine(),
        "cpu_model": cpu_model,
        "cpu_count": os.cpu_count() or 1,
        "bind_cores": bind_cores,
        "cpu_governor": governor,
        "perf_event_paranoid": int(paranoid) if paranoid and paranoid.lstrip("-").isdigit() else None,
        "rmem_max_bytes": int(rmem_max) if rmem_max and rmem_max.isdigit() else None,
        "nic": nic_info,
    }


def read_proc_stat(pid: int) -> dict[str, int] | None:
    """Read utime, stime (jiffies), minflt, majflt, rss_pages from /proc/<pid>/stat."""
    raw = _read_text(Path(f"/proc/{pid}/stat"))
    if not raw:
        return None
    rparen = raw.rfind(")")
    if rparen < 0:
        return None
    fields = raw[rparen + 2 :].split()
    # fields[0] is state (field 3 in proc(5)).
    if len(fields) < 22:
        return None
    return {
        "minflt": int(fields[7]),
        "majflt": int(fields[9]),
        "utime_ticks": int(fields[11]),
        "stime_ticks": int(fields[12]),
        "num_threads": int(fields[17]),
        "rss_pages": int(fields[21]),
    }


def read_proc_threads(pid: int) -> dict[int, dict[str, Any]]:
    """Read per-thread utime/stime ticks and comm from /proc/<pid>/task/*/stat."""
    task_dir = Path(f"/proc/{pid}/task")
    threads: dict[int, dict[str, Any]] = {}
    if not task_dir.is_dir():
        return threads
    for entry in task_dir.iterdir():
        if not entry.name.isdigit():
            continue
        tid = int(entry.name)
        raw = _read_text(entry / "stat")
        if not raw:
            continue
        lparen = raw.find("(")
        rparen = raw.rfind(")")
        if lparen < 0 or rparen < 0:
            continue
        comm = raw[lparen + 1 : rparen]
        fields = raw[rparen + 2 :].split()
        if len(fields) < 13:
            continue
        threads[tid] = {
            "tid": tid,
            "comm": comm,
            "utime_ticks": int(fields[11]),
            "stime_ticks": int(fields[12]),
        }
    return threads


def read_smaps_rollup(pid: int) -> dict[str, int]:
    """Read Rss and Pss in KiB from /proc/<pid>/smaps_rollup."""
    raw = _read_text(Path(f"/proc/{pid}/smaps_rollup"))
    result = {"rss_kb": 0, "pss_kb": 0}
    if not raw:
        return result
    for line in raw.splitlines():
        if line.startswith("Rss:"):
            parts = line.split()
            if len(parts) >= 2 and parts[1].isdigit():
                result["rss_kb"] = int(parts[1])
        elif line.startswith("Pss:"):
            parts = line.split()
            if len(parts) >= 2 and parts[1].isdigit():
                result["pss_kb"] = int(parts[1])
    return result


def read_softirqs(cores: list[int] | None = None) -> dict[str, int]:
    """Read per-category SoftIRQ counts across selected cores (or all cores)."""
    raw = _read_text(Path("/proc/softirqs"))
    totals = {"NET_RX": 0, "NET_TX": 0, "TIMER": 0, "SCHED": 0}
    if not raw:
        return totals
    lines = raw.splitlines()
    if not lines:
        return totals
    header_cols = lines[0].split()
    num_cpus = len(header_cols)
    selected = (
        [c for c in cores if 0 <= c < num_cpus] if cores is not None else list(range(num_cpus))
    )
    for line in lines[1:]:
        if ":" not in line:
            continue
        name, rest = line.split(":", 1)
        key = name.strip()
        if key not in totals:
            continue
        vals = rest.split()
        totals[key] = sum(int(vals[idx]) for idx in selected if idx < len(vals))
    return totals


def read_nic_stats(nic: str | None) -> dict[str, int]:
    """Read NIC RX byte/packet/drop counters from sysfs."""
    out = {"rx_bytes": 0, "rx_packets": 0, "rx_dropped": 0, "rx_missed_errors": 0}
    if not nic:
        return out
    base = Path(f"/sys/class/net/{nic}/statistics")
    if not base.is_dir():
        return out
    for key in out:
        val = _read_text(base / key)
        if val and val.isdigit():
            out[key] = int(val)
    return out


def read_udp_drops() -> int:
    """Sum socket receive-buffer drops from /proc/net/udp and /proc/net/udp6."""
    total_drops = 0
    for proto_file in ("/proc/net/udp", "/proc/net/udp6"):
        raw = _read_text(Path(proto_file))
        if not raw:
            continue
        for line in raw.splitlines()[1:]:
            cols = line.split()
            if cols and cols[-1].isdigit():
                total_drops += int(cols[-1])
    return total_drops


def _linear_slope(xs: list[float], ys: list[float]) -> float:
    if len(xs) < 2 or len(xs) != len(ys):
        return 0.0
    n = float(len(xs))
    mx = sum(xs) / n
    my = sum(ys) / n
    denom = sum((x - mx) ** 2 for x in xs)
    if denom <= 1e-9:
        return 0.0
    numer = sum((x - mx) * (y - my) for x, y in zip(xs, ys))
    return numer / denom


class ProcSampler:
    """Background poller collecting CPU, thread, memory, SoftIRQ, and NIC metrics."""

    def __init__(
        self,
        pid: int,
        nic: str | None = None,
        bind_cores: list[int] | None = None,
        interval_sec: float = 0.5,
    ) -> None:
        self.pid = pid
        self.nic = nic
        self.bind_cores = bind_cores
        self.interval_sec = max(0.1, interval_sec)
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None

        self._t0 = 0.0
        self._t1 = 0.0
        self._start_stat: dict[str, int] | None = None
        self._end_stat: dict[str, int] | None = None
        self._start_threads: dict[int, dict[str, Any]] = {}
        self._end_threads: dict[int, dict[str, Any]] = {}
        self._start_softirqs: dict[str, int] = {}
        self._end_softirqs: dict[str, int] = {}
        self._start_nic: dict[str, int] = {}
        self._end_nic: dict[str, int] = {}
        self._start_udp_drops = 0
        self._end_udp_drops = 0

        self._samples_t: list[float] = []
        self._cpu_samples: list[float] = []
        self._user_samples: list[float] = []
        self._sys_samples: list[float] = []
        self._rss_samples_mb: list[float] = []
        self._pss_samples_mb: list[float] = []
        self._nic_mbps_samples: list[float] = []
        self._prev_stat: dict[str, int] | None = None
        self._prev_nic: dict[str, int] | None = None
        self._prev_t = 0.0

    def start(self) -> None:
        self._t0 = time.monotonic()
        self._prev_t = self._t0
        self._start_stat = read_proc_stat(self.pid)
        self._prev_stat = self._start_stat
        self._start_threads = read_proc_threads(self.pid)
        self._start_softirqs = read_softirqs(self.bind_cores)
        self._start_nic = read_nic_stats(self.nic)
        self._prev_nic = self._start_nic
        self._start_udp_drops = read_udp_drops()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _sample_once(self) -> None:
        now = time.monotonic()
        dt = now - self._prev_t
        if dt <= 0.0:
            return
        cur_stat = read_proc_stat(self.pid)
        if cur_stat and self._prev_stat:
            du = max(0, cur_stat["utime_ticks"] - self._prev_stat["utime_ticks"])
            ds = max(0, cur_stat["stime_ticks"] - self._prev_stat["stime_ticks"])
            user_pct = (du / CLK_TCK) / dt * 100.0
            sys_pct = (ds / CLK_TCK) / dt * 100.0
            self._user_samples.append(user_pct)
            self._sys_samples.append(sys_pct)
            self._cpu_samples.append(user_pct + sys_pct)
            self._end_stat = cur_stat
        self._prev_stat = cur_stat

        threads = read_proc_threads(self.pid)
        if threads:
            self._end_threads = threads

        smaps = read_smaps_rollup(self.pid)
        rss_kb = smaps["rss_kb"]
        if rss_kb == 0 and cur_stat:
            rss_kb = cur_stat["rss_pages"] * (os.sysconf("SC_PAGE_SIZE") // 1024)
        if rss_kb > 0:
            self._samples_t.append(now - self._t0)
            self._rss_samples_mb.append(rss_kb / 1024.0)
            self._pss_samples_mb.append((smaps["pss_kb"] or rss_kb) / 1024.0)

        if self.nic:
            cur_nic = read_nic_stats(self.nic)
            if self._prev_nic:
                dbytes = max(0, cur_nic["rx_bytes"] - self._prev_nic["rx_bytes"])
                self._nic_mbps_samples.append((dbytes * 8.0) / (dt * 1e6))
            self._prev_nic = cur_nic
            self._end_nic = cur_nic

        self._prev_t = now

    def _run(self) -> None:
        while not self._stop.wait(self.interval_sec):
            self._sample_once()

    def stop(self) -> dict[str, Any]:
        self._stop.set()
        if self._thread is not None:
            self._thread.join(timeout=2.0)
        self._sample_once()
        self._t1 = time.monotonic()
        elapsed = max(1e-6, self._t1 - self._t0)

        end_softirqs = read_softirqs(self.bind_cores)
        end_udp_drops = read_udp_drops()
        end_nic = self._end_nic or read_nic_stats(self.nic)

        cpu_mean = sum(self._cpu_samples) / len(self._cpu_samples) if self._cpu_samples else 0.0
        cpu_peak = max(self._cpu_samples) if self._cpu_samples else 0.0
        user_mean = sum(self._user_samples) / len(self._user_samples) if self._user_samples else 0.0
        sys_mean = sum(self._sys_samples) / len(self._sys_samples) if self._sys_samples else 0.0

        if self._start_stat and self._end_stat:
            du_tot = max(0, self._end_stat["utime_ticks"] - self._start_stat["utime_ticks"])
            ds_tot = max(0, self._end_stat["stime_ticks"] - self._start_stat["stime_ticks"])
            user_mean = (du_tot / CLK_TCK) / elapsed * 100.0
            sys_mean = (ds_tot / CLK_TCK) / elapsed * 100.0
            cpu_mean = user_mean + sys_mean
            minflt_delta = max(0, self._end_stat["minflt"] - self._start_stat["minflt"])
            majflt_delta = max(0, self._end_stat["majflt"] - self._start_stat["majflt"])
        else:
            minflt_delta = 0
            majflt_delta = 0

        top_threads = []
        for tid, end_t in self._end_threads.items():
            start_t = self._start_threads.get(tid)
            du = end_t["utime_ticks"] - (start_t["utime_ticks"] if start_t else 0)
            ds = end_t["stime_ticks"] - (start_t["stime_ticks"] if start_t else 0)
            tot_pct = max(0.0, (du + ds) / CLK_TCK / elapsed * 100.0)
            top_threads.append(
                {
                    "tid": tid,
                    "comm": end_t["comm"],
                    "cpu_pct": round(tot_pct, 2),
                }
            )
        top_threads.sort(key=lambda item: item["cpu_pct"], reverse=True)

        softirq_delta = {
            k: max(0, end_softirqs.get(k, 0) - self._start_softirqs.get(k, 0))
            for k in ("NET_RX", "NET_TX", "TIMER", "SCHED")
        }
        softirq_per_sec = {k: round(v / elapsed, 2) for k, v in softirq_delta.items()}

        rss_mean = sum(self._rss_samples_mb) / len(self._rss_samples_mb) if self._rss_samples_mb else 0.0
        rss_max = max(self._rss_samples_mb) if self._rss_samples_mb else 0.0
        pss_mean = sum(self._pss_samples_mb) / len(self._pss_samples_mb) if self._pss_samples_mb else 0.0
        rss_drift_per_min = _linear_slope(self._samples_t, self._rss_samples_mb) * 60.0

        nic_rx_bytes = max(0, end_nic.get("rx_bytes", 0) - self._start_nic.get("rx_bytes", 0))
        nic_rx_packets = max(0, end_nic.get("rx_packets", 0) - self._start_nic.get("rx_packets", 0))
        nic_rx_dropped = max(0, end_nic.get("rx_dropped", 0) - self._start_nic.get("rx_dropped", 0))
        nic_rx_missed = max(
            0, end_nic.get("rx_missed_errors", 0) - self._start_nic.get("rx_missed_errors", 0)
        )
        nic_total_drops = nic_rx_dropped + nic_rx_missed
        nic_drop_rate = (
            (nic_total_drops / (nic_rx_packets + nic_total_drops) * 100.0)
            if (nic_rx_packets + nic_total_drops) > 0
            else 0.0
        )

        return {
            "elapsed_sec": round(elapsed, 3),
            "cpu": {
                "total_core_pct_mean": round(cpu_mean, 2),
                "total_core_pct_peak": round(cpu_peak, 2),
                "user_core_pct_mean": round(user_mean, 2),
                "sys_core_pct_mean": round(sys_mean, 2),
                "top_threads": top_threads[:8],
            },
            "softirqs": {
                "delta": softirq_delta,
                "per_sec": softirq_per_sec,
            },
            "memory": {
                "rss_mb_mean": round(rss_mean, 2),
                "rss_mb_max": round(rss_max, 2),
                "pss_mb_mean": round(pss_mean, 2),
                "rss_drift_mb_per_min": round(rss_drift_per_min, 3),
                "minflt_delta": minflt_delta,
                "majflt_delta": majflt_delta,
            },
            "io": {
                "nic": self.nic,
                "nic_rx_mbps_mean": round((nic_rx_bytes * 8.0) / (elapsed * 1e6), 3)
                if self.nic
                else 0.0,
                "nic_rx_mbps_peak": round(max(self._nic_mbps_samples), 3)
                if self._nic_mbps_samples
                else 0.0,
                "nic_rx_packets": nic_rx_packets,
                "nic_rx_dropped": nic_total_drops,
                "nic_drop_rate_pct": round(nic_drop_rate, 4),
                "udp_rcvbuf_drops_delta": max(0, end_udp_drops - self._start_udp_drops),
            },
        }


def parse_perf_stat_csv(stderr_text: str) -> dict[str, Any]:
    """Parse `perf stat -x ,` CSV output into cache miss metrics."""
    counters: dict[str, float] = {}
    for line in stderr_text.splitlines():
        parts = [p.strip() for p in line.split(",")]
        if len(parts) < 3:
            continue
        val_s, _, event = parts[0], parts[1], parts[2]
        if not val_s or val_s.startswith("<"):
            continue
        try:
            counters[event] = float(val_s)
        except ValueError:
            continue
    if not counters:
        return {"available": False, "reason": "perf counters unavailable or permission denied"}

    cache_refs = counters.get("cache-references", 0.0)
    cache_misses = counters.get("cache-misses", 0.0)
    llc_loads = counters.get("LLC-loads", 0.0)
    llc_misses = counters.get("LLC-load-misses", 0.0)
    return {
        "available": True,
        "cache_references": int(cache_refs),
        "cache_misses": int(cache_misses),
        "cache_miss_rate_pct": round((cache_misses / cache_refs) * 100.0, 3)
        if cache_refs > 0
        else None,
        "llc_loads": int(llc_loads),
        "llc_load_misses": int(llc_misses),
        "llc_miss_rate_pct": round((llc_misses / llc_loads) * 100.0, 3)
        if llc_loads > 0
        else None,
    }


class PerfStatSession:
    """Optional perf stat sidecar for cache-miss sampling."""

    def __init__(self, pid: int) -> None:
        self.pid = pid
        self._proc: subprocess.Popen[str] | None = None

    def start(self) -> None:
        perf_bin = shutil.which("perf")
        if not perf_bin:
            return
        cmd = [
            perf_bin,
            "stat",
            "-x",
            ",",
            "-p",
            str(self.pid),
            "-e",
            "cache-references,cache-misses,LLC-loads,LLC-load-misses",
        ]
        try:
            self._proc = subprocess.Popen(
                cmd,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                text=True,
            )
        except OSError:
            self._proc = None

    def stop(self) -> dict[str, Any]:
        if self._proc is None:
            return {"available": False, "reason": "perf binary not available"}
        try:
            self._proc.send_signal(signal.SIGINT)
            _, stderr = self._proc.communicate(timeout=3.0)
        except subprocess.TimeoutExpired:
            self._proc.kill()
            _, stderr = self._proc.communicate()
        return parse_perf_stat_csv(stderr or "")

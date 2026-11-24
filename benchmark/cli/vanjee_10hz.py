"""Run a non-reconfiguring, live VanJee 720-16 benchmark."""

from __future__ import annotations

import argparse
import html
import json
import os
from pathlib import Path
import shlex
import socket
import tempfile
import time

from cli.vanjee_packets import PacketObserver, PacketSequence
from cli.report import render_html
from cli.wheelos_sensor_bench import main as bench_main


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mainboard", required=True, help="Built Cyber mainboard executable.")
    parser.add_argument("--component", required=True, help="Built VanJee component .so.")
    parser.add_argument("--host-address", required=True)
    parser.add_argument("--lidar-address", required=True)
    parser.add_argument("--host-port", type=int, default=3001)
    parser.add_argument("--lidar-port", type=int, default=3333)
    parser.add_argument("--interface", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--driver-version", help="Driver source revision or release used for this run.")
    parser.add_argument("--duration", default="30m")
    parser.add_argument("--warmup", default="20s")
    parser.add_argument("--profile", default="orin_edge", choices=["orin_edge", "x86_ipc"])
    parser.add_argument("--no-perf", action="store_true")
    parser.add_argument("--window-file", type=Path, help="Shared window for an external packet observer.")
    parser.add_argument("--packet-result", type=Path, help="JSON written by an external packet observer.")
    args = parser.parse_args(argv)

    if bool(args.window_file) != bool(args.packet_result):
        parser.error("--window-file and --packet-result must be supplied together.")
    if args.window_file and (args.window_file.exists() or args.packet_result.exists()):
        parser.error("External window and packet result paths must not already exist.")
    for address in (args.host_address, args.lidar_address):
        socket.inet_pton(socket.AF_INET, address)
    if not (1 <= args.host_port <= 65535 and 1 <= args.lidar_port <= 65535):
        parser.error("Ports must be between 1 and 65535.")
    if not os.environ.get("CYBER_DOMAIN_ID") or not os.environ.get("CYBER_PATH"):
        parser.error("Set an isolated CYBER_DOMAIN_ID and CYBER_PATH before running.")
    mainboard = Path(args.mainboard).resolve(strict=True)
    component = Path(args.component).resolve(strict=True)
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    config = out / "vanjee-live.pb.txt"
    dag = out / "vanjee-live.dag"
    if any(path.exists() for path in (config, dag, out / "baseline.json")):
        parser.error("Output directory already contains a run; choose a fresh directory.")

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
        try:
            probe.bind((args.host_address, args.host_port))
        except OSError as exc:
            parser.error(f"Cannot safely bind the requested UDP address and port: {exc}")

    channel = f"/apollo/benchmark/vanjeelidar/{os.getpid()}/PointCloud2"
    q = json.dumps
    with config.open("x", encoding="utf-8") as file:
        file.write(f"""config_base {{
  scan_channel: {q(channel + "/Scan")}
  point_cloud_channel: {q(channel)}
  frame_id: "vanjeelidar_up"
  source_type: ONLINE_LIDAR
}}
imu_channel: {q(channel + "/Imu")}
imu_frame_id: "vanjeelidar_up_imu"
driver_param {{
  lidar_type: "vanjee_720_16"
  input_type: 1
  input_param {{
    connect_type: 1
    host_msop_port: {args.host_port}
    lidar_msop_port: {args.lidar_port}
    host_address: {q(args.host_address)}
    lidar_address: {q(args.lidar_address)}
  }}
  decoder_param {{
    publish_mode: 2
    min_distance: 0.3
    max_distance: 120
    start_angle: 0
    end_angle: 360
    use_lidar_clock: false
    use_offset_timestamp: true
    imu_enable: 0
    imu_orientation_enable: false
    dense_points: false
    wait_for_difop: true
    config_from_file: false
    point_cloud_enable: true
  }}
}}
""")
    with dag.open("x", encoding="utf-8") as file:
        file.write(f"""module_config {{
  module_library: {q(str(component))}
  components {{
    class_name: "VanjeelidarComponent"
    config {{
      name: "vanjee_10hz_benchmark"
      config_file_path: {q(str(config))}
    }}
  }}
}}
""")

    with tempfile.TemporaryDirectory(prefix=".vanjee-window-", dir=out) as tmp:
        window = args.window_file.resolve() if args.window_file else Path(tmp) / "window"
        observer = None
        if not args.packet_result:
            observer = PacketObserver(
                args.interface, window,
                PacketSequence(args.lidar_address, args.host_address, args.lidar_port, args.host_port),
            )
            observer.start()
        command = shlex.join([str(mainboard), "-d", str(dag), "-p", "vanjee_10hz_benchmark"])
        bench_args = [
            "run", "--device", "vanjee_720_16_live", "--cmd", command,
            "--channel", channel, "--msg-type", "pointcloud",
            "--warmup", args.warmup, "--duration", args.duration,
            "--nic", args.interface, "--profile", args.profile, "--out", str(out),
            "--window-file", str(window),
        ]
        if args.no_perf:
            bench_args.append("--no-perf")
        if args.driver_version:
            bench_args.extend(["--driver-version", args.driver_version])
        try:
            result = bench_main(bench_args)
        finally:
            packets = observer.finish() if observer else {"available": False}

    if args.packet_result:
        deadline = time.monotonic() + 10
        while not args.packet_result.is_file() and time.monotonic() < deadline:
            time.sleep(0.1)
        if args.packet_result.is_file():
            packets = json.loads(args.packet_result.read_text(encoding="utf-8"))
        else:
            packets = {"available": False, "reason": "External packet observer produced no result."}

    baseline_path = out / "baseline.json"
    baseline = json.loads(baseline_path.read_text(encoding="utf-8"))
    baseline["vanjee_packet_observation"] = packets
    baseline["vanjee_run"] = {
        "host_address": args.host_address, "lidar_address": args.lidar_address,
        "host_port": args.host_port, "lidar_port": args.lidar_port,
        "interface": args.interface, "model": "vanjee_720_16",
        "imu_enabled": True, "use_lidar_clock": False,
    }
    baseline_path.write_text(json.dumps(baseline, indent=2) + "\n", encoding="utf-8")
    observation = (
        f"Observed {packets['packets']} packets; sequence gaps estimate "
        f"{packets['missing_sequence_estimate']}, duplicates {packets['duplicates']}, "
        f"out-of-order {packets['out_of_order']}. Capture socket drops "
        f"{packets['capture_socket_drops']} (whole observer lifetime)."
        if packets.get("available")
        else f"Unavailable: {packets.get('reason', 'not started')}."
    )
    caveat = (
        "Packet observation is passive and does not prove delivery to the driver. "
        "Device clock synchronization and IMU output are not verified."
    )
    page = render_html([baseline])
    page = page.replace(
        "</body>",
        f"<h2>VanJee UDP observation</h2><p>{html.escape(observation)}</p>"
        f"<p>{html.escape(caveat)}</p></body>",
    )
    (out / "report.html").write_text(page, encoding="utf-8")
    with (out / "report.md").open("a", encoding="utf-8") as file:
        file.write(f"\n## VanJee UDP observation\n\n{observation}\n\n{caveat}\n")
    if (baseline.get("channel_metrics") or {}).get("message_count", 0) == 0:
        print("No point-cloud messages observed; this is not a valid live baseline.")
        return 1
    return result


if __name__ == "__main__":
    raise SystemExit(main())

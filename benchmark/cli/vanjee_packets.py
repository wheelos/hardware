"""Passive 720-series UDP packet-sequence observation."""

from __future__ import annotations

import argparse
import json
import mmap
import socket
import struct
import threading
import time
from pathlib import Path
from typing import Any


class PacketSequence:
    def __init__(self, source: str, destination: str, source_port: int, port: int):
        self.source = socket.inet_aton(source)
        self.destination = socket.inet_aton(destination)
        self.source_port = source_port
        self.port = port
        self.packets = 0
        self.missing = 0
        self.duplicates = 0
        self.out_of_order = 0
        self.last: int | None = None

    def record(self, frame: bytes) -> None:
        if len(frame) < 42 or frame[12:14] != b"\x08\x00":
            return
        ihl = (frame[14] & 15) * 4
        offset = 14 + ihl
        if (ihl < 20 or len(frame) < offset + 8 or frame[23] != 17
                or frame[26:30] != self.source or frame[30:34] != self.destination
                or struct.unpack_from("!H", frame, 20)[0] & 0x3fff):
            return
        source_port, port, length, _ = struct.unpack_from("!HHHH", frame, offset)
        payload = frame[offset + 8:]
        if (source_port != self.source_port or port != self.port or length != 1261
                or len(payload) < 1253 or payload[2:5] != b"\x10\x01\x12"):
            return
        sequence = struct.unpack_from("!H", payload, 1249)[0]
        self.packets += 1
        if self.last is not None:
            diff = (sequence - self.last) & 0xffff
            if diff == 0:
                self.duplicates += 1
                return
            if diff > 32768:
                self.out_of_order += 1
                return
            self.missing += diff - 1
        self.last = sequence

    def result(self) -> dict[str, Any]:
        return {
            "packets": self.packets,
            "missing_sequence_estimate": self.missing,
            "duplicates": self.duplicates,
            "out_of_order": self.out_of_order,
        }


class PacketObserver:
    def __init__(self, interface: str, window_path: Path, sequence: PacketSequence):
        self.interface = interface
        self.window_path = window_path
        self.sequence = sequence
        self.ready = threading.Event()
        self.stop = threading.Event()
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.result: dict[str, Any] = {"available": False}

    def start(self) -> None:
        self.thread.start()
        self.ready.wait()

    def finish(self) -> dict[str, Any]:
        self.stop.set()
        self.thread.join()
        return self.result

    def _run(self) -> None:
        sock = None
        try:
            sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
            sock.bind((self.interface, 0))
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4 * 1024 * 1024)
            sock.settimeout(0.1)
        except OSError as exc:
            if sock is not None:
                sock.close()
            self.result = {"available": False, "reason": str(exc)}
            self.ready.set()
            return
        self.ready.set()
        mapping = None
        try:
            while not self.stop.is_set():
                if mapping is None and self.window_path.is_file():
                    with self.window_path.open("rb") as file:
                        if self.window_path.stat().st_size >= 16:
                            mapping = mmap.mmap(file.fileno(), 16, access=mmap.ACCESS_READ)
                try:
                    frame = sock.recv(65535)
                except socket.timeout:
                    continue
                now = time.monotonic_ns()
                if mapping is not None:
                    start, end = struct.unpack_from("=QQ", mapping)
                    if start and start <= now < end:
                        self.sequence.record(frame)
            packet_stats = sock.getsockopt(263, 6, 8)  # SOL_PACKET, PACKET_STATISTICS
            _, capture_drops = struct.unpack("=II", packet_stats)
            self.result = {
                "available": True,
                "interface": self.interface,
                "capture_socket_drops": capture_drops,
                **self.sequence.result(),
            }
        except OSError as exc:
            self.result = {"available": False, "reason": str(exc)}
        finally:
            if mapping is not None:
                mapping.close()
            sock.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--interface", required=True)
    parser.add_argument("--source", required=True)
    parser.add_argument("--destination", required=True)
    parser.add_argument("--source-port", type=int, default=3333)
    parser.add_argument("--port", type=int, default=3001)
    parser.add_argument("--window-file", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--ready-file", type=Path, required=True)
    parser.add_argument("--timeout", type=float, default=1900)
    args = parser.parse_args()
    observer = PacketObserver(
        args.interface, args.window_file,
        PacketSequence(args.source, args.destination, args.source_port, args.port),
    )
    observer.start()
    args.ready_file.write_text("ready\n", encoding="utf-8")
    deadline = time.monotonic() + args.timeout
    completed = False
    try:
        while observer.thread.is_alive() and time.monotonic() < deadline:
            if args.window_file.is_file():
                with args.window_file.open("rb") as file:
                    data = file.read(16)
                if len(data) == 16:
                    start, end = struct.unpack("=QQ", data)
                    if start and end > start and time.monotonic_ns() >= end:
                        completed = True
                        break
            time.sleep(0.1)
    finally:
        result = observer.finish()
        if result.get("available") and not completed:
            result = {"available": False, "reason": "Measurement window did not finish before observer timeout."}
        temp_out = args.out.with_name(args.out.name + ".tmp")
        with temp_out.open("x", encoding="utf-8") as file:
            file.write(json.dumps(result, indent=2) + "\n")
        temp_out.replace(args.out)
    return 0 if result.get("available") else 1


if __name__ == "__main__":
    raise SystemExit(main())

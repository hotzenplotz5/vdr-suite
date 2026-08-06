#!/usr/bin/env python3
"""Loopback-only SVDRP relay with one acceptance-only response-loss gate."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import signal
import socket
import threading
from typing import BinaryIO

MAX_LINE = 65536
STOP = threading.Event()


def read_line(stream: BinaryIO) -> bytes:
    value = stream.readline(MAX_LINE + 1)
    if not value or len(value) > MAX_LINE or not value.endswith(b"\n"):
        raise RuntimeError("invalid or incomplete SVDRP line")
    return value


def read_reply(stream: BinaryIO) -> bytes:
    first = read_line(stream)
    if len(first) < 5 or not first[:3].isdigit() or first[3:4] not in (b" ", b"-"):
        raise RuntimeError("malformed SVDRP reply")
    code = first[:3]
    lines = [first]
    if first[3:4] == b" ":
        return b"".join(lines)
    while True:
        line = read_line(stream)
        lines.append(line)
        if line.startswith(code + b" "):
            return b"".join(lines)
        if not line.startswith(code + b"-"):
            raise RuntimeError("inconsistent SVDRP multiline reply")


def append_log(path: Path, event: str, request: bytes, response: bytes) -> None:
    record = {
        "event": event,
        "request": request.decode("utf-8", "strict").rstrip("\r\n"),
        "response": response.decode("utf-8", "strict").rstrip("\r\n"),
    }
    with path.open("a", encoding="utf-8") as output:
        output.write(json.dumps(record, separators=(",", ":"), sort_keys=True))
        output.write("\n")
        output.flush()


def consume_drop_budget(path: Path) -> bool:
    try:
        value = int(path.read_text(encoding="ascii").strip() or "0")
    except (FileNotFoundError, ValueError):
        value = 0
    if value <= 0:
        return False
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(f"{value - 1}\n", encoding="ascii")
    temporary.replace(path)
    return True


def serve_client(
    client: socket.socket,
    upstream_host: str,
    upstream_port: int,
    drop_file: Path,
    log_file: Path,
) -> None:
    try:
        with client, socket.create_connection(
            (upstream_host, upstream_port), timeout=3.0
        ) as upstream:
            client.settimeout(5.0)
            upstream.settimeout(5.0)
            client_stream = client.makefile("rb", buffering=0)
            upstream_stream = upstream.makefile("rb", buffering=0)
            greeting = read_reply(upstream_stream)
            client.sendall(greeting)
            request = read_line(client_stream)
            upstream.sendall(request)
            response = read_reply(upstream_stream)
            is_execute = b" PLUG suitebridge NPROBE EXEC " in b" " + request
            if is_execute and consume_drop_budget(drop_file):
                append_log(log_file, "drop", request, response)
                return
            append_log(log_file, "relay", request, response)
            client.sendall(response)
    except (OSError, RuntimeError, UnicodeError) as error:
        with log_file.open("a", encoding="utf-8") as output:
            output.write(json.dumps({
                "event": "error",
                "diagnostic": type(error).__name__,
            }, separators=(",", ":"), sort_keys=True))
            output.write("\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--listen-host", default="127.0.0.1")
    parser.add_argument("--listen-port", type=int, required=True)
    parser.add_argument("--upstream-host", default="127.0.0.1")
    parser.add_argument("--upstream-port", type=int, required=True)
    parser.add_argument("--drop-file", type=Path, required=True)
    parser.add_argument("--log-file", type=Path, required=True)
    parser.add_argument("--ready-file", type=Path, required=True)
    parser.add_argument("--stop-file", type=Path, required=True)
    args = parser.parse_args()
    if args.listen_host not in {"127.0.0.1", "::1"} or \
            args.upstream_host not in {"127.0.0.1", "::1"}:
        raise SystemExit("loopback hosts required")
    if not (1 <= args.listen_port <= 65535 and
            1 <= args.upstream_port <= 65535):
        raise SystemExit("valid ports required")

    args.log_file.parent.mkdir(parents=True, exist_ok=True)
    args.drop_file.write_text("0\n", encoding="ascii")
    args.stop_file.unlink(missing_ok=True)
    args.ready_file.unlink(missing_ok=True)

    family = socket.AF_INET6 if args.listen_host == "::1" else socket.AF_INET
    with socket.socket(family, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((args.listen_host, args.listen_port))
        server.listen(16)
        server.settimeout(0.5)
        args.ready_file.write_text("ready\n", encoding="ascii")
        while not STOP.is_set() and not args.stop_file.exists():
            try:
                client, _ = server.accept()
            except TimeoutError:
                continue
            serve_client(
                client,
                args.upstream_host,
                args.upstream_port,
                args.drop_file,
                args.log_file,
            )
    return 0


if __name__ == "__main__":
    signal.signal(signal.SIGTERM, lambda *_: STOP.set())
    signal.signal(signal.SIGINT, lambda *_: STOP.set())
    raise SystemExit(main())

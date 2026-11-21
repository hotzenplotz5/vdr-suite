#!/usr/bin/env python3

import argparse
import base64
import json
import socket
import sys
import urllib.error
import urllib.parse
import urllib.request
from typing import Any

MAX_SVDRP_BYTES = 8192
MAX_HTTP_BYTES = 1024 * 1024


def read_line(sock: socket.socket, buffer: bytearray) -> bytes:
    while b"\n" not in buffer:
        chunk = sock.recv(1024)
        if not chunk:
            raise RuntimeError("SVDRP connection closed before a complete line")
        buffer.extend(chunk)
        if len(buffer) > MAX_SVDRP_BYTES:
            raise RuntimeError("SVDRP reply exceeds bounded size")

    line, remainder = buffer.split(b"\n", 1)
    buffer[:] = remainder
    return line.rstrip(b"\r")


def parse_svdrp_line(line: bytes) -> tuple[int, str, str]:
    if len(line) < 4 or not line[:3].isdigit():
        raise RuntimeError("invalid SVDRP reply line")

    separator = chr(line[3])
    if separator not in (" ", "-"):
        raise RuntimeError("invalid SVDRP reply separator")

    return int(line[:3]), separator, line[4:].decode("utf-8", errors="strict")


def svdrp_request(host: str, port: int, command: str, timeout: float) -> tuple[int, str]:
    with socket.create_connection((host, port), timeout=timeout) as sock:
        sock.settimeout(timeout)
        buffer = bytearray()
        greeting_code, greeting_separator, _ = parse_svdrp_line(
            read_line(sock, buffer)
        )
        if greeting_code != 220 or greeting_separator != " ":
            raise RuntimeError(f"unexpected SVDRP greeting: {greeting_code}")

        sock.sendall((command + "\r\n").encode("ascii"))

        reply_code = 0
        payload_lines: list[str] = []
        while True:
            code, separator, payload = parse_svdrp_line(read_line(sock, buffer))
            if reply_code == 0:
                reply_code = code
            elif code != reply_code:
                raise RuntimeError("inconsistent SVDRP multiline reply code")

            payload_lines.append(payload)
            if separator == " ":
                break
            if len(payload_lines) >= 64:
                raise RuntimeError("SVDRP reply exceeds bounded line count")

        return reply_code, "\n".join(payload_lines)


def http_request(
    url: str,
    username: str,
    password: str,
    timeout: float,
) -> tuple[int, str, bytes]:
    headers = {"Accept": "application/json"}
    if username or password:
        token = base64.b64encode(
            f"{username}:{password}".encode("utf-8")
        ).decode("ascii")
        headers["Authorization"] = "Basic " + token

    request = urllib.request.Request(url, headers=headers, method="GET")
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            body = response.read(MAX_HTTP_BYTES + 1)
            if len(body) > MAX_HTTP_BYTES:
                raise RuntimeError("HTTP response exceeds bounded size")
            return response.status, response.headers.get_content_type(), body
    except urllib.error.HTTPError as error:
        body = error.read(MAX_HTTP_BYTES + 1)
        return error.code, error.headers.get_content_type(), body


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def parse_json_document(payload: str, source: str) -> dict[str, Any]:
    try:
        document = json.loads(payload)
    except json.JSONDecodeError as error:
        raise RuntimeError(f"{source} returned invalid JSON: {error}") from error

    require(isinstance(document, dict), f"{source} JSON must be an object")
    return document


def public_image_urls(document: dict[str, Any]) -> list[str]:
    urls: list[str] = []

    preferred = document.get("preferredArtwork")
    if isinstance(preferred, dict) and preferred.get("available") is True:
        url = preferred.get("url")
        if isinstance(url, str) and url:
            urls.append(url)

    people = document.get("people")
    if isinstance(people, list):
        for person in people:
            if not isinstance(person, dict):
                continue
            image = person.get("image")
            if isinstance(image, dict) and image.get("available") is True:
                url = image.get("url")
                if isinstance(url, str) and url:
                    urls.append(url)

    images = document.get("images")
    if isinstance(images, list):
        for entry in images:
            if not isinstance(entry, dict):
                continue
            image = entry.get("image")
            if isinstance(image, dict) and image.get("available") is True:
                url = image.get("url")
                if isinstance(url, str) and url:
                    urls.append(url)

    return urls


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Run a bounded read-only live smoke test for Suite Bridge EPG "
            "scraper metadata and the public VDR-Suite API."
        )
    )
    parser.add_argument("--channel-id", required=True)
    parser.add_argument("--event-id", required=True)
    parser.add_argument("--backend", default="default")
    parser.add_argument("--svdrp-host", default="127.0.0.1")
    parser.add_argument("--svdrp-port", type=int, default=6419)
    parser.add_argument("--http-base", default="http://127.0.0.1:18080")
    parser.add_argument("--username", default="admin")
    parser.add_argument("--password", default="vdr-suite")
    parser.add_argument("--timeout", type=float, default=5.0)
    args = parser.parse_args()

    require(args.svdrp_port > 0 and args.svdrp_port <= 65535, "invalid SVDRP port")
    require(args.timeout > 0, "timeout must be positive")

    command = f"PLUG suitebridge META {args.channel_id} {args.event_id}"
    reply_code, plugin_payload = svdrp_request(
        args.svdrp_host,
        args.svdrp_port,
        command,
        args.timeout,
    )
    require(reply_code == 250, f"META returned SVDRP {reply_code}: {plugin_payload}")

    plugin_document = parse_json_document(plugin_payload, "Suite Bridge META")
    require(plugin_document.get("schema") == 1, "META schema is not 1")
    require(
        plugin_document.get("provider") in ("tvscraper", "none"),
        "META provider is invalid",
    )
    require(isinstance(plugin_document.get("found"), bool), "META found flag is missing")

    query = urllib.parse.urlencode(
        {
            "backend": args.backend,
            "channelId": args.channel_id,
            "eventId": args.event_id,
        }
    )
    metadata_url = args.http_base.rstrip("/") + "/api/epg/cache/metadata?" + query
    status, content_type, body = http_request(
        metadata_url,
        args.username,
        args.password,
        args.timeout,
    )
    require(status == 200, f"public metadata API returned HTTP {status}: {body!r}")
    require(content_type == "application/json", "public metadata API did not return JSON")

    public_payload = body.decode("utf-8", errors="strict")
    public_document = parse_json_document(public_payload, "public metadata API")
    require("/var/cache/" not in public_payload, "public JSON leaks a private cache path")
    require("\"path\"" not in public_payload, "public JSON exposes a private path field")

    plugin_found = plugin_document.get("found") is True
    public_available = public_document.get("available") is True
    require(
        plugin_found == public_available,
        "Suite Bridge and public API disagree about metadata availability",
    )

    urls = public_image_urls(public_document)
    for url in urls:
        require(url.startswith("/api/epg/cache/metadata/image?"), "invalid public image URL")

    if urls:
        image_url = urllib.parse.urljoin(args.http_base.rstrip("/") + "/", urls[0])
        image_status, image_type, image_body = http_request(
            image_url,
            args.username,
            args.password,
            args.timeout,
        )
        require(image_status == 200, f"public metadata image returned HTTP {image_status}")
        require(image_type in ("image/jpeg", "image/png"), "unsupported public image type")
        require(len(image_body) > 0, "public metadata image is empty")

    print("PASS: Suite Bridge META and public EPG metadata API are consistent")
    print(f"found={str(plugin_found).lower()}")
    print(f"people={len(public_document.get('people', []))}")
    print(f"images={len(public_document.get('images', []))}")
    print(f"publicImageUrls={len(urls)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, OSError, UnicodeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)

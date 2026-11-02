#!/usr/bin/env python3
"""MLD package staging only: configure existing Suite Agent and daemon files.

Called by the MLD recipe after make install-runtime DESTDIR=... and before
creating/installing the package. Never modifies the live /etc directory.
The recipe, not a browser request, owns initial privileged onboarding.
"""
import argparse
import os
from pathlib import Path
import stat
import sys
import tempfile
from urllib.parse import urlsplit

from vdr_suite_backend_agent_autostart import NotReady, read_config

PLACEHOLDER = "https://control-plane.example.invalid"
AGENT_REL = Path("etc/vdr-suite/backend-agent.conf")
DAEMON_REL = Path("etc/default/vdr-suite-daemon")


def checked_staging_file(root, rel):
    path = root / rel
    # DESTDIR must not contain links escaping into the host's real /etc.
    # Checking only the leaf would allow an intermediate /etc symlink.
    current = root
    for part in rel.parts:
        current = current / part
        if current.is_symlink():
            raise NotReady("linked package staging path: " + str(rel))
    if not path.is_file() or not path.resolve().is_relative_to(root.resolve()):
        raise NotReady("missing or escaped staged package file: " + str(rel))
    return path


def validated_origin(url):
    if not url or any(c.isspace() or ord(c) < 32 for c in url):
        raise NotReady("MLD package must supply a trusted HTTPS Control Plane origin")
    try:
        parsed = urlsplit(url)
        # No implicit remote host, credentials, paths, query, or fragments.
        if (parsed.scheme != "https" or not parsed.hostname or
                parsed.hostname.endswith(".invalid") or
                parsed.username or parsed.password or parsed.query or
                parsed.fragment or parsed.path not in ("", "/") or
                parsed.port is not None and not 1 <= parsed.port <= 65535):
            raise NotReady("MLD Control Plane origin must be an HTTPS origin")
    except ValueError as exc:
        raise NotReady("invalid MLD Control Plane origin") from exc
    return url.rstrip("/")


def validated_channels(value):
    if (not value or any(c.isspace() or ord(c) < 32 for c in value)
            or not value.startswith("/") or "//" in value or
            any(segment in (".", "..") for segment in value.split("/"))):
        raise NotReady("MLD channel source must be a stable absolute VDR path")
    # This source is installed by VDR, not by the Suite package. A symlink
    # to the user's selected MLD channel list is valid at runtime.
    return value


def settings(text):
    items = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise NotReady("invalid staged Agent configuration")
        k, v = line.split("=", 1)
        if k in items:
            raise NotReady("duplicate staged Agent config: " + k)
        items[k] = v.strip()
    return items


def substitute(source, updates):
    lines = source.splitlines(keepends=True)
    seen = set()
    for i, line in enumerate(lines):
        key = line.split("=", 1)[0].strip()
        if not key.startswith("#") and key in updates:
            if key in seen:
                raise NotReady("duplicate staged Agent config: " + key)
            seen.add(key)
            lines[i] = key + "=" + updates[key] + "\n"
    if seen != set(updates):
        raise NotReady("staged Agent config is missing expected keys")
    return "".join(lines)


def update_atomically(path, content):
    previous = path.read_text(encoding="utf-8")
    if previous == content:
        return False
    details = path.stat()
    if not stat.S_ISREG(details.st_mode) or details.st_mode & 0o022:
        raise NotReady("staged package file permissions are unsafe")
    temp = None
    try:
        with tempfile.NamedTemporaryFile(
                mode="w", encoding="utf-8", dir=path.parent,
                prefix=".mld-first-run-", delete=False) as output:
            temp = Path(output.name)
            os.fchmod(output.fileno(), stat.S_IMODE(details.st_mode))
            output.write(content)
            output.flush()
            os.fsync(output.fileno())
        os.replace(temp, path)
    finally:
        if temp is not None and temp.exists():
            temp.unlink()
    return True


def stage(root, origin, channels):
    root = Path(root)
    # Never change a live installation by inadvertently accepting /.
    if (not root.is_absolute() or root.is_symlink() or
            root.resolve() == Path("/") or not root.is_dir()):
        raise NotReady("a separate absolute DESTDIR package staging root is required")
    origin = validated_origin(origin)
    channels = validated_channels(channels)
    config = checked_staging_file(root, AGENT_REL)
    daemon = checked_staging_file(root, DAEMON_REL)
    content = config.read_text(encoding="utf-8")
    fields = settings(content)
    if fields.get("BACKEND_ID") != "default":
        raise NotReady("MLD first-run expects the existing local default Backend")
    if fields.get("CONTROL_PLANE_URL") not in (PLACEHOLDER, origin):
        raise NotReady("do not replace an existing configured Control Plane")
    if fields.get("CHANNELS_CONF_PATH") not in (
            "/var/lib/vdr/channels.conf", channels):
        raise NotReady("do not replace an existing selected VDR channel source")
    adapters = [item.strip() for item in fields.get("ADAPTERS", "").split(",")
                if item.strip()]
    observations = [item.strip() for item in
                    fields.get("OBSERVATION_DOMAINS", "").split(",")
                    if item.strip()]
    if "channels-conf" not in adapters:
        adapters.append("channels-conf")
    if "backend-health" not in observations:
        observations.append("backend-health")
    if "channels" not in observations:
        observations.append("channels")
    next_config = substitute(content, {
        "CONTROL_PLANE_URL": origin,
        "CHANNELS_CONF_PATH": channels,
        "ADAPTERS": ",".join(adapters),
        "OBSERVATION_DOMAINS": ",".join(observations),
    })
    # Reuse the Agent's existing HTTPS/identity validation before staging.
    with tempfile.TemporaryDirectory() as tmp:
        candidate = Path(tmp) / "backend-agent.conf"
        candidate.write_text(next_config, encoding="utf-8")
        read_config(candidate)
    daemon_content = daemon.read_text(encoding="utf-8")
    if "VDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=" in daemon_content:
        if any(line.strip() not in (
                "VDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=true",)
                for line in daemon_content.splitlines()
                if line.strip().startswith(
                    "VDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=")):
            raise NotReady("do not override existing Suite daemon setup policy")
        next_daemon = daemon_content
    else:
        next_daemon = daemon_content.rstrip("\n") + (
            "\nVDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=true\n")
    # Inspect and validate both inputs before touching either staged file.
    for path in (config, daemon):
        info = path.stat()
        if not stat.S_ISREG(info.st_mode) or info.st_mode & 0o022:
            raise NotReady("unsafe staged package config permissions")
    update_atomically(config, next_config)
    update_atomically(daemon, next_daemon)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage-root", required=True)
    parser.add_argument("--control-plane-url", required=True)
    parser.add_argument("--channels-source", required=True)
    args = parser.parse_args()
    try:
        stage(args.stage_root, args.control_plane_url, args.channels_source)
    except (NotReady, OSError, UnicodeError) as exc:
        print("MLD_FIRST_RUN_STAGE=BLOCKED: " + str(exc), file=sys.stderr)
        return 1
    print("MLD_FIRST_RUN_STAGE=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

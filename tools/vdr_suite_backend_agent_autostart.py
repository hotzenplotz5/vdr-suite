#!/usr/bin/env python3
"""Enable Backend Agent boot startup only after an existing enrollment is healthy.

No enrollment, service start/restart, direct SQL, or secret output. The existing
Agent admin --status utility performs its normal repository initialization.
Current readiness lookup requires a same-host Control Plane database; remote
Agent hosts fail closed until their authenticated status path is implemented.
Called by the operator's post-enrollment setup flow, never by dpkg postinst.
"""
import argparse
import json
import os
from pathlib import Path
import pwd
import re
import stat
import subprocess
import sys
import time
from urllib.parse import urlsplit

UNIT = "vdr-suite-backend-agent.service"
CONFIG = Path("/etc/vdr-suite/backend-agent.conf")
IDENTITY = "/var/lib/vdr-suite/backend-agent/identity"
ENROLLMENT = "/var/lib/vdr-suite/backend-agent/enrollment"
AGENT_BIN = "/usr/sbin/vdr-suite-backend-agent"
ADMIN_BIN = "/usr/sbin/vdr-suite-backend-agent-admin"


class NotReady(Exception):
    pass


def command(argv):
    try:
        return subprocess.run(argv, check=True, capture_output=True, text=True,
                              timeout=15).stdout.strip()
    except (OSError, subprocess.SubprocessError) as exc:
        raise NotReady("required installed command failed") from exc


def properties():
    names = ("LoadState", "UnitFileState", "ActiveState", "SubState",
             "User", "Group", "ExecStart", "FragmentPath", "DropInPaths")
    out = command(["systemctl", "show", UNIT] +
                  [option for name in names for option in ("-p", name)])
    values = dict(line.split("=", 1) for line in out.splitlines() if "=" in line)
    if any(name not in values for name in names):
        raise NotReady("incomplete systemd unit inspection")
    return values


def read_config(path=CONFIG):
    if path.is_symlink() or not path.is_file():
        raise NotReady("Agent configuration missing or symlinked")
    values = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise NotReady("invalid Agent configuration")
        key, value = line.split("=", 1)
        if key in values:
            raise NotReady("duplicate Agent configuration key")
        values[key] = value.strip()
    if values.get("IDENTITY_PATH") != IDENTITY or (
            values.get("ENROLLMENT_PATH") != ENROLLMENT):
        raise NotReady("unexpected Agent identity/enrollment paths")
    backend = values.get("BACKEND_ID", "")
    if not re.fullmatch(r"[A-Za-z0-9._-]{1,128}", backend):
        raise NotReady("invalid configured Backend ID")
    origin = values.get("CONTROL_PLANE_URL", "")
    parsed = urlsplit(origin)
    if (parsed.scheme != "https" or not parsed.hostname or parsed.username or
            parsed.password or parsed.fragment or parsed.query or
            parsed.hostname.endswith(".invalid") or
            parsed.hostname == "example.invalid" or any(
                character.isspace() for character in origin)):
        raise NotReady("real HTTPS Control Plane is not configured")
    return backend


def check_identity(path=Path(IDENTITY), enrollment=Path(ENROLLMENT)):
    if enrollment.exists() or enrollment.is_symlink():
        raise NotReady("enrollment package remains unconsumed")
    try:
        details = path.lstat()
    except OSError as exc:
        raise NotReady("no persisted Agent identity") from exc
    if not stat.S_ISREG(details.st_mode) or details.st_mode & 0o077:
        raise NotReady("Agent identity missing or not protected")
    if details.st_uid != pwd.getpwnam("vdr").pw_uid or details.st_nlink != 1:
        raise NotReady("Agent identity has unexpected owner or file links")
    # Read only the two non-secret identity fields. Ignore credential_secret.
    fields = {}
    try:
        with path.open("rb") as stored:
            for raw in stored:
                if raw.startswith((b"agent_id=", b"backend_id=")):
                    key, value = raw.rstrip(b"\r\n").split(b"=", 1)
                    field = key.decode("ascii")
                    if field in fields:
                        raise NotReady("duplicate protected identity field")
                    fields[field] = value.decode("ascii")
    except (OSError, UnicodeError, ValueError) as exc:
        raise NotReady("unreadable protected Agent identity") from exc
    if (not re.fullmatch(r"[A-Za-z0-9._:-]{1,128}",
                         fields.get("agent_id", "")) or
            not re.fullmatch(r"[A-Za-z0-9._-]{1,128}",
                             fields.get("backend_id", ""))):
        raise NotReady("protected Agent identity fields invalid")
    return fields["agent_id"], fields["backend_id"]


def read_status(backend):
    try:
        status = json.loads(command(
            [ADMIN_BIN, "--backend", backend, "--status"]))
    except (ValueError, TypeError) as exc:
        raise NotReady("invalid Agent status response") from exc
    now = int(time.time())
    health = status.get("backendHealthObservation") or {}
    if (status.get("present") is not True or
            status.get("backendId") != backend or
            status.get("state") != "online" or
            not isinstance(status.get("agentId"), str) or
            not status["agentId"] or
            type(status.get("backendGeneration")) is not int or
            status["backendGeneration"] < 1 or
            type(status.get("heartbeatSequence")) is not int or
            status["heartbeatSequence"] < 1 or
            type(status.get("leaseExpiresAt")) is not int or
            status["leaseExpiresAt"] <= now + 15 or
            type(status.get("lastHeartbeatAt")) is not int or
            not now - 150 <= status["lastHeartbeatAt"] <= now + 5 or
            health.get("present") is not True or
            health.get("backendGeneration") != status["backendGeneration"] or
            type(health.get("acceptedAt")) is not int or
            not now - 150 <= health["acceptedAt"] <= now + 5):
        raise NotReady("Agent not enrolled, online, or recently observed")
    return status["agentId"]


def preflight():
    unit = properties()
    if (unit["LoadState"] != "loaded" or
            unit["ActiveState"] != "active" or
            unit["SubState"] != "running" or
            unit["UnitFileState"] not in ("disabled", "enabled") or
            unit["User"] != "vdr" or unit["Group"] != "vdr" or
            unit["DropInPaths"] or
            unit["FragmentPath"] not in (
                "/usr/lib/systemd/system/" + UNIT,
                "/lib/systemd/system/" + UNIT) or
            AGENT_BIN not in unit["ExecStart"] or
            "--config " + str(CONFIG) not in unit["ExecStart"]):
        raise NotReady("installed Agent unit differs from reviewed running unit")
    backend = read_config()
    agent_id, identity_backend = check_identity()
    if identity_backend != backend or read_status(backend) != agent_id:
        raise NotReady("Agent status does not match local identity")
    return unit["UnitFileState"], agent_id, backend


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    choice = parser.add_mutually_exclusive_group(required=True)
    choice.add_argument("--check", action="store_true",
                        help="readiness check only; never change systemd")
    choice.add_argument("--enable", action="store_true",
                        help="after enrollment, enable boot startup without restart")
    args = parser.parse_args()
    try:
        if args.enable and os.geteuid() != 0:
            raise NotReady("root required for systemd enable")
        state, agent_id, backend = preflight()
        if args.check:
            print("AGENT_BOOT_PREFLIGHT=PASS")
            print("AGENT_AUTOSTART=" + state)
            return 0
        # Recheck immediately before changing the persistent boot policy.
        state2, agent_id2, backend2 = preflight()
        if agent_id2 != agent_id or backend2 != backend or state2 != state:
            raise NotReady("Agent identity or systemd state changed")
        if state == "disabled":
            command(["systemctl", "enable", UNIT])  # Deliberately NOT --now.
        if command(["systemctl", "is-enabled", UNIT]) != "enabled" or (
                command(["systemctl", "is-active", UNIT]) != "active"):
            raise NotReady("could not verify enabled and still-running service")
        print("AGENT_AUTOSTART=ENABLED")
        print("AGENT_RESTART=NOT_REQUESTED")
        return 0
    except NotReady as exc:
        print("AGENT_BOOT_PREFLIGHT=BLOCKED: " + str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

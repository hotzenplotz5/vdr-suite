#!/usr/bin/env python3
"""Enable Backend Agent boot startup only after an existing enrollment is healthy.

Boot activation performs no enrollment or service start/restart. An optional\ninstaller-only --prepare-enrollment step reuses the existing enrollment binary\nand never prints credentials. No direct SQL or secret output. The existing
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
ENROLL_BIN = "/usr/sbin/vdr-suite-backend-agent-enroll"
DATABASE = "/var/lib/vdr-suite/vdr-suite.db"
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


def first_run_service_state(unit):
    """Read-only service discovery; no install, enroll, systemctl mutation."""
    properties = command(["systemctl", "show", unit, "-p", "LoadState",
                          "-p", "ActiveState"])
    values = dict(line.split("=", 1) for line in properties.splitlines()
                  if "=" in line)
    if values.get("LoadState") != "loaded":
        return "NOT_INSTALLED"
    return "RUNNING" if values.get("ActiveState") == "active" else "NOT_RUNNING"


def first_run_agent_state():
    """Classify local setup without reading or changing protected credentials.

    This is an internal preflight for a later installer-owned setup flow,
    not an end-user command and not proof that the Agent is connected.
    """
    if first_run_service_state("vdr.service") != "RUNNING":
        return "VDR_UNAVAILABLE"
    if first_run_service_state("vdr-suite-daemon.service") != "RUNNING":
        return "DAEMON_UNAVAILABLE"
    if first_run_service_state(UNIT) == "NOT_INSTALLED":
        return "AGENT_NOT_INSTALLED"
    if CONFIG.is_symlink() or not CONFIG.is_file():
        return "CONFIGURATION_REQUIRED"
    # An existing enrollment/identity must never be overwritten or
    # automatically recreated after an interrupted first run.
    identity = Path(IDENTITY)
    enrollment = Path(ENROLLMENT)
    if identity.is_symlink() or enrollment.is_symlink():
        return "UNEXPECTED_AGENT_STATE"
    if identity.is_file():
        return "IDENTITY_PRESENT"
    if enrollment.is_file():
        return "ENROLLMENT_PENDING"
    if identity.exists() or enrollment.exists():
        return "UNEXPECTED_AGENT_STATE"
    try:
        read_config()
    except (NotReady, OSError):
        return "CONFIGURATION_REQUIRED"
    return "ENROLLMENT_REQUIRED"



def prepare_local_enrollment():
    """Installer-owned one-time setup. The Agent consumes this package later.

    Reuses the existing Control Plane enrollment command and does not start
    services, create credentials itself, or replace prior identities.
    """
    if os.geteuid() != 0:
        raise NotReady("root required for initial Agent enrollment")
    if first_run_agent_state() != "ENROLLMENT_REQUIRED":
        raise NotReady("local Agent is not awaiting a first enrollment")
    backend = read_config(CONFIG)
    if backend != "default":
        raise NotReady("automatic local setup requires the default Backend")
    unit = properties()
    if (unit["LoadState"] != "loaded" or
            unit["ActiveState"] != "inactive" or
            unit["UnitFileState"] not in ("disabled", "enabled") or
            unit["User"] != "vdr" or unit["Group"] != "vdr" or
            unit["DropInPaths"] or
            AGENT_BIN not in unit["ExecStart"] or
            "--config " + str(CONFIG) not in unit["ExecStart"]):
        raise NotReady("installed Agent service is not ready for initial setup")

    state_dir = Path(ENROLLMENT).parent
    try:
        directory = state_dir.lstat()
        vdr = pwd.getpwnam("vdr")
    except (OSError, KeyError) as exc:
        raise NotReady("Agent state directory or service account missing") from exc
    if (not stat.S_ISDIR(directory.st_mode) or
            directory.st_mode & 0o077 or
            directory.st_uid not in (0, vdr.pw_uid) or
            Path(IDENTITY).exists() or Path(IDENTITY).is_symlink() or
            Path(ENROLLMENT).exists() or Path(ENROLLMENT).is_symlink() or
            Path(ENROLLMENT + ".pending").exists()):
        raise NotReady("Agent state is not a clean first-install state")
    database = Path(DATABASE)
    if database.is_symlink() or not database.is_file():
        raise NotReady("configured local Control Plane database unavailable")
    if directory.st_uid == 0 and vdr.pw_uid != 0:
        os.chown(state_dir, vdr.pw_uid, vdr.pw_gid)

    # Existing tool registers the one-time verifier with the Control Plane;
    # the raw token is contained only in the protected enrollment file.
    command([ENROLL_BIN, "--database", str(database), "--backend", backend,
             "--output", ENROLLMENT])

    # The enrollment binary runs as root. Hand over only the created file
    # to the packaged vdr:vdr Agent, without reading or printing its token.
    try:
        fd = os.open(ENROLLMENT, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
        try:
            entry = os.fstat(fd)
            if (not stat.S_ISREG(entry.st_mode) or
                    stat.S_IMODE(entry.st_mode) != 0o600 or
                    entry.st_nlink != 1 or entry.st_uid != 0):
                raise NotReady("created enrollment package is not protected")
            os.fchown(fd, vdr.pw_uid, vdr.pw_gid)
        finally:
            os.close(fd)
    except OSError as exc:
        raise NotReady("enrollment created but Agent ownership setup failed") from exc
    return "ENROLLMENT_PREPARED"


def channel_observation_source(path=CONFIG):
    """Require installer-selected channel facts before starting an Agent.

    No guessing the MLD VDR configuration directory or changing a pre-existing
    Agent configuration. The distribution setup owns this configuration.
    """
    fields = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line and not line.startswith("#") and "=" in line:
            key, value = line.split("=", 1)
            fields[key] = value.strip()
    adapters = [x.strip() for x in fields.get("ADAPTERS", "").split(",") if x.strip()]
    domains = [x.strip() for x in fields.get("OBSERVATION_DOMAINS", "").split(",") if x.strip()]
    if "channels-conf" not in adapters or "channels" not in domains:
        raise NotReady("Channel observation is not configured")
    source = Path(fields.get("CHANNELS_CONF_PATH", ""))
    # MLD channels.conf may legitimately link to the selected channel list.
    if not source.is_absolute() or not source.is_file():
        raise NotReady("configured VDR channel source is unavailable")
    return source


def check_channel_observation(backend, agent_id):
    """Read the existing Agent administration cursor; never infer freshness."""
    try:
        status = json.loads(command([ADMIN_BIN, "--backend", backend, "--status"]))
    except (TypeError, ValueError) as exc:
        raise NotReady("invalid Channel observation response") from exc
    observation = status.get("channelObservation") or {}
    now = int(time.time())
    if (status.get("present") is not True or
            status.get("backendId") != backend or
            status.get("agentId") != agent_id or
            type(status.get("backendGeneration")) is not int or
            not observation.get("present") or
            observation.get("backendGeneration") != status["backendGeneration"] or
            type(observation.get("snapshotGeneration")) is not int or
            observation["snapshotGeneration"] < 1 or
            type(observation.get("producerSequence")) is not int or
            observation["producerSequence"] < 1 or
            not isinstance(observation.get("resourceRevision"), str) or
            not observation["resourceRevision"] or
            type(observation.get("factCount")) is not int or
            observation["factCount"] < 1 or
            type(observation.get("acceptedAt")) is not int or
            not now - 150 <= observation["acceptedAt"] <= now + 5):
        raise NotReady("matching current VDR Channel observation unavailable")


def activate_local_agent():
    """Installer-owned activation, not a browser or ordinary user command.

    Consumes prepared enrollment through the actual packaged Agent service;
    checks its original security/health and Channel cursor before enabling
    boot startup through the existing guarded preflight.
    """
    if os.geteuid() != 0:
        raise NotReady("root required for local Agent activation")
    if first_run_agent_state() != "ENROLLMENT_PENDING":
        raise NotReady("local Agent has no pending first enrollment")
    backend = read_config()
    if backend != "default":
        raise NotReady("automatic local activation requires the default Backend")
    unit = properties()
    if (unit["LoadState"] != "loaded" or
            unit["ActiveState"] != "inactive" or
            unit["UnitFileState"] not in ("disabled", "enabled") or
            unit["User"] != "vdr" or unit["Group"] != "vdr" or
            unit["DropInPaths"] or
            unit["FragmentPath"] not in (
                "/usr/lib/systemd/system/" + UNIT,
                "/lib/systemd/system/" + UNIT) or
            AGENT_BIN not in unit["ExecStart"] or
            "--config " + str(CONFIG) not in unit["ExecStart"]):
        raise NotReady("Agent service does not match packaged local setup")
    channel_observation_source()
    enrollment = Path(ENROLLMENT)
    info = enrollment.lstat()
    vdr = pwd.getpwnam("vdr")
    if (not stat.S_ISREG(info.st_mode) or
            stat.S_IMODE(info.st_mode) != 0o600 or
            info.st_nlink != 1 or info.st_uid != vdr.pw_uid or
            Path(IDENTITY).exists() or Path(IDENTITY).is_symlink()):
        raise NotReady("pending enrollment is not owned by packaged Agent")

    command(["systemctl", "start", UNIT])

    # Enrollment, first heartbeat and a complete Channel snapshot are
    # asynchronous. Never treat a successful systemctl start as readiness.
    ready = None
    deadline = time.monotonic() + 90
    while time.monotonic() < deadline:
        try:
            state, agent_id, current_backend = preflight()
            if current_backend != backend:
                raise NotReady("Agent bound to unexpected Backend")
            check_channel_observation(backend, agent_id)
            ready = (state, agent_id, current_backend)
            break
        except NotReady:
            time.sleep(2)
    if ready is None:
        raise NotReady("Agent or Channel observation not ready after first start")

    # Reuse the same approved post-enrollment autostart path and its
    # mandatory second preflight. No restart and no second enrollment.
    state, agent_id, current_backend = preflight()
    if ready[1:] != (agent_id, current_backend):
        raise NotReady("Agent identity changed before enabling boot startup")
    check_channel_observation(current_backend, agent_id)
    if state == "disabled":
        command(["systemctl", "enable", UNIT])
    if (command(["systemctl", "is-enabled", UNIT]) != "enabled" or
            command(["systemctl", "is-active", UNIT]) != "active"):
        raise NotReady("could not verify Agent boot startup and running state")
    return "READY"




def installer_onboard_local():
    """One installer-owned, resumable step; no browser/daemon privileges.

    MLD calls this only after the package has installed the reviewed Agent
    unit, real HTTPS Control Plane and selected channels.conf. It never
    creates a second enrollment or changes an existing Agent identity.
    """
    if os.geteuid() != 0:
        raise NotReady("root required for package-owned local onboarding")
    state = first_run_agent_state()
    if state not in ("ENROLLMENT_REQUIRED", "ENROLLMENT_PENDING",
                     "IDENTITY_PRESENT"):
        raise NotReady("local VDR/Agent installation is not ready: " + state)
    if read_config() != "default":
        raise NotReady("automatic local setup requires the default Backend")
    channel_observation_source()

    if state == "IDENTITY_PRESENT":
        # Repeated package configuration is an authenticated readback, not a
        # request to enroll again, start/restart services, or replace state.
        unit_state, agent_id, backend = preflight()
        check_channel_observation(backend, agent_id)
        if unit_state == "disabled":
            # Re-check the same identity and observations immediately before
            # touching the boot policy.
            next_state, next_agent, next_backend = preflight()
            if (next_state != "disabled" or next_agent != agent_id or
                    next_backend != backend):
                raise NotReady("Agent changed before boot enable")
            check_channel_observation(backend, agent_id)
            command(["systemctl", "enable", UNIT])
        if (command(["systemctl", "is-enabled", UNIT]) != "enabled" or
                command(["systemctl", "is-active", UNIT]) != "active"):
            raise NotReady("Agent boot/runtime state not confirmed")
        return "READY"

    if state == "ENROLLMENT_REQUIRED":
        # A successful first call leaves an enrollment file, so retrying
        # cannot issue a second token or re-register the same Backend.
        prepare_local_enrollment()
    return activate_local_agent()



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    choice = parser.add_mutually_exclusive_group(required=True)
    choice.add_argument("--check", action="store_true",
                        help="readiness check only; never change systemd")
    choice.add_argument("--enable", action="store_true",
                        help="after enrollment, enable boot startup without restart")
    choice.add_argument("--setup-status", action="store_true",
                        help="inspect local first-run state without enrollment or changes")
    choice.add_argument("--prepare-enrollment", action="store_true",
                        help="installer-only: prepare one-time local enrollment")
    choice.add_argument("--activate-local", action="store_true",
                        help="installer-only: start enrolled local Agent, confirm Channels, enable boot")
    choice.add_argument("--installer-onboard", action="store_true",
                        help="installer-only: resume local Agent enrollment and activation once")
    args = parser.parse_args()
    try:
        if args.installer_onboard:
            print("LOCAL_BACKEND_SETUP=" + installer_onboard_local())
            return 0
        if args.activate_local:
            print("LOCAL_BACKEND_SETUP=" + activate_local_agent())
            return 0
        if args.prepare_enrollment:
            print("LOCAL_BACKEND_SETUP=" + prepare_local_enrollment())
            return 0
        if args.setup_status:
            print("LOCAL_BACKEND_SETUP=" + first_run_agent_state())
            return 0
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

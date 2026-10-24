#!/usr/bin/env python3
"""MT.I2 read-only install/update preflight for the existing sealed Media Tools stage.

This is NOT an alternate installer, package trust reader, approval issuer,
service controller, or a deployment permission. All mutation stays in the
existing stage and platform-specific installation owners.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import stat
import subprocess
import sys

from stage_media_tools_runtime import (
    INVENTORY_REL, payload_roles, verify_media_tools_stage, digest,
)

APPROVAL = Path("etc/vdr-suite/addon-approvals/rectools.json")
CORE_APPROVAL_DIRECTORY = Path("etc/vdr-suite/addon-approvals")
RUN_LOCK = CORE_APPROVAL_DIRECTORY / "rectools.import-running.lock"
SOURCE_LEDGER = Path("etc/vdr-suite/addon-sources/media-import.tsv")
SERVICE_OVERRIDE = Path(
    "etc/systemd/system/vdr-suite-backend-agent.service.d/20-media-tools.conf"
)
INSTALL_PACKAGE = "vdr-suite-addon-media-tools"


class PreflightError(ValueError):
    """An unsafe input, file, package ownership or installation conflict."""


def safe_root(path: Path) -> Path:
    if not path.is_absolute() or ".." in path.parts or "." in path.parts:
        raise PreflightError("root must be an absolute canonical directory")
    if not path.is_dir() or path.is_symlink() or path.resolve() != path:
        raise PreflightError("root missing, symbolic or noncanonical")
    return path


def examined_file(root: Path, rel: str) -> Path:
    """Reject symlinks anywhere below the root; never follow unknown parents."""
    parts = Path(rel).parts
    if not parts or any(p in ("", ".", "..") for p in parts):
        raise PreflightError("invalid installation inventory path")
    current = root
    for part in parts[:-1]:
        current = current / part
        try:
            st = current.lstat()
        except FileNotFoundError:
            # Missing parents are fine for a first-install plan, but inspect
            # each later component explicitly only when it exists.
            continue
        if not stat.S_ISDIR(st.st_mode):
            raise PreflightError("unsafe installation ancestor: " + str(current))
    return root / rel


def existing_status(root: Path, rel: str, entry: dict) -> str:
    target = examined_file(root, rel)
    try:
        info = target.lstat()
    except FileNotFoundError:
        return "missing"
    if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1:
        raise PreflightError("nonregular/symlink/hardlinked installed file: " + rel)
    if info.st_mode & 0o022:
        raise PreflightError("group/world-writable installed file: " + rel)
    return ("match" if (info.st_size == entry["size"] and
            stat.S_IMODE(info.st_mode) == entry["mode"] and
            digest(target) == entry["sha256"]) else "differs")


def package_owners(paths: list[str]) -> dict[str, str]:
    """Only query the package database; never try to migrate its contents."""
    owners = {}
    for rel in paths:
        completed = subprocess.run(
            ["dpkg-query", "-S", "/" + rel], capture_output=True,
            text=True, check=False, timeout=5,
        )
        if completed.returncode == 0:
            owners[rel] = completed.stdout.strip()[:512]
        elif completed.returncode not in (1,):
            raise PreflightError("dpkg ownership database unavailable")
    return owners


def auxiliary(root: Path) -> dict[str, str]:
    """Presence only; no secret contents, approvals or grants are echoed."""
    status = {}
    for label, rel in (("agent_approval", APPROVAL),
                       ("run_lock", RUN_LOCK),
                       ("source_ledger", SOURCE_LEDGER),
                       ("systemd_override", SERVICE_OVERRIDE)):
        target = examined_file(root, rel.as_posix())
        try:
            info = target.lstat()
        except FileNotFoundError:
            status[label] = "absent"
            continue
        if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1:
            raise PreflightError("unsafe existing " + label)
        if info.st_mode & 0o022:
            raise PreflightError("writable existing " + label)
        status[label] = "present_unverified"
    return status


def inspect(stage: Path, root: Path, approved: str,
            *, query_packages: bool = True) -> dict:
    stage = safe_root(stage)
    root = safe_root(root)
    # Verifies the *entire* common stage, not just the binary being installed.
    inventory = verify_media_tools_stage(stage, approved)
    files = dict(inventory["files"])
    # The inventory itself is also an installed file, sealed by Suite.
    sealed_entry = json.loads(
        (stage / ".vdr-suite-install-runtime-manifest.json").read_text(
            encoding="utf-8"))["files"][INVENTORY_REL.as_posix()]
    files[INVENTORY_REL.as_posix()] = dict(sealed_entry)
    if set(files) != set(payload_roles()) | {INVENTORY_REL.as_posix()}:
        raise PreflightError("unexpected add-on payload set")
    status = {rel: existing_status(root, rel, entry)
              for rel, entry in sorted(files.items())}
    summary = {kind: sum(value == kind for value in status.values())
               for kind in ("match", "missing", "differs")}
    additional = auxiliary(root)
    owners = package_owners(list(status)) if root == Path("/") and query_packages else {}
    # Existing dpkg-owned files are never permitted as a manual overwrite.
    kind = ("absent" if summary["missing"] == len(files) else
            "identical_payload" if summary["match"] == len(files) else
            "partial_or_different")
    if owners:
        kind = "package_owned"
    # Existing security metadata without a payload signals a previous or
    # partial installation. It is NOT a clean first install.
    if kind == "absent" and any(v != "absent" for v in additional.values()):
        kind = "existing_state_without_payload"
    return {
        "mode": "READ_ONLY", "install_state": kind,
        "stage_version": inventory["version"],
        "stage_source_sha": inventory["prepared_source_sha"],
        "payload_counts": summary,
        "payload": status,
        "auxiliary": additional,
        "package_owners": owners,
        "approval": "NOT_VERIFIED",
        "deployment": "NOT_AUTHORIZED",
    }


def prepared_state(work: Path, approved: str) -> dict:
    """Reuse an existing already-compiled and sealed candidate; no rebuild."""
    work = safe_root(work)
    record_path = examined_file(work, "candidate.txt")
    try:
        if not stat.S_ISREG(record_path.lstat().st_mode):
            raise PreflightError("unsafe prepared candidate file")
        candidate = record_path.read_text(encoding="ascii").splitlines()
    except (OSError, UnicodeError) as exc:
        raise PreflightError("missing or invalid prepared candidate") from exc
    if len(candidate) != 2 or len(candidate[0]) != 40 or any(
            c not in "0123456789abcdef" for c in candidate[0]):
        raise PreflightError("invalid prepared source SHA")
    if candidate[1] != approved:
        raise PreflightError("prepared candidate differs from independent review")
    stage = work / "stage-with-media-tools"
    inventory = verify_media_tools_stage(stage, approved)
    if inventory["prepared_source_sha"] != candidate[0]:
        raise PreflightError("prepared source differs from sealed inventory")
    # The already-built workers must agree with the exact packaged payload.
    # No compilation, package changes, host rights, or installation performed.
    for worker in ("vdr-suite-media-import", "vdr-suite-media-shrink",
                   "vdr-suite-media-check"):
        path = examined_file(work, "build/addons/" + worker)
        if not path.is_file() or path.is_symlink():
            raise PreflightError("missing prepared worker: " + worker)
        rel = "usr/libexec/vdr-suite/addons/rectools/" + worker
        if digest(path) != inventory["files"][rel]["sha256"]:
            raise PreflightError("prepared worker diverges: " + worker)
    return {
        "mode": "READ_ONLY", "prepared": "verified_existing_artifacts",
        "stage": str(stage), "source_sha": candidate[0],
        "version": inventory["version"], "build": "REUSED_NO_COMPILATION",
        "deployment": "NOT_AUTHORIZED",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "check", "verify"))
    parser.add_argument("--stage", type=Path)
    parser.add_argument("--live-root", type=Path)
    parser.add_argument("--prepared-workdir", type=Path)
    parser.add_argument("--approved-handler-sha256", required=True)
    args = parser.parse_args()
    try:
        if args.action == "prepare":
            if (args.prepared_workdir is None or args.stage is not None or
                    args.live_root is not None):
                raise PreflightError("prepare needs only --prepared-workdir")
            report = prepared_state(args.prepared_workdir,
                                    args.approved_handler_sha256)
            print(json.dumps(report, sort_keys=True, indent=2))
            print("MEDIA_TOOLS_PREPARED_REUSE=PASS")
            return 0
        if args.stage is None or args.live_root is None or (
                args.prepared_workdir is not None):
            raise PreflightError("check/verify need --stage and --live-root")
        report = inspect(args.stage, args.live_root,
                         args.approved_handler_sha256)
        print(json.dumps(report, sort_keys=True, indent=2))
        if args.action == "verify":
            if report["install_state"] != "identical_payload":
                raise PreflightError("installed Media Tools payload differs")
            print("MEDIA_TOOLS_PAYLOAD_VERIFY=PASS")
        else:
            print("MEDIA_TOOLS_INSTALL_CHECK=PASS (read-only, no deployment approval)")
        return 0
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired,
            KeyError, TypeError) as exc:
        print("MEDIA_TOOLS_INSTALL_PREFLIGHT=STOP " + str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

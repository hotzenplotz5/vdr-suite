#!/usr/bin/env python3
"""Explicit root-admin approval issuer for a fixed reviewed add-on handler.

No implicit trust from addon.json or dpkg. This tool NEVER executes the
handler, grants capabilities, installs packages or changes Suite runtime.
Only an independent administrator-approved SHA-256 may be supplied.
"""
from __future__ import annotations

import argparse
import fcntl
import json
import os
import re
import stat
import sys
import uuid
from pathlib import Path

from addon_contract import ContractError, validate_directory
from admin_approval_evidence import APPROVAL_PATH
from manual_handler_evidence import examine as examine_binary

_SHA = re.compile(r"[0-9a-f]{64}\Z")
_VERSION = re.compile(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\Z")


class IssueError(ValueError):
    pass


def issue(root: Path, module: str, version: str, approved_sha256: str,
          origin: str, allow_live_root: bool = False) -> Path:
    if os.geteuid() != 0:
        raise IssueError("root-administrator-required")
    if not root.is_absolute() or "." in root.parts or ".." in root.parts:
        raise IssueError("unsafe-root")
    if root == Path("/") and not allow_live_root:
        raise IssueError("live-root-requires-explicit-confirmation")
    if module != "rectools":
        raise IssueError("no-reviewed-handler")
    if not isinstance(version, str) or not _VERSION.fullmatch(version):
        raise IssueError("invalid-version")
    if not isinstance(approved_sha256, str) or not _SHA.fullmatch(approved_sha256):
        raise IssueError("invalid-approved-digest")
    if origin not in ("manual", "debian"):
        raise IssueError("invalid-origin")

    # Source metadata is descriptive; it must agree with the independent
    # approved identity but cannot generate or approve the expected hash.
    manifest_path = root / "usr/share/vdr-suite/addons/rectools"
    try:
        manifest = validate_directory(manifest_path, "rectools")
    except (ContractError, OSError, UnicodeError, ValueError) as exc:
        raise IssueError("invalid-installed-metadata") from exc
    if (manifest["version"] != version or
            manifest["package"] != "vdr-suite-addon-media-tools"):
        raise IssueError("approval-metadata-mismatch")

    binary = examine_binary(root, module, approved_sha256)
    if not binary["consistent"]:
        raise IssueError("binary-" + binary["reason"])

    # Traverse and create only explicitly reviewed /etc approval directories.
    # All parent directories are pinned via openat and checked as root-owned
    # and non-group/world-writable. Never accept symlinked path components.
    flags = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW
    held = os.open("/", flags)
    temporary = ""
    try:
        for part in (*root.parts[1:], *APPROVAL_PATH.split("/")[:-1]):
            info = os.fstat(held)
            if (not stat.S_ISDIR(info.st_mode) or info.st_uid != 0 or
                    info.st_mode & 0o022):
                raise IssueError("unsafe-approval-parent")
            try:
                child = os.open(part, flags, dir_fd=held)
            except FileNotFoundError:
                # Only the approval directory hierarchy may be created.
                if part not in ("etc", "vdr-suite", "addon-approvals"):
                    raise IssueError("missing-root-component")
                os.mkdir(part, mode=0o700, dir_fd=held)
                child = os.open(part, flags, dir_fd=held)
                # Only the exact directory just created by this issuer:
                # metadata contains no secrets, and the unprivileged Agent
                # must be able to inspect root-controlled approval evidence.
                os.fchmod(child, 0o755)
            os.close(held)
            held = child

        info = os.fstat(held)
        if (not stat.S_ISDIR(info.st_mode) or info.st_uid != 0 or
                info.st_mode & 0o022):
            raise IssueError("unsafe-approval-parent")
        fcntl.flock(held, fcntl.LOCK_EX)
        document = {
            "schemaVersion": 1,
            "id": "rectools",
            "package": "vdr-suite-addon-media-tools",
            "version": version,
            "sha256": approved_sha256,
            "origin": origin,
            "state": "approved",
        }
        payload = (json.dumps(document, sort_keys=True, separators=(",", ":")) + "\n").encode()
        if len(payload) > 4096:
            raise IssueError("approval-record-oversized")

        temporary = ".rectools-" + uuid.uuid4().hex + ".tmp"
        fd = os.open(temporary, os.O_CREAT | os.O_EXCL | os.O_WRONLY |
                     os.O_NOFOLLOW, 0o600, dir_fd=held)
        try:
            # Newly created record only; no chmod of existing host files.
            os.fchmod(fd, 0o644)
            with os.fdopen(fd, "wb") as file:
                file.write(payload)
                file.flush()
                os.fsync(file.fileno())
            # Hardlink publication is atomic and refuses to overwrite an
            # already published approval. No implicit update or downgrade.
            os.link(temporary, APPROVAL_PATH.rsplit("/", 1)[-1],
                    src_dir_fd=held, dst_dir_fd=held, follow_symlinks=False)
            os.fsync(held)
        finally:
            os.unlink(temporary, dir_fd=held)
            temporary = ""
        return root / APPROVAL_PATH
    except OSError as exc:
        raise IssueError("approval-install-failed") from exc
    finally:
        if temporary:
            try:
                os.unlink(temporary, dir_fd=held)
            except OSError:
                pass
        os.close(held)




def rotate(root: Path, module: str, version: str, expected_sha256: str,
           approved_sha256: str, origin: str,
           allow_live_root: bool = False) -> Path:
    """CAS-rotate an existing *active* root approval, never resurrect a revoke.

    Upgrades to a different handler interface version or install origin must
    be separately reviewed, not inferred from a new package's self-report.
    An approval does not activate Media Tools or prove Debian provenance.
    """
    if os.geteuid() != 0:
        raise IssueError("root-administrator-required")
    if not root.is_absolute() or "." in root.parts or ".." in root.parts:
        raise IssueError("unsafe-root")
    if root == Path("/") and not allow_live_root:
        raise IssueError("live-root-requires-explicit-confirmation")
    if module != "rectools":
        raise IssueError("no-reviewed-handler")
    if version != "0.1.0" or not _VERSION.fullmatch(version):
        raise IssueError("unreviewed-handler-version")
    if (not isinstance(expected_sha256, str) or
            not _SHA.fullmatch(expected_sha256) or
            not isinstance(approved_sha256, str) or
            not _SHA.fullmatch(approved_sha256) or
            expected_sha256 == approved_sha256):
        raise IssueError("invalid-approval-rotation-digests")
    if origin not in ("manual", "debian"):
        raise IssueError("invalid-origin")
    try:
        manifest = validate_directory(
            root / "usr/share/vdr-suite/addons/rectools", "rectools")
    except (ContractError, OSError, UnicodeError, ValueError) as exc:
        raise IssueError("invalid-installed-metadata") from exc
    if (manifest["version"] != version or
            manifest["package"] != "vdr-suite-addon-media-tools"):
        raise IssueError("approval-metadata-mismatch")
    candidate = examine_binary(root, module, approved_sha256)
    if not candidate["consistent"]:
        raise IssueError("binary-" + candidate["reason"])

    flags = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW
    held = os.open("/", flags)
    temporary = ""
    try:
        for part in (*root.parts[1:], *APPROVAL_PATH.split("/")[:-1]):
            parent = os.fstat(held)
            if (not stat.S_ISDIR(parent.st_mode) or parent.st_uid != 0 or
                    parent.st_mode & 0o022):
                raise IssueError("unsafe-approval-parent")
            child = os.open(part, flags, dir_fd=held)
            os.close(held)
            held = child
        parent = os.fstat(held)
        if (not stat.S_ISDIR(parent.st_mode) or parent.st_uid != 0 or
                parent.st_mode & 0o022):
            raise IssueError("unsafe-approval-parent")
        # This also serializes rotations with issue/revoke by this issuer.
        fcntl.flock(held, fcntl.LOCK_EX)
        name = APPROVAL_PATH.rsplit("/", 1)[-1]
        fd = os.open(name, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK,
                     dir_fd=held)
        with os.fdopen(fd, "rb") as source:
            before = os.fstat(source.fileno())
            if (not stat.S_ISREG(before.st_mode) or before.st_uid != 0 or
                    before.st_mode & 0o022 or before.st_nlink != 1 or
                    not 0 < before.st_size <= 4096):
                raise IssueError("unsafe-approval-record")
            raw = source.read(4097)
            after = os.fstat(source.fileno())
            if (len(raw) != before.st_size or
                    (before.st_dev, before.st_ino, before.st_uid,
                     before.st_mode, before.st_nlink, before.st_size,
                     before.st_mtime_ns, before.st_ctime_ns) !=
                    (after.st_dev, after.st_ino, after.st_uid,
                     after.st_mode, after.st_nlink, after.st_size,
                     after.st_mtime_ns, after.st_ctime_ns)):
                raise IssueError("approval-record-changed")

        def unique(pairs):
            values = {}
            for key, value in pairs:
                if key in values:
                    raise IssueError("duplicate-approval-key")
                values[key] = value
            return values

        record = json.loads(raw.decode("utf-8"), object_pairs_hook=unique)
        if (not isinstance(record, dict) or
                set(record) != {
                    "schemaVersion", "id", "package", "version",
                    "sha256", "origin", "state",
                } or type(record["schemaVersion"]) is not int or
                record["schemaVersion"] != 1 or
                record["id"] != "rectools" or
                record["package"] != "vdr-suite-addon-media-tools" or
                record["version"] != version or
                record["origin"] != origin or
                not isinstance(record["sha256"], str) or
                not _SHA.fullmatch(record["sha256"]) or
                record["state"] not in ("approved", "revoked")):
            raise IssueError("invalid-approval-record")
        if record["state"] != "approved":
            raise IssueError("approval-revoked")
        if record["sha256"] != expected_sha256:
            raise IssueError("stale-approval-rotation")

        record["sha256"] = approved_sha256
        payload = (json.dumps(record, sort_keys=True,
                              separators=(",", ":")) + "\n").encode()
        temporary = ".rectools-rotate-" + uuid.uuid4().hex + ".tmp"
        temp_fd = os.open(temporary, os.O_CREAT | os.O_EXCL | os.O_WRONLY |
                          os.O_NOFOLLOW, 0o600, dir_fd=held)
        try:
            os.fchmod(temp_fd, 0o644)
            with os.fdopen(temp_fd, "wb") as target:
                target.write(payload)
                target.flush()
                os.fsync(target.fileno())
            current = os.stat(name, dir_fd=held, follow_symlinks=False)
            if (current.st_dev, current.st_ino, current.st_uid,
                    current.st_mode, current.st_nlink, current.st_size,
                    current.st_mtime_ns, current.st_ctime_ns) != (
                    before.st_dev, before.st_ino, before.st_uid,
                    before.st_mode, before.st_nlink, before.st_size,
                    before.st_mtime_ns, before.st_ctime_ns):
                raise IssueError("approval-record-changed")
            # No implicit activation; a swapped package/binary can only
            # proceed with its own independently approved new SHA.
            os.replace(temporary, name, src_dir_fd=held, dst_dir_fd=held)
            temporary = ""
            os.fsync(held)
        finally:
            if temporary:
                os.unlink(temporary, dir_fd=held)
                temporary = ""
        return root / APPROVAL_PATH
    except (OSError, ValueError, UnicodeError, TypeError) as exc:
        if isinstance(exc, IssueError):
            raise
        raise IssueError("approval-rotation-failed") from exc
    finally:
        if temporary:
            try:
                os.unlink(temporary, dir_fd=held)
            except OSError:
                pass
        os.close(held)


def revoke(root: Path, module: str, allow_live_root: bool = False) -> Path:
    """Atomically mark an existing admin approval revoked, even if binary is gone."""
    if os.geteuid() != 0:
        raise IssueError("root-administrator-required")
    if not root.is_absolute() or "." in root.parts or ".." in root.parts:
        raise IssueError("unsafe-root")
    if root == Path("/") and not allow_live_root:
        raise IssueError("live-root-requires-explicit-confirmation")
    if module != "rectools":
        raise IssueError("no-reviewed-handler")

    flags = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW
    held = os.open("/", flags)
    temporary = ""
    try:
        for part in (*root.parts[1:], *APPROVAL_PATH.split("/")[:-1]):
            parent = os.fstat(held)
            if (not stat.S_ISDIR(parent.st_mode) or parent.st_uid != 0 or
                    parent.st_mode & 0o022):
                raise IssueError("unsafe-approval-parent")
            child = os.open(part, flags, dir_fd=held)
            os.close(held)
            held = child
        parent = os.fstat(held)
        if (not stat.S_ISDIR(parent.st_mode) or parent.st_uid != 0 or
                parent.st_mode & 0o022):
            raise IssueError("unsafe-approval-parent")
        fcntl.flock(held, fcntl.LOCK_EX)
        name = APPROVAL_PATH.rsplit("/", 1)[-1]
        fd = os.open(name, os.O_RDONLY | os.O_NOFOLLOW, dir_fd=held)
        with os.fdopen(fd, "rb") as source:
            original = os.fstat(source.fileno())
            if (not stat.S_ISREG(original.st_mode) or original.st_uid != 0 or
                    original.st_mode & 0o022 or not 0 < original.st_size <= 4096):
                raise IssueError("unsafe-approval-record")
            raw = source.read(4097)
            last = os.fstat(source.fileno())
            if len(raw) != original.st_size or (
                original.st_dev, original.st_ino, original.st_size,
                original.st_mtime_ns, original.st_ctime_ns) != (
                last.st_dev, last.st_ino, last.st_size,
                last.st_mtime_ns, last.st_ctime_ns):
                raise IssueError("approval-record-changed")

        def unique(pairs):
            record = {}
            for key, value in pairs:
                if key in record:
                    raise IssueError("duplicate-approval-key")
                record[key] = value
            return record

        document = json.loads(raw.decode("utf-8"), object_pairs_hook=unique)
        if (not isinstance(document, dict) or
                set(document) != {
                    "schemaVersion", "id", "package", "version",
                    "sha256", "origin", "state",
                } or type(document["schemaVersion"]) is not int or
                document["schemaVersion"] != 1 or
                document["id"] != "rectools" or
                document["package"] != "vdr-suite-addon-media-tools" or
                not isinstance(document["sha256"], str) or
                not _SHA.fullmatch(document["sha256"]) or
                not isinstance(document["version"], str) or
                not _VERSION.fullmatch(document["version"]) or
                document["origin"] not in ("manual", "debian") or
                document["state"] not in ("approved", "revoked")):
            raise IssueError("invalid-approval-record")

        if document["state"] == "revoked":
            return root / APPROVAL_PATH

        document["state"] = "revoked"
        payload = (json.dumps(document, sort_keys=True, separators=(",", ":")) + "\n").encode()
        temporary = ".rectools-revoke-" + uuid.uuid4().hex + ".tmp"
        temp_fd = os.open(temporary, os.O_CREAT | os.O_EXCL | os.O_WRONLY |
                          os.O_NOFOLLOW, 0o600, dir_fd=held)
        try:
            os.fchmod(temp_fd, 0o644)
            with os.fdopen(temp_fd, "wb") as target:
                target.write(payload)
                target.flush()
                os.fsync(target.fileno())
            present = os.stat(name, dir_fd=held, follow_symlinks=False)
            if (present.st_dev, present.st_ino, present.st_size,
                    present.st_mtime_ns, present.st_ctime_ns) != (
                    original.st_dev, original.st_ino, original.st_size,
                    original.st_mtime_ns, original.st_ctime_ns):
                raise IssueError("approval-record-changed")
            os.replace(temporary, name, src_dir_fd=held, dst_dir_fd=held)
            temporary = ""
            os.fsync(held)
        finally:
            if temporary:
                os.unlink(temporary, dir_fd=held)
                temporary = ""
        return root / APPROVAL_PATH
    except (OSError, ValueError, UnicodeError, TypeError) as exc:
        if isinstance(exc, IssueError):
            raise
        raise IssueError("approval-revocation-failed") from exc
    finally:
        if temporary:
            try:
                os.unlink(temporary, dir_fd=held)
            except OSError:
                pass
        os.close(held)

def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("issue", "rotate", "revoke"))
    parser.add_argument("--root", required=True)
    parser.add_argument("--module", required=True)
    parser.add_argument("--version", default="")
    parser.add_argument("--expected-sha256", default="",
                        help="Current approved SHA-256 required for CAS rotation")
    parser.add_argument("--approved-sha256", default="",
                        help="Independently reviewed expected digest, NOT generated from installed file")
    parser.add_argument("--origin", choices=("manual", "debian"), default="")
    parser.add_argument("--allow-live-root", action="store_true",
                        help="Explicit operator approval to write on / (never used in tests)")
    args = parser.parse_args(argv)
    try:
        if args.command == "issue":
            location = issue(Path(args.root), args.module, args.version,
                             args.approved_sha256, args.origin, args.allow_live_root)
            print("ADDON_ADMIN_APPROVAL_ISSUED=" + str(location))
        elif args.command == "rotate":
            location = rotate(Path(args.root), args.module, args.version,
                              args.expected_sha256, args.approved_sha256,
                              args.origin, args.allow_live_root)
            print("ADDON_ADMIN_APPROVAL_ROTATED=" + str(location))
        else:
            location = revoke(Path(args.root), args.module, args.allow_live_root)
            print("ADDON_ADMIN_APPROVAL_REVOKED=" + str(location))
        return 0
    except (IssueError, OSError) as exc:
        print("ADDON_ADMIN_APPROVAL=FAIL " + str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

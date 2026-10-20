#!/usr/bin/env python3
"""Explicit root-admin approval issuer for a fixed reviewed add-on handler.

No implicit trust from addon.json or dpkg. This tool NEVER executes the
handler, grants capabilities, installs packages or changes Suite runtime.
Only an independent administrator-approved SHA-256 may be supplied.
"""
from __future__ import annotations

import argparse
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
            os.close(held)
            held = child

        info = os.fstat(held)
        if (not stat.S_ISDIR(info.st_mode) or info.st_uid != 0 or
                info.st_mode & 0o022):
            raise IssueError("unsafe-approval-parent")
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


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("issue",))
    parser.add_argument("--root", required=True)
    parser.add_argument("--module", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--approved-sha256", required=True,
                        help="Independently reviewed expected digest, NOT generated from installed file")
    parser.add_argument("--origin", choices=("manual", "debian"), required=True)
    parser.add_argument("--allow-live-root", action="store_true",
                        help="Explicit operator approval to write on / (never used in tests)")
    args = parser.parse_args(argv)
    try:
        location = issue(Path(args.root), args.module, args.version,
                         args.approved_sha256, args.origin, args.allow_live_root)
        print("ADDON_ADMIN_APPROVAL_ISSUED=" + str(location))
        return 0
    except (IssueError, OSError) as exc:
        print("ADDON_ADMIN_APPROVAL=FAIL " + str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

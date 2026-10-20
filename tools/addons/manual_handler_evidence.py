#!/usr/bin/env python3
"""Read-only manual Media Tools handler integrity diagnostic.

The SHA-256 expectation must come from a separately reviewed, trusted
deployment record. A caller-supplied digest is NOT package provenance or
permission to execute the binary. No subprocesses or handler execution.
"""
from __future__ import annotations

import hashlib
import re
import stat
from pathlib import Path

HANDLER = "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import"
SHA256 = re.compile(r"[0-9a-f]{64}\Z")
MAX_BYTES = 128 * 1024 * 1024


def examine(root: Path, module: str, expected_sha256: str) -> dict:
    def deny(reason: str) -> dict:
        return {"consistent": False, "reason": reason}

    if module != "rectools":
        return deny("no-reviewed-manual-handler")
    if not isinstance(expected_sha256, str) or not SHA256.fullmatch(expected_sha256):
        return deny("invalid-approved-digest")
    if not root.is_absolute() or "." in root.parts or ".." in root.parts:
        return deny("unsafe-root")
    path = root
    try:
        # No configurable executable path; disallow symlinks at every level.
        for part in HANDLER.split("/"):
            info_parent = path.lstat()
            mode = info_parent.st_mode
            if not stat.S_ISDIR(mode) or stat.S_ISLNK(mode):
                return deny("unsafe-handler-path")
            if info_parent.st_uid != 0 or mode & 0o022:
                return deny("unsafe-handler-parent")
            path = path / part
        info = path.lstat()
        if not stat.S_ISREG(info.st_mode) or stat.S_ISLNK(info.st_mode):
            return deny("unsafe-handler-file")
        if not info.st_mode & 0o100:
            return deny("handler-not-executable")
        if info.st_uid != 0 or info.st_mode & 0o022:
            return deny("unsafe-handler-ownership")
        if not 0 < info.st_size <= MAX_BYTES:
            return deny("invalid-handler-size")
        digest = hashlib.sha256()
        # O_NOFOLLOW and descriptor identity prevent final-component symlink
        # substitution; the path can still change after the check. Runtime
        # must revalidate and pin the opened descriptor before dispatch.
        import os
        fd = os.open(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0))
        with os.fdopen(fd, "rb") as source:
            current = os.fstat(source.fileno())
            if (current.st_dev, current.st_ino, current.st_size,
                    current.st_mtime_ns) != (info.st_dev, info.st_ino,
                                             info.st_size, info.st_mtime_ns):
                return deny("handler-changed-during-check")
            if not stat.S_ISREG(current.st_mode) or current.st_uid != 0 or current.st_mode & 0o022:
                return deny("unsafe-handler-ownership")
            total = 0
            while True:
                chunk = source.read(min(1024 * 1024, MAX_BYTES + 1 - total))
                if not chunk:
                    break
                total += len(chunk)
                if total > MAX_BYTES:
                    return deny("invalid-handler-size")
                digest.update(chunk)
            final = os.fstat(source.fileno())
            if (final.st_size, final.st_mtime_ns, final.st_ctime_ns) != (
                    current.st_size, current.st_mtime_ns, current.st_ctime_ns):
                return deny("handler-changed-during-check")
        if digest.hexdigest() != expected_sha256:
            return deny("handler-digest-mismatch")
        return {"consistent": True, "reason": "manual-handler-bytes-match-approved-digest"}
    except OSError:
        return deny("missing-or-unreadable-handler")

#!/usr/bin/env python3
"""Read-only integrity check of the fixed manual Media Tools handler.

No package trust or executable grant is created by this diagnostic.
Every path component is opened relative to a held directory descriptor;
symlinks and writable/non-root parent directories are denied.
"""
from __future__ import annotations

import hashlib
import os
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

    directory_flags = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW
    held = None
    try:
        # Start from a stable root descriptor rather than checking and then
        # reopening a path by name. Ancestors of an isolated DESTDIR are also
        # checked, including a potentially writable /tmp ancestor.
        held = os.open("/", directory_flags)
        for component in (*root.parts[1:], *HANDLER.split("/")[:-1]):
            parent = os.fstat(held)
            if (not stat.S_ISDIR(parent.st_mode) or parent.st_uid != 0 or
                    parent.st_mode & 0o022):
                return deny("unsafe-handler-parent")
            next_fd = os.open(component, directory_flags, dir_fd=held)
            os.close(held)
            held = next_fd
        parent = os.fstat(held)
        if (not stat.S_ISDIR(parent.st_mode) or parent.st_uid != 0 or
                parent.st_mode & 0o022):
            return deny("unsafe-handler-parent")

        fd = os.open(HANDLER.rsplit("/", 1)[-1],
                     os.O_RDONLY | os.O_NOFOLLOW, dir_fd=held)
        with os.fdopen(fd, "rb") as source:
            first = os.fstat(source.fileno())
            if not stat.S_ISREG(first.st_mode):
                return deny("unsafe-handler-file")
            if not first.st_mode & 0o100:
                return deny("handler-not-executable")
            if first.st_uid != 0 or first.st_mode & 0o022:
                return deny("unsafe-handler-ownership")
            # Match the native C++ descriptor policy: a hardlinked handler
            # cannot become an independently trusted immutable executable.
            if first.st_nlink != 1:
                return deny("unsafe-handler-link-count")
            if not 0 < first.st_size <= MAX_BYTES:
                return deny("invalid-handler-size")
            digest = hashlib.sha256()
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
            if (first.st_dev, first.st_ino, first.st_size, first.st_mtime_ns,
                    first.st_ctime_ns) != (
                    final.st_dev, final.st_ino, final.st_size, final.st_mtime_ns,
                    final.st_ctime_ns):
                return deny("handler-changed-during-check")
        if digest.hexdigest() != expected_sha256:
            return deny("handler-digest-mismatch")
        # The descriptor closes here. A future Agent must independently pin
        # and revalidate its own executable descriptor at dispatch.
        return {"consistent": True, "reason": "manual-handler-bytes-match-approved-digest"}
    except OSError:
        return deny("missing-or-unreadable-handler")
    finally:
        if held is not None:
            os.close(held)

#!/usr/bin/env python3
"""P7: create and verify an SQLite recovery rehearsal in a PRIVATE scratch folder.

This tool deliberately cannot write under /var/lib/vdr-suite or modify an
installed service. Supply an already offline database COPY placed into a
0700, caller-owned scratch workspace. It is not an automatic migration tool.
"""
import argparse
from contextlib import closing
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sqlite3
import stat
import sys
import time

GiB = 1024 ** 3
MiB = 1024 ** 2
MAX_BACKUP_SECONDS = 60


def require_workspace(folder):
    if os.geteuid() == 0:
        raise ValueError("recovery rehearsals must not run as root")
    if not folder.is_absolute() or folder.is_symlink() or folder.resolve() != folder:
        raise ValueError("workspace must be an existing real absolute directory")
    st = folder.stat()
    if not stat.S_ISDIR(st.st_mode) or st.st_uid != os.geteuid():
        raise ValueError("workspace must be a caller-owned directory")
    if stat.S_IMODE(st.st_mode) & 0o077:
        raise ValueError("workspace must be private (mode 0700)")
    return folder


def member(folder, name):
    if not re.fullmatch(r"[A-Za-z0-9_][A-Za-z0-9_.-]{0,127}", name):
        raise ValueError("workspace filename must be a plain basename")
    return folder / name


def regular_private_source(path):
    if path.is_symlink() or not path.is_file() or path.stat().st_uid != os.geteuid():
        raise ValueError("source must be a regular caller-owned non-symlink file")
    for suffix in ("-wal", "-shm"):
        sidecar = Path(str(path) + suffix)
        if sidecar.is_symlink():
            raise ValueError("database sidecar cannot be a symlink")


def ensure_new_file(path):
    if os.path.lexists(path):
        raise ValueError("refusing to overwrite an existing recovery file: " + path.name)


def space_preflight(folder, source, capacity=shutil.disk_usage):
    wal = Path(str(source) + "-wal")
    source_bytes = source.stat().st_size + (wal.stat().st_size if wal.is_file() else 0)
    if source_bytes <= 0:
        raise ValueError("empty SQLite source")
    disk = capacity(folder)
    reserve = max(4 * GiB, disk.total * 15 // 100)
    # Two concurrent new database copies, journals, metadata and slack.
    budget = max(256 * MiB, source_bytes * 5 + 64 * MiB)
    print("RESOURCE_TOTAL={} RESOURCE_FREE={} RESERVE={} BUDGET={}".format(
        disk.total, disk.free, reserve, budget), flush=True)
    if disk.free < reserve + budget:
        raise ValueError("insufficient filesystem capacity for offline rehearsal")
    print("RESOURCE_PREFLIGHT=PASS", flush=True)
    return reserve


def sqlite_facts(con):
    integrity = con.execute("PRAGMA integrity_check").fetchone()[0]
    if integrity != "ok":
        raise ValueError("SQLite integrity check failed")
    version = con.execute("PRAGMA user_version").fetchone()[0]
    schema = con.execute(
        "SELECT type, name, COALESCE(sql, '') FROM sqlite_master "
        "WHERE name NOT LIKE 'sqlite_%' ORDER BY type, name").fetchall()
    digest = hashlib.sha256(repr(schema).encode("utf-8")).hexdigest()
    return {"integrity": integrity, "user_version": version,
            "schema_sha256": digest, "object_count": len(schema)}


def file_hash(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def create_exclusive(path):
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
    if hasattr(os, "O_NOFOLLOW"):
        flags |= os.O_NOFOLLOW
    fd = os.open(path, flags, 0o600)
    os.close(fd)


def rehearse(workspace, source_name, backup_name, restored_name,
             capacity=shutil.disk_usage):
    folder = require_workspace(Path(workspace))
    if len({source_name, backup_name, restored_name}) != 3:
        raise ValueError("source, backup and restored filenames must differ")
    source, backup, restored = (member(folder, n) for n in
                                (source_name, backup_name, restored_name))
    regular_private_source(source)
    for target in (backup, restored):
        ensure_new_file(target)
    reserve = space_preflight(folder, source, capacity=capacity)
    created = []
    try:
        # mode=ro includes committed WAL frames. The caller must already have
        # made the input an offline, quiesced copy: no active service state here.
        with closing(sqlite3.connect(source.as_uri() + "?mode=ro", uri=True, timeout=2)) as original:
            expected = sqlite_facts(original)
            create_exclusive(backup)
            created.append(backup)
            start = time.monotonic()

            def progress(_status, _remaining, _total):
                if time.monotonic() - start > MAX_BACKUP_SECONDS:
                    raise TimeoutError("SQLite backup exceeded time limit")

            with closing(sqlite3.connect(backup)) as snapshot:
                original.backup(snapshot, pages=128, progress=progress, sleep=0.025)
                if sqlite_facts(snapshot) != expected:
                    raise ValueError("backup changed the SQLite schema or integrity")
        digest = file_hash(backup)
        created.append(restored)
        flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
        if hasattr(os, "O_NOFOLLOW"):
            flags |= os.O_NOFOLLOW
        fd = os.open(restored, flags, 0o600)
        with backup.open("rb") as incoming, os.fdopen(fd, "wb") as outgoing:
            shutil.copyfileobj(incoming, outgoing, length=1024 * 1024)
        if file_hash(restored) != digest:
            raise ValueError("restored bytes differ from backup")
        with closing(sqlite3.connect(restored.as_uri() + "?mode=ro", uri=True)) as checked:
            if sqlite_facts(checked) != expected:
                raise ValueError("restored database failed SQLite verification")
        free = capacity(folder).free
        if free < reserve:
            raise ValueError("post-rehearsal filesystem reserve violated")
        print("RESOURCE_POSTCHECK=PASS", flush=True)
        return {"result": "P7_OFFLINE_RECOVERY_REHEARSAL_PASS",
                "source": source.name, "backup": backup.name,
                "restored": restored.name, "backup_sha256": digest,
                "backup_bytes": backup.stat().st_size, **expected,
                "scope": "private offline scratch only; no live restore or migration"}
    except Exception:
        # Only files created by this operation, within verified private workspace.
        for target in reversed(created):
            if target.is_file() and not target.is_symlink():
                target.unlink()
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workspace", required=True, type=Path)
    parser.add_argument("--source", required=True, help="offline copied database filename")
    parser.add_argument("--backup", default="vdr-suite-backup.db")
    parser.add_argument("--restored", default="vdr-suite-restored.db")
    args = parser.parse_args()
    try:
        result = rehearse(args.workspace, args.source, args.backup, args.restored)
    except (OSError, ValueError, TimeoutError, sqlite3.Error) as error:
        print("P7_OFFLINE_RECOVERY_REHEARSAL=STOP: " + str(error), file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())

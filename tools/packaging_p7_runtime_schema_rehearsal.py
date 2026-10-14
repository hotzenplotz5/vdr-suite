#!/usr/bin/env python3
"""P7 historical schema -> CURRENT C++ repository initializers, offline only.

Only creates databases in a fresh, auto-deleted private scratch directory.
No live service, apt, package installation, or production database access.
"""
import argparse
from contextlib import closing
import os
from pathlib import Path
import shutil
import sqlite3
import subprocess
import tempfile

from test_packaging_p7_historical_schema import (
    LEGACY, LEGACY_GIT_BLOB_SHA1, TABLES, create_legacy_database,
    require_consistent, source_git_blob_hash,
)

GiB = 1024 ** 3
MiB = 1024 ** 2


def original_rows(connection, columns):
    """Compare only columns that existed in the historical Git schema."""
    snapshot = {}
    for table in TABLES:
        names = columns[table]
        quoted = ",".join('"' + name.replace('"', '""') + '"' for name in names)
        snapshot[table] = connection.execute(
            "SELECT " + quoted + " FROM " + table + " ORDER BY rowid"
        ).fetchall()
    return snapshot


def run(binary):
    if os.geteuid() == 0:
        raise ValueError("P7 runtime fixture must not run as root")
    if source_git_blob_hash(LEGACY.read_bytes()) != LEGACY_GIT_BLOB_SHA1:
        raise ValueError("historical Git SQL fixture provenance mismatch")
    executable = Path(binary).resolve(strict=True)
    if not executable.is_file() or not os.access(executable, os.X_OK):
        raise ValueError("P7 runtime fixture executable missing")

    # The runner constructs every database filename inside this unique folder.
    with tempfile.TemporaryDirectory(prefix="p7-cpp-repository-") as scratch:
        folder = Path(scratch).resolve()
        mode = folder.stat().st_mode & 0o777
        if folder.stat().st_uid != os.geteuid() or mode & 0o077:
            raise ValueError("P7 fixture workspace must be caller-owned/private")
        disk = shutil.disk_usage(folder)
        reserve = max(4 * GiB, disk.total * 15 // 100)
        budget = 128 * MiB
        print("RESOURCE_TOTAL={} RESOURCE_FREE={} RESERVE={} BUDGET={}".format(
            disk.total, disk.free, reserve, budget), flush=True)
        if disk.free < reserve + budget:
            raise ValueError("P7 historical runtime fixture: insufficient space")
        print("RESOURCE_PREFLIGHT=PASS", flush=True)

        old, backup, restored = (
            folder / n for n in ("old.db", "before.db", "restored.db"))
        create_legacy_database(old)
        with closing(sqlite3.connect(old)) as source, closing(
                sqlite3.connect(backup)) as target:
            require_consistent(source)
            columns = {
                table: [r[1] for r in source.execute(
                    "PRAGMA table_info('" + table + "')")] for table in TABLES
            }
            before = original_rows(source, columns)
            original_objects = dict(source.execute(
                "SELECT name,sql FROM sqlite_master WHERE name NOT LIKE 'sqlite_%'"
            ).fetchall())
            source.backup(target)
        assert all(len(rows) == 1 for rows in before.values()), (
            "historical records missing in test fixture")

        # Each separate process reopens the database and calls the real C++
        # ensureSchema methods twice. This exercises restart idempotence.
        for count in (1, 2):
            p = subprocess.run([str(executable), str(old)],
                               text=True, capture_output=True, timeout=45)
            if p.returncode or "P7_RUNTIME_SCHEMA_REPOSITORIES=PASS" not in p.stdout:
                raise AssertionError("P7 C++ schema pass {} failed: {} {}".format(
                    count, p.stdout[-2000:], p.stderr[-2000:]))
            with closing(sqlite3.connect(old)) as connection:
                require_consistent(connection)
                assert original_rows(connection, columns) == before, (
                    "historical records changed after runtime schema initializer")
                after_objects = dict(connection.execute(
                    "SELECT name,sql FROM sqlite_master WHERE name NOT LIKE 'sqlite_%'"
                ).fetchall())
                assert all(after_objects.get(k) == v
                           for k, v in original_objects.items()), (
                    "historical SQL objects changed unexpectedly")
                assert connection.execute(
                    "SELECT count(*) FROM suite_metadata_schema_versions"
                ).fetchone()[0] >= 1
                assert connection.execute(
                    "SELECT count(*) FROM vdr_global_search_schema_versions"
                ).fetchone()[0] == 1

        # Recovery remains completely separate from the migrated fixture.
        with closing(sqlite3.connect(backup)) as source, closing(
                sqlite3.connect(restored)) as target:
            source.backup(target)
            require_consistent(target)
            assert original_rows(target, columns) == before, (
                "offline restored rows differ from historical source")
            assert dict(target.execute(
                "SELECT name,sql FROM sqlite_master WHERE name NOT LIKE 'sqlite_%'"
            ).fetchall()) == original_objects, (
                "offline restored schema differs from historical source")

        final = shutil.disk_usage(folder)
        if final.free < reserve:
            raise ValueError("P7 runtime rehearsal broke filesystem reserve")
        print("RESOURCE_POSTCHECK=PASS", flush=True)
        print("P7_HISTORICAL_CPP_SCHEMA_REHEARSAL=PASS", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True)
    args = parser.parse_args()
    try:
        run(args.binary)
    except (OSError, ValueError, AssertionError, sqlite3.Error,
            subprocess.TimeoutExpired) as error:
        parser.exit(1, "P7_HISTORICAL_CPP_SCHEMA_REHEARSAL=FAIL: " +
                    str(error) + "\n")


if __name__ == "__main__":
    main()

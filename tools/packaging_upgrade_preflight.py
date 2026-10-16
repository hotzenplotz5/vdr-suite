#!/usr/bin/env python3
"""Read-only inspection of a candidate VDR-Suite SQLite database before upgrade."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sqlite3
import sys

def reject_sidecars(db: Path):
    """Immutable SQLite ignores WAL content; reject any nearby journal state."""
    for suffix in ("-wal", "-shm", "-journal"):
        sidecar = Path(str(db) + suffix)
        # lexists sees dangling symlinks too, avoiding an unsafe clean result.
        if os.path.lexists(sidecar):
            raise ValueError(
                "SQLite sidecar present (" + suffix +
                "); preflight requires a quiesced, checkpointed offline copy"
            )


# These are explicit, versioned SQLite tables found in the real repositories.
# This list is NOT a complete registry of all runtime-owned schemas.
KNOWN_VERSION_TABLES = (
    "suite_metadata_schema_versions",
    "vdr_global_search_schema_versions",
)


def inspect_known_versions(con, table_names):
    observed = {}
    for name in KNOWN_VERSION_TABLES:
        if name not in table_names:
            continue
        versions = []
        for (value,) in con.execute("SELECT version FROM " + name + " ORDER BY version"):
            if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
                raise ValueError("invalid version marker in " + name)
            versions.append(value)
        observed[name] = {"applied_versions": versions,
                          "highest_version": versions[-1] if versions else None}
    return observed


def check(db: Path):
    if not db.is_file() or db.is_symlink():
        raise ValueError("database must be an existing regular non-symlink file")
    # SQLite immutable=1 does not consult committed WAL frames. A database
    # with a WAL could otherwise pass an integrity check on stale content.
    # This is ONLY an offline-copy inspection, never a proof of live quiescence.
    original = db.resolve(strict=True)
    reject_sidecars(original)
    uri = original.as_uri() + "?mode=ro&immutable=1"
    con = sqlite3.connect(uri, uri=True)
    try:
        integrity = con.execute("PRAGMA quick_check").fetchone()[0]
        version = con.execute("PRAGMA user_version").fetchone()[0]
        tables = [row[0] for row in con.execute(
            "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name")]
        if integrity != "ok":
            raise ValueError("database quick_check failed: " + str(integrity))
        known_versions = inspect_known_versions(con, set(tables))
        # Match the repository's offline recovery rehearsal fingerprint.
        # sqlite_master definitions are metadata: no row values or credentials
        # enter this hash. It identifies DDL, not an approved migration path.
        schema_objects = con.execute(
            "SELECT type, name, COALESCE(sql, '') FROM sqlite_master "
            "WHERE name NOT LIKE 'sqlite_%' ORDER BY type, name").fetchall()
        schema_sha256 = hashlib.sha256(
            repr(schema_objects).encode("utf-8")).hexdigest()
        # quick_check does not validate foreign keys. Never accept an offline
        # snapshot as structurally consistent when referential integrity fails.
        if con.execute("PRAGMA foreign_key_check").fetchone() is not None:
            raise ValueError("database foreign_key_check failed")
        # Catch a sidecar introduced while opening/reading a candidate.
        reject_sidecars(original)
        return {"schema": 3, "database": str(db), "user_version": version,
                "table_count": len(tables), "integrity": integrity,
                "foreign_key_check": "ok",
                "schema_object_count": len(schema_objects),
                "schema_sha256": schema_sha256,
                "known_schema_versions": known_versions,
                "compatibility": "REVIEW_REQUIRED",
                "note": "Offline-only, partial schema-marker inventory and "
                        "full DDL fingerprint; not proof of daemon provenance, "
                        "WAL quiescence, migration or compatibility approval"}
    finally:
        con.close()

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--database", type=Path, required=True)
    args = p.parse_args()
    try:
        print(json.dumps(check(args.database), sort_keys=True))
    except (sqlite3.Error, OSError, ValueError) as e:
        p.exit(1, "UPGRADE_PREFLIGHT_FAILED: " + str(e) + "\n")

if __name__ == "__main__":
    main()

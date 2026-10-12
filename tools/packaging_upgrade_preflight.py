#!/usr/bin/env python3
"""Read-only inspection of a candidate VDR-Suite SQLite database before upgrade."""
import argparse
import json
from pathlib import Path
import sqlite3
import sys

def check(db: Path):
    if not db.is_file() or db.is_symlink():
        raise ValueError("database must be an existing regular non-symlink file")
    # SQLite URI mode=ro avoids accidental database creation. Immutable also
    # suppresses sidecar creation; do NOT treat this as a live DB backup.
    uri = db.resolve().as_uri() + "?mode=ro&immutable=1"
    con = sqlite3.connect(uri, uri=True)
    try:
        integrity = con.execute("PRAGMA quick_check").fetchone()[0]
        version = con.execute("PRAGMA user_version").fetchone()[0]
        tables = [row[0] for row in con.execute(
            "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name")]
        if integrity != "ok":
            raise ValueError("database quick_check failed: " + str(integrity))
        return {"schema": 1, "database": str(db), "user_version": version,
                "table_count": len(tables), "integrity": integrity,
                "note": "Read-only snapshot inspection; no migration or backup performed"}
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

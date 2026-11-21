#!/usr/bin/env python3
"""Create the repository test database without requiring the sqlite3 CLI."""

from pathlib import Path
import sqlite3
import sys


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: prepare_test_database.py DATABASE SCHEMA SAMPLE_DATA", file=sys.stderr)
        return 64
    database_path = Path(sys.argv[1])
    schema_path = Path(sys.argv[2])
    sample_path = Path(sys.argv[3])
    database_path.unlink(missing_ok=True)
    connection = sqlite3.connect(database_path)
    try:
        connection.execute("PRAGMA foreign_keys = ON")
        connection.executescript(schema_path.read_text(encoding="utf-8"))
        connection.executescript(sample_path.read_text(encoding="utf-8"))
        connection.commit()
    except (OSError, sqlite3.Error) as error:
        print(f"failed to prepare test database: {error}", file=sys.stderr)
        return 1
    finally:
        connection.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

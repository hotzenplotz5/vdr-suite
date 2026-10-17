#!/usr/bin/env python3
"""P7: verified historical Git schema to current additive SQL, offline only.

Historical source is the byte-exact SQL from tag
v1.60.15m-recording-actions-cache-consistency (annotated tag object
50e699956dbd4407a57680b6359dca54f000ee74, peeled commit 646020746bc449b220ff4f24b80992e5f38f0162).
This is not a test of older .deb packages or daemon runtime migrations.
"""
from contextlib import closing
import hashlib
from pathlib import Path
import sqlite3
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
LEGACY = HERE / "fixtures" / "packaging" / "v1.60.15m-vdr-suite.sql"
METADATA = HERE.parent / "database" / "schema" / "metadata-platform-v1.sql"
LEGACY_GIT_BLOB_SHA1 = "af0dd4265ca37ce8ef34f9ff1fbac18e079cf0ac"
TABLES = ("recordings", "metadata", "artwork", "jobs", "epg_events",
          "vdr_recording_cache", "epgsearch_native_fuzzy_capability_probes",
          "vdr_recording_cache_status")


def source_git_blob_hash(contents):
    return hashlib.sha1(
        b"blob " + str(len(contents)).encode("ascii") + b"\0" + contents
    ).hexdigest()


def create_legacy_database(path):
    """Never connects to an installed database; path belongs to test scratch."""
    with closing(sqlite3.connect(path)) as connection:
        connection.execute("PRAGMA foreign_keys=ON")
        connection.executescript(LEGACY.read_text(encoding="utf-8"))
        connection.execute(
            "INSERT INTO recordings(title,recording_path,recording_format) "
            "VALUES(?,?,?)", ("P7 legacy", "/offline/fixture.rec", "TS"))
        connection.execute(
            "INSERT INTO metadata(recording_id,media_type,title) "
            "VALUES(?,?,?)", (1, "MOVIE", "Legacy metadata"))
        connection.execute(
            "INSERT INTO artwork(recording_id,metadata_id,poster_path) "
            "VALUES(?,?,?)", (1, 1, "/offline/poster.jpg"))
        connection.execute(
            "INSERT INTO jobs(recording_id,job_type,status) VALUES(?,?,?)",
            (1, "SCRAPE", "PENDING"))
        connection.execute(
            "INSERT INTO epg_events(backend_id,channel_id,event_id,title,"
            "start_time,end_time) VALUES(?,?,?,?,?,?)",
            ("test", "one", "42", "Historical EPG", "2026-01-01", "2026-01-02"))
        connection.execute(
            "INSERT INTO vdr_recording_cache(backend_id,cache_key,title) "
            "VALUES(?,?,?)", ("test", "key", "Historical cache"))
        connection.execute(
            "INSERT INTO epgsearch_native_fuzzy_capability_probes(backend_id) "
            "VALUES(?)", ("test",))
        connection.execute(
            "INSERT INTO vdr_recording_cache_status(backend_id) VALUES(?)",
            ("test",))
        connection.commit()


def legacy_snapshot(connection):
    """Inspect original object definitions and data, not the new tables."""
    return {
        "objects": connection.execute(
            "SELECT type,name,sql FROM sqlite_master "
            "WHERE name NOT LIKE 'sqlite_%' AND "
            "name NOT LIKE 'suite_metadata_%' AND "
            "name NOT LIKE 'idx_suite_metadata_%' AND "
            "name NOT LIKE 'uq_suite_metadata_%' AND "
            "name NOT LIKE 'trg_suite_metadata_%' "
            "ORDER BY type,name").fetchall(),
        "rows": {
            name: connection.execute("SELECT * FROM " + name + " ORDER BY rowid")
            .fetchall() for name in TABLES
        },
        "user_version": connection.execute("PRAGMA user_version").fetchone()[0],
    }


def require_consistent(connection):
    integrity = connection.execute("PRAGMA integrity_check").fetchone()[0]
    if integrity != "ok":
        raise AssertionError("historical SQLite integrity failed: " + integrity)
    violations = connection.execute("PRAGMA foreign_key_check").fetchall()
    if violations:
        raise AssertionError("historical foreign-key violations: " + str(violations))


def apply_current_additive_metadata_schema(connection, fail_after=False):
    # The source SQL is executed within one SQLite transaction. This proves
    # only this *additive SQL file* on this historical fixture, not the runtime
    # repositories' many independent ensureSchema() paths.
    connection.execute("PRAGMA foreign_keys=ON")
    script = "BEGIN IMMEDIATE;\n" + METADATA.read_text(encoding="utf-8")
    if fail_after:
        script += "\nINSERT INTO p7_missing_table VALUES(1);\n"
    script += "\nCOMMIT;\n"
    try:
        connection.executescript(script)
    except sqlite3.Error:
        connection.rollback()
        raise


class HistoricalSchemaRehearsalTests(unittest.TestCase):
    def test_historical_schema_is_exact_tagged_git_blob(self):
        self.assertEqual(
            source_git_blob_hash(LEGACY.read_bytes()), LEGACY_GIT_BLOB_SHA1)

    def test_additive_schema_idempotent_and_preserves_historical_data(self):
        with tempfile.TemporaryDirectory(prefix="p7-historical-") as scratch:
            db = Path(scratch) / "old.db"
            create_legacy_database(db)
            with closing(sqlite3.connect(db)) as connection:
                connection.execute("PRAGMA foreign_keys=ON")
                before = legacy_snapshot(connection)
                require_consistent(connection)
                self.assertEqual({name: len(before["rows"][name])
                                  for name in TABLES},
                                 {name: 1 for name in TABLES})
                apply_current_additive_metadata_schema(connection)
                require_consistent(connection)
                self.assertEqual(legacy_snapshot(connection), before)
                versions = connection.execute(
                    "SELECT version FROM suite_metadata_schema_versions "
                    "ORDER BY version").fetchall()
                self.assertEqual(versions, [(1,)])
                count = connection.execute(
                    "SELECT count(*) FROM sqlite_master WHERE "
                    "name LIKE 'suite_metadata_%'").fetchone()[0]
                self.assertGreater(count, 1)
                apply_current_additive_metadata_schema(connection)
                require_consistent(connection)
                self.assertEqual(legacy_snapshot(connection), before)
                self.assertEqual(
                    connection.execute(
                        "SELECT version FROM suite_metadata_schema_versions "
                        "ORDER BY version").fetchall(), versions)

    def test_failed_schema_transaction_leaves_historical_data_intact(self):
        with tempfile.TemporaryDirectory(prefix="p7-historical-") as scratch:
            db = Path(scratch) / "old.db"
            create_legacy_database(db)
            with closing(sqlite3.connect(db)) as connection:
                connection.execute("PRAGMA foreign_keys=ON")
                before = legacy_snapshot(connection)
                with self.assertRaisesRegex(sqlite3.Error, "p7_missing_table"):
                    apply_current_additive_metadata_schema(
                        connection, fail_after=True)
                require_consistent(connection)
                self.assertEqual(legacy_snapshot(connection), before)
                self.assertEqual(connection.execute(
                    "SELECT count(*) FROM sqlite_master "
                    "WHERE name='suite_metadata_schema_versions'"
                ).fetchone()[0], 0)

    def test_verified_backup_restores_preupgrade_schema_and_rows(self):
        with tempfile.TemporaryDirectory(prefix="p7-historical-") as scratch:
            folder = Path(scratch)
            old, backup, recovered = (
                folder / name for name in ("old.db", "before.db", "restored.db"))
            create_legacy_database(old)
            with closing(sqlite3.connect(old)) as source, closing(
                    sqlite3.connect(backup)) as target:
                source.backup(target)
                before = legacy_snapshot(source)
            with closing(sqlite3.connect(old)) as connection:
                apply_current_additive_metadata_schema(connection)
                self.assertNotEqual(legacy_snapshot(connection), {})
                self.assertEqual(connection.execute(
                    "SELECT count(*) FROM suite_metadata_schema_versions"
                ).fetchone()[0], 1)
            with closing(sqlite3.connect(backup)) as source, closing(
                    sqlite3.connect(recovered)) as target:
                source.backup(target)
                require_consistent(target)
                self.assertEqual(legacy_snapshot(target), before)
                self.assertEqual(target.execute(
                    "SELECT count(*) FROM sqlite_master "
                    "WHERE name='suite_metadata_schema_versions'"
                ).fetchone()[0], 0)


if __name__ == "__main__":
    unittest.main()

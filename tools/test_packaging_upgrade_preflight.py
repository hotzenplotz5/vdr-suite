import importlib.util
import hashlib
import os
from unittest import mock
from contextlib import closing
from pathlib import Path
import sqlite3
import tempfile
import unittest

module_path = Path(__file__).with_name("packaging_upgrade_preflight.py")
spec = importlib.util.spec_from_file_location("preflight", module_path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

recovery_path = Path(__file__).with_name("packaging_recovery_rehearsal.py")
recovery_spec = importlib.util.spec_from_file_location(
    "packaging_recovery_rehearsal", recovery_path)
recovery = importlib.util.module_from_spec(recovery_spec)
recovery_spec.loader.exec_module(recovery)

class PreflightTests(unittest.TestCase):
    def test_valid(self):
        with tempfile.TemporaryDirectory() as directory:
            db = Path(directory) / "test.db"
            con = sqlite3.connect(db)
            con.execute("CREATE TABLE example (id INTEGER PRIMARY KEY)")
            con.execute("PRAGMA user_version=7")
            con.commit()
            con.close()
            result = module.check(db)
            self.assertEqual(result["user_version"], 7)
            self.assertEqual(result["integrity"], "ok")
            self.assertEqual(result["schema"], 4)
            self.assertEqual(result["compatibility"], "REVIEW_REQUIRED")
            self.assertEqual(result["foreign_key_check"], "ok")
            self.assertEqual(result["schema_object_count"], 1)
            self.assertRegex(result["schema_sha256"], r"^[0-9a-f]{64}$")
            self.assertEqual(result["file_sha256"],
                             hashlib.sha256(db.read_bytes()).hexdigest())
            self.assertEqual(result["file_size_bytes"], db.stat().st_size)

    def test_rejects_committed_wal_without_ignoring_its_data(self):
        with tempfile.TemporaryDirectory() as directory:
            db = Path(directory) / "wal.db"
            # Keep the writer connection open so committed data stays in WAL.
            with closing(sqlite3.connect(db)) as conn:
                self.assertEqual(
                    conn.execute("PRAGMA journal_mode=WAL").fetchone()[0], "wal")
                conn.execute("CREATE TABLE committed_only_in_wal (x INTEGER)")
                conn.execute("INSERT INTO committed_only_in_wal VALUES (42)")
                conn.commit()
                wal = Path(str(db) + "-wal")
                before_main, before_wal = db.read_bytes(), wal.read_bytes()
                with self.assertRaisesRegex(ValueError, "sidecar present"):
                    module.check(db)
                self.assertEqual(db.read_bytes(), before_main)
                self.assertEqual(wal.read_bytes(), before_wal)
                self.assertEqual(
                    conn.execute("SELECT x FROM committed_only_in_wal").fetchone()[0], 42)

    def test_rejects_stale_journal_and_dangling_shm_symlink(self):
        with tempfile.TemporaryDirectory() as directory:
            db = Path(directory) / "old.db"
            with closing(sqlite3.connect(db)) as con:
                con.execute("CREATE TABLE sample (n INTEGER)")
                con.commit()
            journal = Path(str(db) + "-journal")
            journal.write_bytes(b"untrusted journal bytes")
            with self.assertRaisesRegex(ValueError, "sidecar present"):
                module.check(db)
            self.assertEqual(journal.read_bytes(), b"untrusted journal bytes")
            journal.unlink()
            shm = Path(str(db) + "-shm")
            shm.symlink_to(Path(directory) / "missing")
            with self.assertRaisesRegex(ValueError, "sidecar present"):
                module.check(db)
            self.assertTrue(shm.is_symlink())

    def test_reports_known_schema_versions_read_only(self):
        with tempfile.TemporaryDirectory() as directory:
            db = Path(directory) / "markers.db"
            with closing(sqlite3.connect(db)) as con:
                con.executescript(
                    "CREATE TABLE suite_metadata_schema_versions("
                    "version INTEGER PRIMARY KEY,description TEXT);"
                    "INSERT INTO suite_metadata_schema_versions VALUES(1,'base');"
                    "INSERT INTO suite_metadata_schema_versions VALUES(7,'manual');"
                    "CREATE TABLE vdr_global_search_schema_versions("
                    "version INTEGER PRIMARY KEY,description TEXT);"
                    "INSERT INTO vdr_global_search_schema_versions VALUES(1,'backfill');")
            before = db.read_bytes()
            data = module.check(db)
            self.assertEqual(data["schema"], 4)
            self.assertEqual(data["known_schema_versions"], {
                "suite_metadata_schema_versions": {
                    "applied_versions": [1, 7], "highest_version": 7},
                "vdr_global_search_schema_versions": {
                    "applied_versions": [1], "highest_version": 1},
            })
            self.assertEqual(db.read_bytes(), before)

    def test_invalid_schema_marker_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            db = Path(directory) / "invalid.db"
            with closing(sqlite3.connect(db)) as con:
                con.executescript(
                    "CREATE TABLE suite_metadata_schema_versions(version TEXT);"
                    "INSERT INTO suite_metadata_schema_versions VALUES('unknown');")
            with self.assertRaisesRegex(ValueError, "invalid version marker"):
                module.check(db)

    def test_schema_fingerprint_is_stable_and_matches_recovery_rehearsal(self):
        with tempfile.TemporaryDirectory() as directory:
            db = Path(directory) / "evidence.db"
            with closing(sqlite3.connect(db)) as conn:
                conn.executescript(
                    "CREATE TABLE sample (id INTEGER PRIMARY KEY, secret TEXT);"
                    "INSERT INTO sample VALUES (1, 'private-p7-row');")
            before = db.read_bytes()
            initial = module.check(db)
            self.assertEqual(db.read_bytes(), before)
            # Compare with the actual offline recovery implementation,
            # not a duplicate formula that could drift independently.
            with closing(sqlite3.connect(db)) as conn:
                expected = recovery.sqlite_facts(conn)["schema_sha256"]
            self.assertEqual(initial["schema_sha256"], expected)
            self.assertNotIn("private-p7-row", str(initial))
            self.assertEqual(initial["file_sha256"],
                             hashlib.sha256(before).hexdigest())
            self.assertEqual(initial["file_size_bytes"], len(before))
            with closing(sqlite3.connect(db)) as conn:
                conn.execute(
                    "INSERT INTO sample VALUES(2, 'another-confidential-value')")
                conn.commit()
            changed_rows = module.check(db)
            self.assertEqual(initial["schema_sha256"],
                             changed_rows["schema_sha256"])
            self.assertNotEqual(initial["file_sha256"],
                                changed_rows["file_sha256"])
            with closing(sqlite3.connect(db)) as conn:
                conn.execute("ALTER TABLE sample ADD COLUMN note TEXT")
                conn.commit()
            changed_schema = module.check(db)
            self.assertNotEqual(initial["schema_sha256"],
                                changed_schema["schema_sha256"])
            self.assertEqual(changed_schema["compatibility"],
                             "REVIEW_REQUIRED")

    def test_foreign_key_violation_is_rejected_without_mutation(self):
        with tempfile.TemporaryDirectory() as directory:
            db = Path(directory) / "orphan.db"
            with closing(sqlite3.connect(db)) as conn:
                conn.executescript(
                    "PRAGMA foreign_keys=OFF;"
                    "CREATE TABLE parent (id INTEGER PRIMARY KEY);"
                    "CREATE TABLE child (id INTEGER PRIMARY KEY,"
                    "parent_id INTEGER REFERENCES parent(id));"
                    "INSERT INTO child VALUES(1, 404);")
            before = db.read_bytes()
            with self.assertRaisesRegex(ValueError, "foreign_key_check failed"):
                module.check(db)
            self.assertEqual(db.read_bytes(), before)

    def test_file_replacement_during_inspection_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            db = Path(directory) / "source.db"
            replacement = Path(directory) / "replacement.db"
            for path, value in ((db, "old"), (replacement, "replacement")):
                with closing(sqlite3.connect(path)) as conn:
                    conn.execute("CREATE TABLE same_schema (value TEXT)")
                    conn.execute("INSERT INTO same_schema VALUES (?)", (value,))
                    conn.commit()
            original_inspector = module.inspect_known_versions

            def replace_after_database_open(con, table_names):
                # Only a private test fixture. The SQLite reader still holds
                # its original inode; the pathname now points at another DB.
                os.replace(replacement, db)
                return original_inspector(con, table_names)

            with mock.patch.object(module, "inspect_known_versions",
                                   side_effect=replace_after_database_open):
                with self.assertRaisesRegex(ValueError, "changed during inspection"):
                    module.check(db)
            with closing(sqlite3.connect(db)) as conn:
                self.assertEqual(conn.execute(
                    "SELECT value FROM same_schema").fetchone()[0],
                                 "replacement")

    def test_hardlink_to_database_is_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            db = Path(directory) / "source.db"
            alias = Path(directory) / "other-name.db"
            with closing(sqlite3.connect(db)) as conn:
                conn.execute("CREATE TABLE records(id INTEGER)")
                conn.commit()
            os.link(db, alias)
            before = db.read_bytes()
            with self.assertRaisesRegex(ValueError, "single-link regular file"):
                module.check(db)
            self.assertEqual(db.read_bytes(), before)
            self.assertEqual(alias.read_bytes(), before)

    def test_does_not_create_database(self):
        with tempfile.TemporaryDirectory() as directory:
            db = Path(directory) / "missing.db"
            with self.assertRaises(ValueError):
                module.check(db)
            self.assertFalse(db.exists())

if __name__ == "__main__":
    unittest.main()

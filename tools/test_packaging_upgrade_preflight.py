import importlib.util
from contextlib import closing
from pathlib import Path
import sqlite3
import tempfile
import unittest

module_path = Path(__file__).with_name("packaging_upgrade_preflight.py")
spec = importlib.util.spec_from_file_location("preflight", module_path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

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

    def test_does_not_create_database(self):
        with tempfile.TemporaryDirectory() as directory:
            db = Path(directory) / "missing.db"
            with self.assertRaises(ValueError):
                module.check(db)
            self.assertFalse(db.exists())

if __name__ == "__main__":
    unittest.main()

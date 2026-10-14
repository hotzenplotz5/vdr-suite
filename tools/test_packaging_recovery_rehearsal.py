#!/usr/bin/env python3
"""Scratch-only, non-root P7 SQLite backup/recovery regression tests."""
import importlib.util
from contextlib import closing
import os
from pathlib import Path
import sqlite3
import stat
import tempfile
from unittest.mock import patch
from types import SimpleNamespace
import unittest

SCRIPT = Path(__file__).with_name("packaging_recovery_rehearsal.py")
spec = importlib.util.spec_from_file_location("packaging_recovery", SCRIPT)
recovery = importlib.util.module_from_spec(spec)
spec.loader.exec_module(recovery)

BIG_DISK = lambda _: SimpleNamespace(total=100 * 1024**3, free=80 * 1024**3)


class RecoveryRehearsalTests(unittest.TestCase):
    def make_database(self, folder, wal=False):
        path = folder / "original.db"
        con = sqlite3.connect(path)
        if wal:
            con.execute("PRAGMA journal_mode=WAL")
        con.execute("CREATE TABLE entries (id INTEGER PRIMARY KEY, value TEXT)")
        con.execute("PRAGMA user_version=17")
        con.execute("INSERT INTO entries VALUES (1, 'committed')")
        con.commit()
        return path, con

    def test_restores_committed_wal_and_preserves_source(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            path, con = self.make_database(folder, wal=True)
            try:
                wal = Path(str(path) + "-wal")
                self.assertTrue(wal.exists())
                before_db = path.read_bytes()
                before_wal = wal.read_bytes()
                result = recovery.rehearse(folder, path.name, "backup.db",
                                           "restored.db", capacity=BIG_DISK)
                self.assertEqual(result["result"], "P7_OFFLINE_RECOVERY_REHEARSAL_PASS")
                self.assertEqual(result["user_version"], 17)
                self.assertEqual(result["integrity"], "ok")
                self.assertEqual(path.read_bytes(), before_db)
                self.assertEqual(wal.read_bytes(), before_wal)
                # sqlite3.Connection.__exit__ commits but does not close the
                # handle. Closing prevents Python 3.13+ ResourceWarning.
                with closing(sqlite3.connect(folder / "restored.db")) as checked:
                    self.assertEqual(
                        checked.execute("SELECT value FROM entries WHERE id=1").fetchone()[0],
                        "committed")
                # Regression gate: the readback handle must really be closed.
                with self.assertRaises(sqlite3.ProgrammingError):
                    checked.execute("SELECT 1")
                for name in ("backup.db", "restored.db"):
                    self.assertEqual(stat.S_IMODE((folder / name).stat().st_mode), 0o600)
                self.assertEqual(
                    recovery.file_hash(folder / "backup.db"),
                    recovery.file_hash(folder / "restored.db"))
            finally:
                con.close()

    def test_refuses_to_overwrite_backup(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            path, con = self.make_database(folder)
            con.close()
            (folder / "backup.db").write_text("owned", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "refusing to overwrite"):
                recovery.rehearse(folder, path.name, "backup.db", "restored.db",
                                  capacity=BIG_DISK)
            self.assertEqual((folder / "backup.db").read_text(), "owned")
            self.assertFalse((folder / "restored.db").exists())

    def test_refuses_path_escape_or_reused_names(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            path, con = self.make_database(folder)
            con.close()
            for invalid in ("../outside.db", "/etc/passwd", "..", "a/b"):
                with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                    recovery.rehearse(folder, invalid, "backup.db", "restored.db",
                                      capacity=BIG_DISK)
            with self.assertRaisesRegex(ValueError, "must differ"):
                recovery.rehearse(folder, path.name, path.name, "restored.db",
                                  capacity=BIG_DISK)

    def test_refuses_symlink_source_and_sidecar(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            path, con = self.make_database(folder)
            con.close()
            (folder / "linked.db").symlink_to(path)
            with self.assertRaisesRegex(ValueError, "non-symlink"):
                recovery.rehearse(folder, "linked.db", "backup.db", "restored.db",
                                  capacity=BIG_DISK)
            Path(str(path) + "-wal").symlink_to(path)
            with self.assertRaisesRegex(ValueError, "sidecar"):
                recovery.rehearse(folder, path.name, "backup.db", "restored.db",
                                  capacity=BIG_DISK)

    def test_refuses_nonprivate_workspace(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            path, con = self.make_database(folder)
            con.close()
            folder.chmod(0o755)
            try:
                with self.assertRaisesRegex(ValueError, "private"):
                    recovery.rehearse(folder, path.name, "backup.db", "restored.db",
                                      capacity=BIG_DISK)
            finally:
                folder.chmod(0o700)

    def test_protected_system_and_codec_checkout_paths_rejected(self):
        for name in ("/var/lib/vdr-suite", "/etc/vdr-suite",
                     "/home/yavdr/vdr-suite", "/home/yavdr/vdr-suite-packaging-test"):
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, "protected"):
                recovery.require_workspace(Path(name))

    def test_concurrent_restore_creation_is_not_deleted_on_failure(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            original, con = self.make_database(folder)
            con.close()
            restored = folder / "restored.db"
            real_open = recovery.os.open

            def raced_open(path, flags, mode=0o777):
                if Path(path) == restored:
                    restored.write_text("belongs to another writer", encoding="utf-8")
                    raise FileExistsError("exclusive create failed")
                return real_open(path, flags, mode)

            with patch.object(recovery.os, "open", side_effect=raced_open):
                with self.assertRaises(FileExistsError):
                    recovery.rehearse(folder, original.name, "backup.db",
                                      restored.name, capacity=BIG_DISK)
            self.assertEqual(restored.read_text(encoding="utf-8"),
                             "belongs to another writer")
            self.assertFalse((folder / "backup.db").exists())

    def test_capacity_stop_occurs_before_first_copy(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            path, con = self.make_database(folder)
            con.close()
            small_disk = lambda _: SimpleNamespace(total=16 * 1024**3,
                                                    free=4 * 1024**3)
            with self.assertRaisesRegex(ValueError, "capacity"):
                recovery.rehearse(folder, path.name, "backup.db", "restored.db",
                                  capacity=small_disk)
            self.assertFalse((folder / "backup.db").exists())
            self.assertFalse((folder / "restored.db").exists())


if __name__ == "__main__":
    unittest.main()

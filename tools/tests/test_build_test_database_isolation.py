"""Regression: separate make BUILD_DIR values never share the SQLite test fixture."""

from pathlib import Path
import sqlite3
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class MakeTestDatabaseIsolation(unittest.TestCase):
    def test_separate_build_directories_receive_independent_fixtures(self):
        with tempfile.TemporaryDirectory(prefix="vdr-suite-build-isolation-") as temp:
            base = Path(temp)
            first = base / "builder-a"
            second = base / "builder-b"
            paths = []
            for directory in (first, second):
                subprocess.run(
                    ["make", "--no-print-directory", "-C", str(ROOT),
                     f"BUILD_DIR={directory}", "prepare-test-db"],
                    check=True, capture_output=True, text=True, timeout=60,
                )
                database = directory / "vdr-suite-test.db"
                self.assertTrue(database.is_file(), str(database))
                with sqlite3.connect(database) as connection:
                    rows = connection.execute(
                        "SELECT COUNT(*) FROM sqlite_master WHERE type='table'"
                    ).fetchone()[0]
                    self.assertGreater(rows, 0)
                paths.append(database)
            self.assertNotEqual(paths[0], paths[1])
            # Removing one checkout's fixture cannot affect the other.
            paths[0].unlink()
            self.assertTrue(paths[1].is_file())


if __name__ == "__main__":
    unittest.main()

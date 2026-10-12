import importlib.util
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

    def test_does_not_create_database(self):
        with tempfile.TemporaryDirectory() as directory:
            db = Path(directory) / "missing.db"
            with self.assertRaises(ValueError):
                module.check(db)
            self.assertFalse(db.exists())

if __name__ == "__main__":
    unittest.main()

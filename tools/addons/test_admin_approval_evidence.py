"""Root-managed approval record remains a non-authorizing optional read."""
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from addon_contract import ROOT, stage
from admin_approval_evidence import APPROVAL_PATH, examine
from installed_registry import inspect, plan_enable


class ApprovalEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="addon-approved-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        stage(ROOT, "rectools", str(self.root))
        self.record = self.root / APPROVAL_PATH
        self.record.parent.mkdir(parents=True)
        self.data = {
            "schemaVersion": 1, "id": "rectools",
            "package": "vdr-suite-addon-media-tools",
            "version": "0.1.0", "sha256": "a" * 64, "origin": "manual",
            "state": "approved",
        }
        self.write(self.data)

    def write(self, value):
        self.record.write_text(json.dumps(value), encoding="utf-8")
        self.record.chmod(0o600)

    def test_default_registry_does_not_read_admin_records(self):
        with patch("installed_registry.examine_approval",
                   side_effect=AssertionError("unexpected approval check")):
            modules = inspect(self.root)["modules"]
            self.assertFalse(any("adminApprovalEvidence" in x for x in modules))
            self.assertEqual(plan_enable(self.root, "rectools")["decision"], "denied")

    def test_optional_evidence_never_activates(self):
        modules = inspect(self.root, verify_admin_approval=True)["modules"]
        rectools = next(x for x in modules if x["id"] == "rectools")
        self.assertIn("adminApprovalEvidence", rectools)
        self.assertFalse(rectools["active"])
        self.assertEqual(rectools["capabilities"], [])
        self.assertNotIn("packageTrusted", rectools)
        self.assertEqual(plan_enable(self.root, "rectools",
                                     verify_admin_approval=True)["decision"], "denied")

    def test_absent_or_nonreviewed_record_fails_closed(self):
        self.record.unlink()
        self.assertFalse(examine(self.root, "rectools")["consistent"])
        for name in ("image", "music"):
            self.assertEqual(examine(self.root, name)["reason"], "no-reviewed-approval")

    def test_symlink_record_rejected(self):
        self.record.rename(self.record.with_suffix(".original"))
        self.record.symlink_to(self.record.with_suffix(".original"))
        self.assertFalse(examine(self.root, "rectools")["consistent"])

    def test_writable_parent_and_record_rejected(self):
        self.record.parent.chmod(0o777)
        self.assertEqual(examine(self.root, "rectools")["reason"],
                         "unsafe-approval-parent")
        self.record.parent.chmod(0o700)
        self.record.chmod(0o666)
        self.assertFalse(examine(self.root, "rectools")["consistent"])

    def test_invalid_root_and_schema_rejected(self):
        self.assertEqual(examine(Path("relative"), "rectools")["reason"], "unsafe-root")
        self.data["origin"] = "invented"
        self.write(self.data)
        self.assertFalse(examine(self.root, "rectools")["consistent"])

    def test_version_and_identity_cannot_drift(self):
        from unittest.mock import patch
        from types import SimpleNamespace
        original = os.fstat

        def synthetic_root_owned(fd):
            actual = original(fd)
            # Simulate metadata on already opened FDs without modifying
            # privileges or a host-owned approval record.
            return SimpleNamespace(
                st_uid=0, st_mode=actual.st_mode & ~0o022,
                st_dev=actual.st_dev, st_ino=actual.st_ino,
                st_size=actual.st_size, st_mtime_ns=actual.st_mtime_ns,
                st_ctime_ns=actual.st_ctime_ns,
            )

        with patch("admin_approval_evidence.os.fstat", synthetic_root_owned):
            with patch("admin_approval_evidence.examine_binary",
                       return_value={"consistent": True, "reason": "fixture"}):
                self.assertTrue(examine(self.root, "rectools")["consistent"])
                self.data["version"] = "9.9.9"
                self.write(self.data)
                self.assertEqual(examine(self.root, "rectools")["reason"],
                                 "approval-metadata-mismatch")
                self.data["version"] = "0.1.0"
                self.data["state"] = "revoked"
                self.write(self.data)
                self.assertEqual(examine(self.root, "rectools")["reason"],
                                 "approval-revoked")
                self.data["state"] = "approved"
                self.data["id"] = "image"
                self.write(self.data)
                self.assertEqual(examine(self.root, "rectools")["reason"],
                                 "invalid-approval-identity")

    def test_cli_is_read_only(self):
        import subprocess
        script = Path(__file__).with_name("installed_registry.py")
        before = self.record.read_bytes()
        completed = subprocess.run(
            ["python3", str(script), "inventory", "--root", str(self.root),
             "--verify-admin-approval"], capture_output=True, text=True, check=True)
        modules = json.loads(completed.stdout)["modules"]
        self.assertIn("adminApprovalEvidence",
                      next(x for x in modules if x["id"] == "rectools"))
        self.assertEqual(before, self.record.read_bytes())


if __name__ == "__main__":
    unittest.main()

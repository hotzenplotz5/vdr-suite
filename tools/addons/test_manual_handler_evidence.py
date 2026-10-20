"""Isolated, non-executing manual handler evidence tests."""
import hashlib
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

from addon_contract import ROOT, stage
from installed_registry import inspect, plan_enable
from manual_handler_evidence import HANDLER, examine


class ManualHandlerEvidenceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="addon-manual-evidence-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.handler = self.root / HANDLER
        self.handler.parent.mkdir(parents=True)
        self.handler.write_bytes(b"not an executable test fixture")
        self.handler.chmod(0o700)
        stage(ROOT, "rectools", str(self.root))
        self.digest = hashlib.sha256(self.handler.read_bytes()).hexdigest()

    def evidence(self, digest=None):
        return examine(self.root, "rectools", self.digest if digest is None else digest)

    def test_default_registry_has_no_manual_or_dpkg_dependency(self):
        entry = next(x for x in inspect(self.root)["modules"] if x["id"] == "rectools")
        self.assertNotIn("manualHandlerEvidence", entry)
        self.assertNotIn("dpkgEvidence", entry)
        self.assertFalse(entry["active"])
        self.assertEqual(entry["capabilities"], [])

    def test_digest_not_supplied_cannot_activate(self):
        self.assertEqual(plan_enable(self.root, "rectools")["decision"], "denied")
        self.assertEqual(self.evidence("garbage")["reason"], "invalid-approved-digest")

    def test_wrong_digest_fails_closed(self):
        result = self.evidence("0" * 64)
        self.assertFalse(result["consistent"])
        self.assertIn(result["reason"], ("handler-digest-mismatch", "unsafe-handler-ownership", "writable-handler-parent"))

    def test_absent_handler_fails_closed(self):
        self.handler.unlink()
        self.assertFalse(self.evidence()["consistent"])

    def test_symlink_handler_fails_closed(self):
        self.handler.rename(self.handler.with_suffix(".original"))
        self.handler.symlink_to(self.handler.with_suffix(".original"))
        self.assertIn(self.evidence()["reason"], ("unsafe-handler-file", "writable-handler-parent"))

    def test_writable_parent_fails_closed(self):
        self.handler.parent.chmod(0o777)
        self.assertEqual(self.evidence()["reason"], "writable-handler-parent")

    def test_executable_permission_required(self):
        self.handler.chmod(0o600)
        result = self.evidence()
        self.assertIn(result["reason"], ("handler-not-executable", "writable-handler-parent"))

    def test_non_rectools_has_no_handler(self):
        self.assertEqual(examine(self.root, "image", self.digest)["reason"],
                         "no-reviewed-manual-handler")
        self.assertEqual(examine(self.root, "tvscraper", self.digest)["reason"],
                         "no-reviewed-manual-handler")

    def test_registry_diagnostic_never_grants_capability(self):
        inventory = inspect(self.root, manual_sha256=self.digest)
        entry = next(x for x in inventory["modules"] if x["id"] == "rectools")
        self.assertIn("manualHandlerEvidence", entry)
        self.assertFalse(entry["active"])
        self.assertEqual(entry["capabilities"], [])
        self.assertNotIn("packageTrusted", entry)
        self.assertEqual(plan_enable(self.root, "rectools",
                                     manual_sha256=self.digest)["decision"], "denied")

    def test_cli_keeps_handler_unexecuted_and_files_unchanged(self):
        script = Path(__file__).with_name("installed_registry.py")
        before = self.handler.read_bytes()
        result = subprocess.run(["python3", str(script), "inventory",
                                 "--root", str(self.root),
                                 "--manual-sha256", self.digest],
                                check=True, capture_output=True, text=True)
        self.assertEqual(len(json.loads(result.stdout)["modules"]), 3)
        self.assertEqual(before, self.handler.read_bytes())

    @unittest.skipUnless(os.geteuid() == 0, "root ownership fixture only")
    def test_matching_root_owned_handler_is_diagnostic_not_trust(self):
        initial = self.evidence()
        # A /tmp fixture is deliberately rejected when an ancestor is
        # group/world writable, even for a root-owned test executable.
        self.assertIn(initial["reason"], (
            "writable-handler-parent", "manual-handler-bytes-match-approved-digest"))
        self.handler.write_bytes(b"tampered")
        altered = self.evidence()
        if initial["consistent"]:
            self.assertEqual(altered["reason"], "handler-digest-mismatch")
        else:
            self.assertEqual(altered["reason"], "writable-handler-parent")


if __name__ == "__main__":
    unittest.main()

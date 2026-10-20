"""Root-owned isolated proof of manual add-on approval and revocation.

Run only inside an ephemeral CI runner as root with TMPDIR=/root.
No live VDR-Suite installation or system paths are changed.
"""
import hashlib
import json
import os
import tempfile
import unittest
from pathlib import Path

from addon_contract import ROOT, stage
from admin_approval_evidence import APPROVAL_PATH, examine
from manual_handler_evidence import HANDLER


@unittest.skipUnless(os.geteuid() == 0, "root-owned install fixture only")
class RootOwnedApprovalAcceptance(unittest.TestCase):
    def setUp(self):
        parent = Path(os.environ.get("TMPDIR", ""))
        if parent != Path("/root") or not parent.is_dir():
            self.skipTest("requires explicitly isolated /root fixture on disposable runner")
        self.temp = tempfile.TemporaryDirectory(prefix="vdr-addon-root-proof-", dir=parent)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        stage(ROOT, "rectools", str(self.root))
        self.handler = self.root / HANDLER
        self.handler.parent.mkdir(parents=True)
        self.handler.write_bytes(b"nonfunctional handler fixture only\n")
        self.handler.chmod(0o700)
        self.approval = self.root / APPROVAL_PATH
        self.approval.parent.mkdir(parents=True)
        self.document = {
            "schemaVersion": 1,
            "id": "rectools",
            "package": "vdr-suite-addon-media-tools",
            "version": "0.1.0",
            "sha256": hashlib.sha256(self.handler.read_bytes()).hexdigest(),
            "origin": "manual",
            "state": "approved",
        }
        self.write()

    def write(self):
        self.approval.write_text(json.dumps(self.document), encoding="utf-8")
        self.approval.chmod(0o600)

    def test_real_root_ownership_and_bytes_match(self):
        self.assertEqual(examine(self.root, "rectools"),
            {"consistent": True, "reason": "root-owned-approval-and-binary-match"})

    def test_tampered_binary_is_rejected(self):
        self.handler.write_bytes(b"changed")
        self.assertEqual(examine(self.root, "rectools")["reason"],
                         "approved-binary-handler-digest-mismatch")

    def test_revoked_installation_is_rejected(self):
        self.document["state"] = "revoked"
        self.write()
        self.assertEqual(examine(self.root, "rectools")["reason"], "approval-revoked")

    def test_removed_binary_is_rejected(self):
        self.handler.unlink()
        self.assertFalse(examine(self.root, "rectools")["consistent"])

    def test_approval_version_drift_is_rejected(self):
        self.document["version"] = "0.2.0"
        self.write()
        self.assertEqual(examine(self.root, "rectools")["reason"],
                         "approval-metadata-mismatch")

    def test_writable_parent_is_rejected(self):
        self.handler.parent.chmod(0o777)
        self.assertEqual(examine(self.root, "rectools")["reason"],
                         "approved-binary-unsafe-handler-parent")


if __name__ == "__main__":
    unittest.main()

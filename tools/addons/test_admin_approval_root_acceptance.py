"""Root-owned isolated proof of manual add-on approval and revocation.

Run only inside an ephemeral CI runner as root with TMPDIR=/root.
No live VDR-Suite installation or system paths are changed.
"""
import hashlib
import json
import os
import subprocess
import sys
import stat
import tempfile
import unittest
from pathlib import Path

from addon_contract import ROOT, stage
from admin_approval_evidence import APPROVAL_PATH, examine
from admin_approval_issuer import issue, rotate, revoke, IssueError
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

    def test_admin_issuer_is_atomic_and_no_overwrite(self):
        self.approval.unlink()
        created = issue(self.root, "rectools", "0.1.0",
                        self.document["sha256"], "manual")
        self.assertEqual(created, self.approval)
        published = self.approval.read_bytes()
        self.assertEqual(stat.S_IMODE(self.approval.stat().st_mode), 0o644)
        self.assertTrue(examine(self.root, "rectools")["consistent"])
        with self.assertRaises(IssueError):
            issue(self.root, "rectools", "0.1.0",
                  self.document["sha256"], "manual")
        self.assertEqual(self.approval.read_bytes(), published)

    def test_rotation_requires_exact_old_hash_and_independent_new_bytes(self):
        old = self.document["sha256"]
        self.handler.write_bytes(b"reviewed replacement fixture bytes\n")
        replacement = hashlib.sha256(self.handler.read_bytes()).hexdigest()
        before = self.approval.read_bytes()
        with self.assertRaisesRegex(IssueError, "invalid-approval-rotation-digests"):
            rotate(self.root, "rectools", "0.1.0", old, old, "manual")
        with self.assertRaises(IssueError):
            rotate(self.root, "rectools", "0.1.0", old, "0" * 64, "manual")
        with self.assertRaisesRegex(IssueError, "stale-approval-rotation"):
            rotate(self.root, "rectools", "0.1.0", "a" * 64,
                   replacement, "manual")
        self.assertEqual(self.approval.read_bytes(), before)
        self.assertEqual(rotate(self.root, "rectools", "0.1.0",
                                old, replacement, "manual"), self.approval)
        self.assertEqual(json.loads(self.approval.read_text())["sha256"],
                         replacement)
        self.assertEqual(stat.S_IMODE(self.approval.stat().st_mode), 0o644)
        self.assertTrue(examine(self.root, "rectools")["consistent"])
        published = self.approval.read_bytes()
        with self.assertRaisesRegex(IssueError, "stale-approval-rotation"):
            rotate(self.root, "rectools", "0.1.0", old,
                   replacement, "manual")
        self.assertEqual(self.approval.read_bytes(), published)

    def test_rotation_fails_closed_on_revoke_missing_and_unsafe_input(self):
        old = self.document["sha256"]
        self.handler.write_bytes(b"replacement for revoked fixture\n")
        replacement = hashlib.sha256(self.handler.read_bytes()).hexdigest()
        with self.assertRaisesRegex(IssueError, "live-root-requires-explicit-confirmation"):
            rotate(Path("/"), "rectools", "0.1.0", old,
                   replacement, "manual")
        with self.assertRaisesRegex(IssueError, "unreviewed-handler-version"):
            rotate(self.root, "rectools", "0.2.0", old,
                   replacement, "manual")
        with self.assertRaisesRegex(IssueError, "invalid-approval-record"):
            rotate(self.root, "rectools", "0.1.0", old,
                   replacement, "debian")
        self.approval.unlink()
        with self.assertRaises(IssueError):
            rotate(self.root, "rectools", "0.1.0", old,
                   replacement, "manual")
        self.write()
        self.assertEqual(revoke(self.root, "rectools"), self.approval)
        revoked_bytes = self.approval.read_bytes()
        with self.assertRaisesRegex(IssueError, "approval-revoked"):
            rotate(self.root, "rectools", "0.1.0", old,
                   replacement, "manual")
        self.assertEqual(self.approval.read_bytes(), revoked_bytes)

    def test_cli_rotation_is_explicit_and_reproducible(self):
        old = self.document["sha256"]
        self.handler.write_bytes(b"rotated by cli fixture\n")
        replacement = hashlib.sha256(self.handler.read_bytes()).hexdigest()
        command = [
            sys.executable,
            str(Path(__file__).with_name("admin_approval_issuer.py")),
            "rotate", "--root", str(self.root), "--module", "rectools",
            "--version", "0.1.0", "--expected-sha256", old,
            "--approved-sha256", replacement, "--origin", "manual",
        ]
        result = subprocess.run(command, capture_output=True, text=True,
                                check=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("ADDON_ADMIN_APPROVAL_ROTATED=", result.stdout)
        self.assertTrue(examine(self.root, "rectools")["consistent"])
        replay = subprocess.run(command, capture_output=True, text=True,
                                check=False)
        self.assertEqual(replay.returncode, 2)
        self.assertIn("stale-approval-rotation", replay.stderr)
        self.assertEqual(json.loads(self.approval.read_text())["sha256"],
                         replacement)

    def test_revocation_persists_through_exact_binary_reinstallation(self):
        old = self.document["sha256"]
        self.assertEqual(revoke(self.root, "rectools"), self.approval)
        revoked = self.approval.read_bytes()
        original = self.handler.read_bytes()
        self.handler.unlink()
        self.handler.write_bytes(original)
        self.handler.chmod(0o700)
        self.assertEqual(hashlib.sha256(self.handler.read_bytes()).hexdigest(), old)
        self.assertEqual(examine(self.root, "rectools")["reason"], "approval-revoked")
        with self.assertRaises(IssueError):
            issue(self.root, "rectools", "0.1.0", old, "manual")
        self.assertEqual(self.approval.read_bytes(), revoked)

    def test_rotation_requires_secure_manifest_and_root_approval(self):
        old = self.document["sha256"]
        self.handler.write_bytes(b"second reviewed replacement\n")
        replacement = hashlib.sha256(self.handler.read_bytes()).hexdigest()
        manifest = self.root / "usr/share/vdr-suite/addons/rectools/addon.json"
        manifest.unlink()
        with self.assertRaisesRegex(IssueError, "invalid-installed-metadata"):
            rotate(self.root, "rectools", "0.1.0", old,
                   replacement, "manual")
        stage_root = self.root / "usr/share/vdr-suite/addons/rectools"
        # Use only the exact disposable fixture: no live paths.
        from addon_contract import FILES
        for filename in FILES:
            source = ROOT / "modules/rectools" / filename
            target = stage_root / filename
            if not target.exists():
                target.write_bytes(source.read_bytes())
                target.chmod(0o644)
        self.approval.chmod(0o666)
        with self.assertRaisesRegex(IssueError, "unsafe-approval-record"):
            rotate(self.root, "rectools", "0.1.0", old,
                   replacement, "manual")
        self.approval.chmod(0o600)
        before = self.approval.read_bytes()
        self.handler.unlink()
        with self.assertRaises(IssueError):
            rotate(self.root, "rectools", "0.1.0", old,
                   replacement, "manual")
        self.assertEqual(self.approval.read_bytes(), before)

    def test_admin_issuer_rejects_unapproved_digest(self):
        self.approval.unlink()
        with self.assertRaises(IssueError):
            issue(self.root, "rectools", "0.1.0", "0" * 64, "manual")
        self.assertFalse(self.approval.exists())

    def test_admin_issuer_refuses_live_root(self):
        with self.assertRaisesRegex(IssueError, "live-root-requires-explicit-confirmation"):
            issue(Path("/"), "rectools", "0.1.0",
                  self.document["sha256"], "manual")

    def test_revocation_survives_missing_handler(self):
        self.handler.unlink()
        self.assertEqual(revoke(self.root, "rectools"), self.approval)
        self.assertEqual(stat.S_IMODE(self.approval.stat().st_mode), 0o644)
        self.assertEqual(examine(self.root, "rectools")["reason"], "approval-revoked")
        before = self.approval.read_bytes()
        self.assertEqual(revoke(self.root, "rectools"), self.approval)
        self.assertEqual(self.approval.read_bytes(), before)
        with self.assertRaises(IssueError):
            issue(self.root, "rectools", "0.1.0",
                  self.document["sha256"], "manual")
        self.assertEqual(self.approval.read_bytes(), before)

    def test_revocation_refuses_live_root(self):
        with self.assertRaisesRegex(IssueError, "live-root-requires-explicit-confirmation"):
            revoke(Path("/"), "rectools")

    def test_invalid_approval_cannot_be_revoked_as_another_identity(self):
        self.document["id"] = "image"
        self.write()
        with self.assertRaises(IssueError):
            revoke(self.root, "rectools")
        self.assertEqual(json.loads(self.approval.read_text())["id"], "image")

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

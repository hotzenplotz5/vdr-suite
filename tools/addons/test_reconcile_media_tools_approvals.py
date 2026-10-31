#!/usr/bin/env python3
"""Isolated tests for a handler-only root+backend approval update."""
import json
from pathlib import Path
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch

import reconcile_media_tools_approvals as sync
from admin_approval_issuer import IssueError

OLD = "a" * 64
NEW = "b" * 64


class ReconcileApprovalsTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="mt-approval-sync-test-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.global_path, self.backend_path = sync.approval_paths(self.root, "default")
        self.global_path.parent.mkdir(parents=True)
        self.write(self.global_path, OLD)
        self.write(self.backend_path, OLD)
        actual = Path.lstat

        def root_owned(path):
            st = actual(path)
            return SimpleNamespace(st_mode=st.st_mode, st_uid=0,
                                   st_nlink=st.st_nlink, st_size=st.st_size)
        self.owner_patch = patch.object(Path, "lstat", root_owned)
        self.owner_patch.start()
        self.addCleanup(self.owner_patch.stop)

    def write(self, path, digest, state="approved"):
        path.write_text(json.dumps({
            "schemaVersion": 1, "id": "rectools",
            "package": "vdr-suite-addon-media-tools",
            "version": "0.1.0", "sha256": digest,
            "origin": "manual", "state": state,
        }), encoding="utf-8")
        path.chmod(0o644)

    def test_already_consistent_does_nothing(self):
        self.write(self.global_path, NEW)
        self.write(self.backend_path, NEW)
        self.assertEqual([], sync.plan(self.root, "default", NEW))
        with patch.object(sync, "examine_handler",
                          return_value={"consistent": True}):
            with patch.object(sync, "rotate") as rotate:
                self.assertEqual("MT_APPROVALS=ALREADY_CONSISTENT",
                                 sync.reconcile(self.root, "default", NEW,
                                                backup_parent=self.root))
                rotate.assert_not_called()

    def test_mixed_new_global_old_backend_is_repaired(self):
        self.write(self.global_path, NEW)
        self.assertEqual([(self.backend_path, OLD)],
                         sync.plan(self.root, "default", NEW))
        self.reconcile_with_fake_issuer()
        self.assertEqual([], sync.plan(self.root, "default", NEW))

    def test_both_old_pins_are_rotated(self):
        self.assertEqual(2, len(sync.plan(self.root, "default", NEW)))
        self.reconcile_with_fake_issuer()
        self.assertEqual([], sync.plan(self.root, "default", NEW))

    def test_revoked_missing_and_invalid_records_are_not_enabled(self):
        self.write(self.backend_path, OLD, state="revoked")
        with self.assertRaises(sync.ReconcileError):
            sync.plan(self.root, "default", NEW)
        self.backend_path.unlink()
        with self.assertRaises(sync.ReconcileError):
            sync.plan(self.root, "default", NEW)
        self.write(self.backend_path, OLD)
        self.backend_path.write_text('{"state":"approved"}', encoding="utf-8")
        with self.assertRaises(sync.ReconcileError):
            sync.plan(self.root, "default", NEW)

    def test_bad_handler_digest_never_rotates(self):
        with patch.object(sync, "examine_handler",
                          return_value={"consistent": False,
                                        "reason": "handler-digest-mismatch"}):
            with patch.object(sync, "rotate") as issuer:
                with self.assertRaises(sync.ReconcileError):
                    sync.reconcile(self.root, "default", NEW,
                                   backup_parent=self.root)
                issuer.assert_not_called()

    def test_failed_global_rotation_compensates_backend(self):
        def fake_rotate(root, module, version, expected, approved, origin,
                        **kwargs):
            path = self.backend_path if kwargs.get("backend_id") else self.global_path
            if path == self.global_path:
                raise IssueError("simulated-failed-global-rotation")
            self.assertEqual(expected, json.loads(path.read_text())["sha256"])
            self.write(path, approved)

        with patch.object(sync, "examine_handler",
                          return_value={"consistent": True}):
            with patch.object(sync, "rotate", side_effect=fake_rotate):
                with self.assertRaises(sync.ReconcileError):
                    sync.reconcile(self.root, "default", NEW,
                                   backup_parent=self.root)
        self.assertEqual(OLD, json.loads(self.global_path.read_text())["sha256"])
        self.assertEqual(OLD, json.loads(self.backend_path.read_text())["sha256"])

    def reconcile_with_fake_issuer(self):
        def fake_rotate(root, module, version, expected, approved, origin,
                        **kwargs):
            path = self.backend_path if kwargs.get("backend_id") else self.global_path
            self.assertEqual(expected, json.loads(path.read_text())["sha256"])
            self.write(path, approved)

        with patch.object(sync, "examine_handler",
                          return_value={"consistent": True}):
            with patch.object(sync, "rotate", side_effect=fake_rotate):
                result = sync.reconcile(self.root, "default", NEW,
                                        backup_parent=self.root)
        self.assertIn("MT_APPROVALS=CONSISTENT BACKUP=", result)


if __name__ == "__main__":
    unittest.main()

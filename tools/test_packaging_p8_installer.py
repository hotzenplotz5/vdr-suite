#!/usr/bin/env python3
"""P8 gate regression: fixture-only; do not run against live host."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).with_name("packaging_p8_installer.py")


class P8InstallerGateTests(unittest.TestCase):
    def run_gate(self, root, action):
        return subprocess.run(
            [sys.executable, str(SCRIPT), action, "--root", str(root)],
            text=True, capture_output=True, check=False, timeout=15)

    @staticmethod
    def put(root, rel, text="operator sentinel"):
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return path

    def test_clean_fixture_does_not_authorize_install(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            checked = self.run_gate(root, "--check")
            self.assertEqual(checked.returncode, 3, checked.stderr)
            observation = json.loads(checked.stdout)
            self.assertEqual(observation["verdict"], "NO_KNOWN_CONFLICT")
            self.assertFalse(observation["install_approved"])

            install = self.run_gate(root, "--install")
            self.assertEqual(install.returncode, 3, install.stderr)
            decision = json.loads(install.stdout)
            self.assertEqual(decision["decision"], "BLOCKED")
            self.assertEqual(decision["host_preflight_verdict"], "NO_KNOWN_CONFLICT")
            self.assertFalse(decision["install_approved"])
            self.assertTrue(decision["read_only"])
            self.assertEqual(list(root.rglob("*")), [])

    def test_unowned_nginx_file_blocks_before_any_install(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            snippet = self.put(root, "etc/nginx/snippets/vdr-suite.conf")
            before = snippet.read_bytes()
            checked = self.run_gate(root, "--check")
            self.assertEqual(checked.returncode, 2)
            self.assertEqual(json.loads(checked.stdout)["verdict"], "BLOCKED")

            install = self.run_gate(root, "--install")
            self.assertEqual(install.returncode, 3)
            report = json.loads(install.stdout)
            self.assertEqual(report["host_preflight_verdict"], "BLOCKED")
            self.assertIn(
                {"code": "UNOWNED_PACKAGE_TARGET",
                 "path": "/etc/nginx/snippets/vdr-suite.conf",
                 "severity": "BLOCK"},
                report["host_preflight_checks"])
            self.assertEqual(snippet.read_bytes(), before)

    def test_manual_daemon_review_still_blocks_install(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            old = self.put(root, "usr/local/sbin/vdr-suite-daemon")
            checked = self.run_gate(root, "--check")
            self.assertEqual(checked.returncode, 3, checked.stderr)
            self.assertEqual(json.loads(checked.stdout)["verdict"],
                             "REVIEW_REQUIRED")
            install = self.run_gate(root, "--install")
            self.assertEqual(install.returncode, 3)
            self.assertEqual(json.loads(install.stdout)["host_preflight_verdict"],
                             "REVIEW_REQUIRED")
            self.assertEqual(old.read_text(), "operator sentinel")

    def test_observed_yavdr_collision_shape_blocks_without_replacing_files(self):
        # Reconstruct public paths only, never inspect the user's host.
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            collisions = [
                "etc/nginx/snippets/vdr-suite.conf",
                "usr/sbin/vdr-suite-daemon",
                "etc/default/vdr-suite-daemon",
                "etc/vdr-suite/backend-agent.conf",
            ]
            originals = {rel: self.put(root, rel, "private sentinel")
                         for rel in collisions}
            legacy = self.put(root, "usr/local/sbin/vdr-suite-daemon")
            active = self.put(root, "etc/nginx/conf.d/vidaa-https.conf",
                              "include /etc/nginx/snippets/vdr-suite-browser-sessions.conf;")
            self.put(root, "etc/nginx/snippets/vdr-suite-browser-sessions.conf",
                     "session placeholder")
            check = self.run_gate(root, "--check")
            self.assertEqual(check.returncode, 2, check.stderr)
            install = self.run_gate(root, "--install")
            self.assertEqual(install.returncode, 3, install.stderr)
            decision = json.loads(install.stdout)
            actual = {(record["path"], record["code"])
                      for record in decision["host_preflight_checks"]}
            for rel in collisions:
                self.assertIn(("/" + rel, "UNOWNED_PACKAGE_TARGET"), actual)
                self.assertEqual(originals[rel].read_text(), "private sentinel")
            self.assertIn(("/usr/local/sbin/vdr-suite-daemon",
                           "EXISTING_MANUAL_OR_STATE_PATH"), actual)
            self.assertIn(("/etc/nginx/conf.d/vidaa-https.conf",
                           "NGINX_EXISTING_VDR_SUITE_ROUTE"), actual)
            self.assertEqual(legacy.read_text(), "operator sentinel")
            self.assertTrue(active.is_file())
            self.assertNotIn("private sentinel", install.stdout)

    def test_unknown_root_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            missing = Path(tmp) / "missing"
            for action in ("--check", "--install"):
                with self.subTest(action=action):
                    result = self.run_gate(missing, action)
                    self.assertEqual(result.returncode, 2)
                    self.assertIn("P8_HOST_PREFLIGHT=BLOCKED", result.stderr)
                    self.assertEqual(result.stdout, "")
                    self.assertFalse(missing.exists())

    def test_build_and_uninstall_are_not_parallel_implementations(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for action in ("--build", "--uninstall"):
                with self.subTest(action=action):
                    result = self.run_gate(root, action)
                    self.assertEqual(result.returncode, 3)
                    self.assertIn("P8_ACTION=BLOCKED", result.stderr)
            self.assertEqual(list(root.rglob("*")), [])


if __name__ == "__main__":
    unittest.main()

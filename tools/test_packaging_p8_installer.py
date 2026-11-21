#!/usr/bin/env python3
"""P8 gate regression: fixture-only; do not run against live host."""
import json
import os
from pathlib import Path
import shutil
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

    def test_i44_observed_four_component_manual_host_never_auto_adopted(self):
        """Reproduce only the operator-reported path shape in a private root.

        The existing P8 installer has no authorized manual-to-package
        transition. Even with a matching optional Media Tools inventory,
        all manual files, approval and SQLite/WAL sentinels remain unchanged.
        """
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            paths = (
                "usr/sbin/vdr-suite-daemon",
                "usr/sbin/vdr-suite-backend-agent",
                "usr/lib/vdr/plugins/libvdr-suitebridge.so.14",
                "usr/lib/systemd/system/vdr-suite-daemon.service",
                "usr/lib/systemd/system/vdr-suite-backend-agent.service",
                "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import",
                "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-check",
                "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-shrink",
                "usr/libexec/vdr-suite/addons/rectools/addon.json",
                "usr/share/vdr-suite/addons/rectools/addon.json",
                "usr/share/vdr-suite/addons/rectools/media-tools-install-inventory.json",
                "etc/vdr-suite/backend-agent.conf",
                "etc/vdr-suite/addon-approvals/rectools.json",
                "etc/vdr-suite/addon-approvals/rectools.import-running.lock",
                "etc/vdr-suite/addon-sources/media-import.tsv",
                "etc/systemd/system/vdr-suite-daemon.service.d/recording-artwork.conf",
                "etc/systemd/system/vdr-suite-backend-agent.service.d/20-media-tools.conf",
                "etc/nginx/snippets/vdr-suite.conf",
                "var/lib/vdr-suite/vdr-suite.db",
                "var/lib/vdr-suite/vdr-suite.db-wal",
            )
            files = {
                rel: self.put(root, rel,
                              "" if rel.endswith(".lock") else "private-host-fixture")
                for rel in paths
            }
            before = {rel: file.read_bytes() for rel, file in files.items()}
            scan = self.run_gate(root, "--check")
            self.assertEqual(scan.returncode, 2, scan.stderr)
            observation = json.loads(scan.stdout)
            self.assertEqual(observation["verdict"], "BLOCKED")
            self.assertFalse(observation["install_approved"])
            findings = {(entry["path"], entry["code"])
                        for entry in observation["checks"]}
            for rel in (
                "usr/sbin/vdr-suite-daemon",
                "usr/sbin/vdr-suite-backend-agent",
                "usr/lib/systemd/system/vdr-suite-daemon.service",
                "usr/lib/systemd/system/vdr-suite-backend-agent.service",
                "etc/vdr-suite/backend-agent.conf",
                "etc/nginx/snippets/vdr-suite.conf",
            ):
                self.assertIn(("/" + rel, "UNOWNED_PACKAGE_TARGET"), findings)
            self.assertIn(
                ("/usr/lib/vdr/plugins/libvdr-suitebridge.so.14",
                 "INSTALLED_VDR_PLUGIN_ABI_REVIEW"), findings)

            denied = self.run_gate(root, "--install")
            self.assertEqual(denied.returncode, 3, denied.stderr)
            decision = json.loads(denied.stdout)
            self.assertEqual(decision["decision"], "BLOCKED")
            self.assertFalse(decision["install_approved"])
            self.assertTrue(decision["read_only"])
            self.assertNotIn("private-host-fixture", denied.stdout)
            for rel, file in files.items():
                self.assertEqual(file.read_bytes(), before[rel], rel)
            print("MT_I44_FOUR_COMPONENT_MANUAL_HOST_REFUSAL=PASS")

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

    def test_check_does_not_create_bytecode_in_source_checkout(self):
        # Run from a small copied source checkout, without -B or env overrides.
        with tempfile.TemporaryDirectory() as tmp:
            base = Path(tmp)
            repo = base / "repo"
            tools = repo / "tools"
            tools.mkdir(parents=True)
            for filename in ("packaging_p8_installer.py",
                             "packaging_host_collision_preflight.py"):
                shutil.copy2(SCRIPT.with_name(filename), tools / filename)
            source_debian = SCRIPT.parent.parent / "debian"
            debian = repo / "debian"
            debian.mkdir()
            for filename in ("vdr-suite-daemon.install",
                             "vdr-suite-backend-agent.install",
                             "vdr-plugin-suitebridge.install"):
                shutil.copy2(source_debian / filename, debian / filename)
            target = base / "host"
            target.mkdir()
            env = dict(os.environ)
            env.pop("PYTHONDONTWRITEBYTECODE", None)
            run = subprocess.run(
                [sys.executable, str(tools / "packaging_p8_installer.py"),
                 "--check", "--root", str(target)],
                env=env, text=True, capture_output=True, timeout=15)
            self.assertEqual(run.returncode, 3, run.stderr)
            self.assertFalse((tools / "__pycache__").exists())
            self.assertEqual(list(target.rglob("*")), [])

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

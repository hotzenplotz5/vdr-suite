#!/usr/bin/env python3
"""Regression tests for pre-unpack .deb collision gates; no live host access."""
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).with_name("packaging_p8_preinst.py")
spec = importlib.util.spec_from_file_location("p8_preinst_gen", SCRIPT)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class PreinstTests(unittest.TestCase):
    def fixture(self, package="vdr-suite-daemon"):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        root = Path(tmp.name)
        host = root / "host"
        host.mkdir()
        bindir = root / "bin"
        bindir.mkdir()
        mock = bindir / "dpkg-query"
        mock.write_text("""#!/bin/sh
case "$1" in
  -S)
    if [ "${MOCK_OWNER:-}" = '' ]; then exit 1; fi
    printf '%s: %s\\n' "${MOCK_OWNER}" "$3"
    ;;
  -W)
    if [ "${MOCK_INSTALLED:-0}" = 1 ]; then
      printf 'install ok installed'
      exit 0
    fi
    exit 1
    ;;
  *) exit 2 ;;
esac
""", encoding="utf-8")
        mock.chmod(0o755)
        script = mod.render(package, test_root=str(host))
        self.assertEqual(subprocess.run(["/bin/sh", "-n"], input=script,
                                        text=True, capture_output=True).returncode, 0)
        return host, bindir, script

    @staticmethod
    def put(root, rel, body="operator sentinel"):
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(body, encoding="utf-8")
        return path

    @staticmethod
    def run_gate(bindir, script, package="vdr-suite-daemon", owner="", installed=False,
                 action="install"):
        env = dict(os.environ, PATH=str(bindir) + os.pathsep + os.environ["PATH"],
                   MOCK_OWNER=owner, MOCK_INSTALLED="1" if installed else "0")
        return subprocess.run(["/bin/sh", "-s", "--", action], input=script,
                              text=True, capture_output=True, timeout=8, env=env)

    def test_manifest_generation_and_debhelper_stage_modes(self):
        self.assertEqual(set(mod.MANIFESTS), {
            "vdr-suite-daemon", "vdr-suite-backend-agent", "vdr-plugin-suitebridge"})
        with tempfile.TemporaryDirectory() as tmp:
            stage = Path(tmp)
            pkg = stage / "vdr-plugin-suitebridge" / "DEBIAN"
            pkg.mkdir(parents=True)
            preinst = pkg / "preinst"
            preinst.write_text("#!/bin/sh\nset -e\n# existing debhelper hook\n", encoding="utf-8")
            result = mod.stage_preinst(stage)
            self.assertEqual(result, ["vdr-plugin-suitebridge"])
            body = preinst.read_text(encoding="utf-8")
            self.assertIn("# BEGIN P8 PREUNPACK", body)
            self.assertIn("p8_preunpack_guard", body)
            self.assertIn("# existing debhelper hook", body)
            self.assertEqual(body.count("# existing debhelper hook"), 1)
            self.assertEqual(preinst.stat().st_mode & 0o777, 0o755)
            self.assertEqual(subprocess.run(
                ["/bin/sh", "-n", str(preinst)],
                capture_output=True).returncode, 0)
            with self.assertRaisesRegex(ValueError, "already guarded"):
                mod.stage_preinst(stage)

    def test_unmodified_debhelper_hook_still_runs_on_abort(self):
        with tempfile.TemporaryDirectory() as tmp:
            stage = Path(tmp)
            pkg = stage / "vdr-suite-daemon" / "DEBIAN"
            pkg.mkdir(parents=True)
            marker = stage / "debhelper-ran"
            script = pkg / "preinst"
            script.write_text("#!/bin/sh\nset -e\nprintf hook > '" + str(marker) + "'\n")
            mod.stage_preinst(stage)
            result = subprocess.run(["/bin/sh", str(script), "abort-upgrade"],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(marker.read_text(), "hook")

    def test_nonexistent_or_symlink_stage_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            stage = Path(tmp)
            with self.assertRaisesRegex(ValueError, "no Debian package"):
                mod.stage_preinst(stage)
            (stage / "vdr-suite-daemon").symlink_to(stage)
            with self.assertRaisesRegex(ValueError, "unsafe Debian"):
                mod.stage_preinst(stage)

    def test_no_collision_is_not_a_release_authorization(self):
        host, bindir, script = self.fixture()
        result = self.run_gate(bindir, script)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(list(host.rglob("*")), [])

    def test_unowned_existing_snippet_blocks_and_preserves_bytes(self):
        host, bindir, script = self.fixture()
        config = self.put(host, "etc/nginx/snippets/vdr-suite.conf")
        before = config.read_bytes()
        result = self.run_gate(bindir, script)
        self.assertEqual(result.returncode, 1)
        self.assertIn("P8_PREUNPACK=BLOCKED", result.stderr)
        self.assertIn("OWNERSHIP_UNKNOWN", result.stderr)
        self.assertEqual(config.read_bytes(), before)

    def test_foreign_owned_file_blocks(self):
        host, bindir, script = self.fixture()
        file = self.put(host, "usr/sbin/vdr-suite-daemon")
        result = self.run_gate(bindir, script, owner="unrelated")
        self.assertEqual(result.returncode, 1)
        self.assertIn("FOREIGN_OR_AMBIGUOUS_OWNER", result.stderr)
        self.assertEqual(file.read_text(), "operator sentinel")

    def test_same_package_owned_file_is_allowed_for_preunpack_only(self):
        host, bindir, script = self.fixture()
        file = self.put(host, "usr/sbin/vdr-suite-daemon")
        result = self.run_gate(bindir, script, owner="vdr-suite-daemon", installed=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(file.read_text(), "operator sentinel")

    def test_unowned_web_tree_is_detected(self):
        host, bindir, script = self.fixture()
        file = self.put(host, "usr/share/vdr-suite/web/frontend/index.html")
        result = self.run_gate(bindir, script)
        self.assertEqual(result.returncode, 1)
        self.assertIn("/usr/share/vdr-suite", result.stderr)
        self.assertEqual(file.read_text(), "operator sentinel")

    def test_legacy_manual_and_state_block(self):
        for path in ("usr/local/sbin/vdr-suite-daemon", "var/lib/vdr-suite/user.db",
                     "etc/systemd/system/vdr-suite-daemon.service"):
            with self.subTest(path=path):
                host, bindir, script = self.fixture()
                file = self.put(host, path)
                result = self.run_gate(bindir, script)
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertEqual(file.read_text(), "operator sentinel")

    def test_pkg_upgrade_ignores_state_but_not_manual_binary(self):
        host, bindir, script = self.fixture()
        file = self.put(host, "var/lib/vdr-suite/user.db")
        result = self.run_gate(bindir, script, installed=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(file.read_text(), "operator sentinel")

    def test_symlink_target_blocks_even_when_owned(self):
        host, bindir, script = self.fixture()
        target = self.put(host, "other.conf")
        link = host / "etc/nginx/snippets/vdr-suite.conf"
        link.parent.mkdir(parents=True, exist_ok=True)
        link.symlink_to(target)
        result = self.run_gate(bindir, script, owner="vdr-suite-daemon")
        self.assertEqual(result.returncode, 1)
        self.assertIn("SYMLINK_TARGET", result.stderr)
        self.assertTrue(link.is_symlink())

    def test_suitebridge_collision_uses_abi_glob(self):
        host, bindir, script = self.fixture("vdr-plugin-suitebridge")
        file = self.put(host, "usr/lib/vdr/plugins/libvdr-suitebridge.so.2.8.3")
        result = self.run_gate(bindir, script)
        self.assertEqual(result.returncode, 1)
        self.assertIn("OWNERSHIP_UNKNOWN", result.stderr)
        self.assertEqual(file.read_text(), "operator sentinel")

    def test_backend_agent_config_collision(self):
        host, bindir, script = self.fixture("vdr-suite-backend-agent")
        file = self.put(host, "etc/vdr-suite/backend-agent.conf")
        result = self.run_gate(bindir, script)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(file.read_text(), "operator sentinel")

    def test_abort_actions_do_not_prevent_dpkg_recovery(self):
        host, bindir, script = self.fixture()
        self.put(host, "etc/nginx/snippets/vdr-suite.conf")
        for action in ("abort-install", "abort-upgrade", "abort-deconfigure"):
            self.assertEqual(self.run_gate(bindir, script, action=action).returncode, 0)

    def test_manifest_unknown_globs_are_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            deb = root / "debian"
            deb.mkdir()
            (deb / "vdr-plugin-suitebridge.install").write_text(
                "usr/lib/vdr/plugins/libvdr-suitebridge.so.[0-9]*\n")
            with self.assertRaisesRegex(ValueError, "unsupported package manifest"):
                mod.manifest_entries(root, "vdr-plugin-suitebridge")


if __name__ == "__main__":
    unittest.main()

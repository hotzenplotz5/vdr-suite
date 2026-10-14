#!/usr/bin/env python3
"""No-root contracts for independent SuiteBridge packaging and build-artifact ignores."""
import shutil
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

ROOT = Path(__file__).resolve().parents[1]


class PackagingProfileTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("make"), "GNU make unavailable")
    def test_plugin_only_build_and_install_do_not_build_server(self):
        for target in ("override_dh_auto_build", "override_dh_auto_install"):
            with self.subTest(target=target):
                run = subprocess.run(
                    ["make", "-n", "-f", "debian/rules", target,
                     "DEB_BUILD_PROFILES=plugin-only", "MAKE=/bin/echo"],
                    cwd=ROOT, text=True, capture_output=True, check=True)
                self.assertIn("vdr-plugin-suite-bridge", run.stdout)
                self.assertNotIn("build-installable", run.stdout)
                self.assertNotIn("make install DESTDIR=", run.stdout)
                self.assertNotIn("/bin/echo install DESTDIR=", run.stdout)

    @unittest.skipUnless(shutil.which("make"), "GNU make unavailable")
    def test_default_build_includes_daemon_and_plugin(self):
        run = subprocess.run(
            ["make", "-n", "-f", "debian/rules", "override_dh_auto_build",
             "DEB_BUILD_PROFILES=", "MAKE=/bin/echo"],
            cwd=ROOT, text=True, capture_output=True, check=True)
        self.assertIn("build-installable", run.stdout)
        self.assertIn("vdr-plugin-suite-bridge", run.stdout)

    def test_control_declares_profile_and_abi_dependency(self):
        data = (ROOT / "debian/control").read_text(encoding="utf-8")
        self.assertEqual(data.count("Build-Profiles: <!plugin-only>"), 2)
        self.assertIn("libsqlite3-dev <!plugin-only>", data)
        self.assertIn("${vdr:Depends}", data)

    @unittest.skipUnless(shutil.which("git"), "Git unavailable")
    def test_build_artifacts_ignored_but_metadata_tracked(self):
        with TemporaryDirectory(prefix="packaging-ignore-") as temp:
            root = Path(temp)
            (root / "vdr-plugin-suite-bridge").mkdir()
            (root / ".gitignore").write_bytes((ROOT / ".gitignore").read_bytes())
            (root / "vdr-plugin-suite-bridge/.gitignore").write_bytes(
                (ROOT / "vdr-plugin-suite-bridge/.gitignore").read_bytes())
            subprocess.run(["git", "init", "-q", str(root)], check=True)
            excluded = (
                "debian/tmp/usr/sbin/vdr-suite-daemon",
                "debian/vdr-plugin-suitebridge/DEBIAN/control",
                "debian/.debhelper/build-stamp",
                "vdr-plugin-suite-bridge/libvdr-suitebridge.so.2.6.0",
                "vdr-suite_0.14.1-1_amd64.deb",
                "vdr-suite_0.14.1-1_source.changes",
                ".build/vdr-suite-daemon",
            )
            included = (
                "debian/rules",
                "debian/control",
                "debian/vdr-plugin-suitebridge.install",
                "debian/changelog",
                "vdr-plugin-suite-bridge/Makefile",
                "vdr-plugin-suite-bridge/suitebridge.cpp",
                "VERSION",
            )
            for name, expected in [(p, True) for p in excluded] + [(p, False) for p in included]:
                with self.subTest(path=name):
                    run = subprocess.run(
                        ["git", "-C", str(root), "check-ignore", "--no-index", "-q", "--", name],
                        capture_output=True, text=True)
                    self.assertIn(run.returncode, (0, 1), run.stderr)
                    self.assertEqual(run.returncode == 0, expected)


if __name__ == "__main__":
    unittest.main()

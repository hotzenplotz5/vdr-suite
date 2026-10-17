#!/usr/bin/env python3
"""Static, no-root contracts for the controlled standalone package builder."""
from pathlib import Path
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools/build_suitebridge_deb_isolated.sh"


class IsolatedBuildTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("bash"), "bash is not present")
    def test_bash_syntax(self):
        subprocess.run(["bash", "-n", str(SCRIPT)], check=True, cwd=ROOT)

    def test_fail_closed_provenance_and_resource_gates(self):
        source = SCRIPT.read_text(encoding="utf-8")
        for required in (
            "work/packaging-debian-ubuntu-install",
            '/home/yavdr/vdr-suite-packaging-test',
            'git -C "$root" rev-parse HEAD',
            'git -C "$root" status --porcelain',
            "git -C \"$root\" ls-remote",
            "dpkg-checkbuilddeps -B -Pplugin-only",
            "RESOURCE_PREFLIGHT=PASS",
            "RESOURCE_POSTCHECK=PASS",
            "dpkg-buildpackage -B -Pplugin-only -us -uc",
            "git clone --no-local",
            "vdr-abi-",
        ):
            with self.subTest(required=required):
                self.assertIn(required, source)
        self.assertNotIn("sudo ", source)
        self.assertNotIn("apt-get ", source)
        self.assertNotIn("systemctl ", source)
        self.assertNotIn("rm -rf ", source)

    def test_build_runs_in_clone_after_preflight(self):
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertLess(source.index("RESOURCE_PREFLIGHT=PASS"), source.index('work="$(mktemp'))
        self.assertLess(source.index('git clone --no-local'), source.index("dpkg-buildpackage -B -Pplugin-only"))


if __name__ == "__main__":
    unittest.main()

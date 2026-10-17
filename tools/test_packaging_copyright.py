#!/usr/bin/env python3
"""Source-only guards for the provisional Debian copyright release boundary."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

class CopyrightMetadataTests(unittest.TestCase):
    def test_dep5_license_scopes_and_release_gate(self):
        value = (ROOT / "debian/copyright").read_text(encoding="utf-8")
        self.assertTrue(value.startswith("Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/"))
        for required in (
            "Files: *",
            "Files: vdr-plugin-suite-bridge/services.h",
            "License: GPL-2.0-or-later",
            "License: GPL-3.0-or-later",
            "Files: web/frontend/assets/recording-genre-*.svg",
            "License: License-grant-pending",
            "not a permission to distribute",
        ):
            with self.subTest(required=required):
                self.assertIn(required, value)
        self.assertNotIn("License-grant-pending", value)
        self.assertIn("Files: web/frontend/logo-vdr-suite.svg", value)

    def test_three_binary_packages_and_debhelper_copyright_sequence(self):
        control = (ROOT / "debian/control").read_text(encoding="utf-8")
        for pkg in ("vdr-suite-daemon", "vdr-suite-backend-agent",
                    "vdr-plugin-suitebridge"):
            self.assertIn("Package: " + pkg + "\n", control)
        rules = (ROOT / "debian/rules")
        self.assertTrue(rules.stat().st_mode & 0o111)
        self.assertIn("dh $@ --with vdrplugin", rules.read_text(encoding="utf-8"))
        # This only guards source packaging; actual .deb ownership must be checked in CI.

if __name__ == "__main__":
    unittest.main()

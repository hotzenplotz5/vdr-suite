"""Offline Debian add-on builder tests; builds metadata only, never installs."""
import importlib.util
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from test_addon_contract import addon

script = Path(__file__).with_name("build_deb.py")
spec = importlib.util.spec_from_file_location("build_deb", script)
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


class DebianAddonTests(unittest.TestCase):
    def test_invalid_maintainer_fails_closed(self):
        for value in ("", "unknown", "Unknown <x>", "Name <bad@example.org>\nInjected: 1"):
            with self.subTest(value=value):
                with self.assertRaises(addon.ContractError):
                    builder.validate_maintainer(value)

    @unittest.skipUnless(shutil.which("dpkg-deb"), "dpkg-deb unavailable")
    def test_build_inert_deb_without_install(self):
        with tempfile.TemporaryDirectory(prefix="addon-deb-test-") as directory:
            package = builder.build(addon.ROOT, "rectools", directory, "Test Builder <builder@example.org>")
            self.assertEqual(package.parent, Path(directory))
            self.assertEqual(package.name, "vdr-suite-addon-media-tools_0.1.0~scaffold1_all.deb")
            fields = subprocess.check_output(["dpkg-deb", "--field", str(package)], text=True)
            self.assertIn("Architecture: all", fields)
            self.assertIn("Inactive VDR-Suite", fields)
            names = subprocess.check_output(["dpkg-deb", "--contents", str(package)], text=True)
            self.assertIn("usr/share/vdr-suite/addons/rectools/addon.json", names)
            self.assertIn("usr/share/vdr-suite/addons/rectools/AGENTS.md", names)
            self.assertNotIn("/etc/", names)
            self.assertNotIn("/lib/systemd/", names)
            with self.assertRaises(addon.ContractError):
                builder.build(addon.ROOT, "rectools", directory, "Test Builder <builder@example.org>")

"""Package-layout regressions for source-compiled optional C++ Media Tools."""
import importlib.util
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from test_addon_contract import addon
from build_deb import ContractError

path = Path(__file__).with_name("build_media_tools_cpp_deb.py")
spec = importlib.util.spec_from_file_location("build_media_tools_cpp_deb", path)
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


class CppPackageTests(unittest.TestCase):
    def test_invalid_inputs(self):
        with tempfile.TemporaryDirectory(prefix="cpp-addon-test-") as folder:
            with self.assertRaises(ContractError):
                builder.build(addon.ROOT, "/tmp/does-not-exist/vdr-suite-media-import",
                              folder, "Tester <tester@example.org>")

    @unittest.skipUnless(shutil.which("dpkg-deb") and shutil.which("dpkg"),
                         "dpkg-deb unavailable")
    def test_stages_inert_cpp_binary(self):
        with tempfile.TemporaryDirectory(prefix="cpp-addon-test-") as folder:
            binary = Path(folder) / "vdr-suite-media-import"
            # Controlled layout test fixture, not a runnable importer.
            shutil.copyfile("/usr/bin/true", binary)
            binary.chmod(0o755)
            built = builder.build(addon.ROOT, str(binary), folder,
                                  "Test Builder <builder@example.org>")
            self.assertIn("~cpppreview1_", built.name)
            control = subprocess.check_output(["dpkg-deb", "--field", str(built)], text=True)
            self.assertIn("Depends: ffmpeg, vdr", control)
            listing = subprocess.check_output(["dpkg-deb", "--contents", str(built)], text=True)
            self.assertIn("usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import", listing)
            self.assertIn("usr/share/vdr-suite/addons/rectools/AGENTS.md", listing)
            self.assertNotIn("/etc/", listing)
            self.assertNotIn("/lib/systemd/", listing)
            with self.assertRaises(ContractError):
                builder.build(addon.ROOT, str(binary), folder,
                              "Test Builder <builder@example.org>")

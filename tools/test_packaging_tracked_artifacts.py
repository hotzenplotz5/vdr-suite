import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location(
    "tracked", Path(__file__).with_name("check_packaging_tracked_artifacts.py"))
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

class TrackedArtifactsTests(unittest.TestCase):
    def test_generated_outputs(self):
        for path in ("debian/tmp/usr/sbin/vdr-suite-daemon",
                     "debian/.debhelper/build", "debian/vdr-suite-daemon/DEBIAN/control",
                     "vdr-plugin-suite-bridge/libvdr-suitebridge.so.2.6.0",
                     ".build/vdr-suite-daemon", "vdr-suite_0.14.1-1_amd64.deb"):
            with self.subTest(path=path):
                self.assertTrue(mod.is_generated(path))

    def test_packaging_sources_stay_tracked(self):
        for path in ("debian/rules", "debian/control", "debian/changelog",
                     "debian/vdr-plugin-suitebridge.install",
                     "vdr-plugin-suite-bridge/suitebridge.cpp", "VERSION"):
            with self.subTest(path=path):
                self.assertFalse(mod.is_generated(path))

if __name__ == "__main__":
    unittest.main()

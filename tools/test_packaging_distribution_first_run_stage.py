#!/usr/bin/env python3
"""No-root regression for shared first-run package configuration."""
from pathlib import Path
import shutil
import tempfile
import unittest

from distribution_first_run_stage import NotReady, stage
from mld66_stage_first_run import stage as mld_stage


ROOT = Path(__file__).resolve().parent.parent


class DistributionFirstRunStageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.package = Path(self.temp.name) / "staged-package"
        (self.package / "etc/vdr-suite").mkdir(parents=True)
        (self.package / "etc/default").mkdir(parents=True)
        self.agent = self.package / "etc/vdr-suite/backend-agent.conf"
        self.daemon = self.package / "etc/default/vdr-suite-daemon"
        shutil.copyfile(ROOT / "packaging/systemd/backend-agent.conf", self.agent)
        shutil.copyfile(ROOT / "packaging/systemd/vdr-suite-daemon.default", self.daemon)

    def test_ubuntu_and_yavdr_share_the_same_staging_contract(self):
        origin = "https://suite.example.org:8443"
        channels = "/var/lib/vdr/channels.conf"
        stage(self.package, origin, channels)
        first = self.agent.read_bytes()
        daemon = self.daemon.read_bytes()
        self.assertIn(b"CONTROL_PLANE_URL=https://suite.example.org:8443\n", first)
        self.assertIn(b"CHANNELS_CONF_PATH=/var/lib/vdr/channels.conf\n", first)
        self.assertIn(b"OBSERVATION_DOMAINS=backend-health,channels\n", first)
        self.assertIn(b"VDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=true\n", daemon)
        stage(self.package, origin, channels)
        self.assertEqual(first, self.agent.read_bytes())
        self.assertEqual(daemon, self.daemon.read_bytes())

    def test_mld_adapter_and_shared_staging_have_identical_effect(self):
        origin = "https://suite.mld.local:8443"
        channels = "/etc/vdr/channels.conf"
        mld_stage(self.package, origin, channels)
        config = self.agent.read_bytes()
        daemon = self.daemon.read_bytes()
        stage(self.package, origin, channels)
        self.assertEqual(config, self.agent.read_bytes())
        self.assertEqual(daemon, self.daemon.read_bytes())

    def test_existing_custom_configuration_is_never_replaced(self):
        self.agent.write_text(self.agent.read_text().replace(
            "https://control-plane.example.invalid",
            "https://already-configured.example.org"))
        original = self.agent.read_bytes()
        daemon = self.daemon.read_bytes()
        with self.assertRaises(NotReady):
            stage(self.package, "https://other.example.org", "/var/lib/vdr/channels.conf")
        self.assertEqual(original, self.agent.read_bytes())
        self.assertEqual(daemon, self.daemon.read_bytes())

    def test_rejects_unconfigured_or_live_staging(self):
        for origin in ("", "http://127.0.0.1:8080", "https://control-plane.example.invalid"):
            with self.subTest(origin=origin), self.assertRaises(NotReady):
                stage(self.package, origin, "/var/lib/vdr/channels.conf")
        with self.assertRaises(NotReady):
            stage("/", "https://suite.example.org", "/var/lib/vdr/channels.conf")


if __name__ == "__main__":
    unittest.main()

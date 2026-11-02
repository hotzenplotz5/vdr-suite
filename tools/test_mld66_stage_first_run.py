#!/usr/bin/env python3
"""No-root package staging contract for the MLD local first-run setup."""
import os
from pathlib import Path
import shutil
import tempfile
import unittest

from mld66_stage_first_run import NotReady, stage

ROOT = Path(__file__).resolve().parent.parent
ORIGIN = "https://suite.mld.local:8443"
CHANNELS = "/etc/vdr/channels.conf"


class MldFirstRunStagingTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name) / "pkg"
        (self.root / "etc/vdr-suite").mkdir(parents=True)
        (self.root / "etc/default").mkdir(parents=True)
        self.agent = self.root / "etc/vdr-suite/backend-agent.conf"
        self.daemon = self.root / "etc/default/vdr-suite-daemon"
        shutil.copyfile(ROOT / "packaging/systemd/backend-agent.conf", self.agent)
        shutil.copyfile(ROOT / "packaging/systemd/vdr-suite-daemon.default", self.daemon)

    def tearDown(self):
        self.temp.cleanup()

    def test_staging_changes_only_existing_two_package_files(self):
        before = self.agent.read_text()
        daemon_before = self.daemon.read_text()
        stage(str(self.root), ORIGIN, CHANNELS)
        config = self.agent.read_text()
        daemon = self.daemon.read_text()
        self.assertIn("CONTROL_PLANE_URL=" + ORIGIN + "\n", config)
        self.assertIn("ADAPTERS=channels-conf\n", config)
        self.assertIn("OBSERVATION_DOMAINS=backend-health,channels\n", config)
        self.assertIn("CHANNELS_CONF_PATH=" + CHANNELS + "\n", config)
        self.assertIn("VDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=true\n", daemon)
        self.assertIn("COMMAND_TYPES=vdr.timer.create", config)
        self.assertIn("VDR_SUITE_SUITE_BRIDGE_ENABLED=true", daemon)
        self.assertNotIn("CONTROL_PLANE_URL=" + ORIGIN, before)
        self.assertNotIn("VDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=true", daemon_before)
        self.assertEqual(sorted(x.relative_to(self.root).as_posix()
                                for x in self.root.rglob("*") if x.is_file()),
                         ["etc/default/vdr-suite-daemon",
                          "etc/vdr-suite/backend-agent.conf"])
        stage(str(self.root), ORIGIN, CHANNELS)
        self.assertEqual(self.agent.read_text(), config)
        self.assertEqual(self.daemon.read_text(), daemon)

    def test_protects_preexisting_origin_and_channel_source(self):
        original = self.agent.read_text()
        self.agent.write_text(original.replace(
            "https://control-plane.example.invalid",
            "https://trusted-existing.local"))
        with self.assertRaises(NotReady):
            stage(str(self.root), ORIGIN, CHANNELS)
        self.assertNotIn("VDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=true",
                         self.daemon.read_text())
        self.agent.write_text(original.replace(
            "CHANNELS_CONF_PATH=/var/lib/vdr/channels.conf",
            "CHANNELS_CONF_PATH=/etc/other/channels.conf"))
        with self.assertRaises(NotReady):
            stage(str(self.root), ORIGIN, CHANNELS)
        self.assertNotIn("VDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=true",
                         self.daemon.read_text())

    def test_refuses_http_untrusted_directory_and_symlink(self):
        with self.assertRaises(NotReady):
            stage(str(self.root), "http://127.0.0.1:18080", CHANNELS)
        with self.assertRaises(NotReady):
            stage("/", ORIGIN, CHANNELS)
        with self.assertRaises(NotReady):
            stage(str(self.root), ORIGIN, "/tmp/../etc/vdr/channels.conf")
        self.agent.unlink()
        self.agent.symlink_to(ROOT / "packaging/systemd/backend-agent.conf")
        with self.assertRaises(NotReady):
            stage(str(self.root), ORIGIN, CHANNELS)
        self.assertNotIn("VDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=true",
                         self.daemon.read_text())

    def test_stage_rejects_parent_directory_symlink_escape(self):
        with tempfile.TemporaryDirectory() as external:
            outside = Path(external)
            (outside / "vdr-suite").mkdir()
            copied = outside / "vdr-suite/backend-agent.conf"
            shutil.copyfile(ROOT / "packaging/systemd/backend-agent.conf", copied)
            self.agent.unlink()
            (self.root / "etc/vdr-suite").rmdir()
            (self.root / "etc/vdr-suite").symlink_to(outside / "vdr-suite")
            with self.assertRaises(NotReady):
                stage(str(self.root), ORIGIN, CHANNELS)
            self.assertIn("control-plane.example.invalid", copied.read_text())
            self.assertNotIn("VDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=true",
                             self.daemon.read_text())

    def test_staging_does_not_touch_credentials_or_external_vdr_config(self):
        private = self.root / "var/lib/vdr-suite/backend-agent/identity"
        private.parent.mkdir(parents=True)
        private.write_text("credential_secret=do-not-touch")
        stage(str(self.root), ORIGIN, CHANNELS)
        self.assertEqual(private.read_text(), "credential_secret=do-not-touch")
        self.assertFalse((self.root / "etc/vdr/channels.conf").exists())

    def test_package_can_supply_trusted_ca_without_changing_existing_state(self):
        ca = "/etc/ssl/certs/mld-suite-local-ca.pem"
        stage(str(self.root), ORIGIN, CHANNELS, ca)
        content = self.agent.read_text()
        self.assertIn("CA_CERTIFICATE_PATH=" + ca + "\n", content)
        stage(str(self.root), ORIGIN, CHANNELS, ca)
        self.assertEqual(self.agent.read_text(), content)
        with self.assertRaises(NotReady):
            stage(str(self.root), ORIGIN, CHANNELS,
                  "/etc/ssl/certs/different-ca.pem")
        self.assertEqual(self.agent.read_text(), content)

    def test_invalid_ca_path_does_not_mutate_staging(self):
        first = self.agent.read_text()
        second = self.daemon.read_text()
        for value in ("relative.pem", "/etc/ssl/../secrets", "/etc//ssl/ca.pem",
                      "/etc/ssl/ca cert.pem"):
            with self.assertRaises(NotReady):
                stage(str(self.root), ORIGIN, CHANNELS, value)
        self.assertEqual(self.agent.read_text(), first)
        self.assertEqual(self.daemon.read_text(), second)

    def test_existing_explicit_daemon_policy_is_preserved(self):
        self.daemon.write_text(self.daemon.read_text() +
                               "\nVDR_SUITE_LOCAL_BACKEND_SETUP_ENABLED=false\n")
        with self.assertRaises(NotReady):
            stage(str(self.root), ORIGIN, CHANNELS)
        self.assertNotIn("CONTROL_PLANE_URL=" + ORIGIN, self.agent.read_text())


if __name__ == "__main__":
    unittest.main()

#!/usr/bin/env python3
"""Isolated, no-root tests for the post-enrollment boot activation helper."""
import importlib.util
import io
import json
import os
from pathlib import Path
import stat
import tempfile
import time
import unittest
from contextlib import redirect_stderr, redirect_stdout
from unittest.mock import Mock, patch

SOURCE = Path(__file__).resolve().parent / "vdr_suite_backend_agent_autostart.py"
spec = importlib.util.spec_from_file_location("agent_autostart", SOURCE)
subject = importlib.util.module_from_spec(spec)
spec.loader.exec_module(subject)


class AutostartTests(unittest.TestCase):
    def test_check_does_not_enable_or_restart(self):
        with (
            patch.object(subject, "preflight", return_value=(
                "disabled", "agt_existing", "default")),
            patch.object(subject, "command") as execute,
            patch("sys.argv", ["helper", "--check"]),
            redirect_stdout(io.StringIO()) as output,
        ):
            self.assertEqual(subject.main(), 0)
            self.assertIn("AGENT_BOOT_PREFLIGHT=PASS", output.getvalue())
            self.assertIn("AGENT_AUTOSTART=disabled", output.getvalue())
            execute.assert_not_called()

    def test_enable_once_without_start_or_restart(self):
        with (
            patch.object(subject, "preflight", return_value=(
                "disabled", "agt_existing", "default")) as preflight,
            patch.object(subject, "command", side_effect=(
                "", "enabled", "active")) as execute,
            patch.object(subject.os, "geteuid", return_value=0),
            patch("sys.argv", ["helper", "--enable"]),
            redirect_stdout(io.StringIO()) as output,
        ):
            self.assertEqual(subject.main(), 0)
            self.assertEqual(preflight.call_count, 2)
            self.assertEqual(execute.call_args_list[0].args[0],
                             ["systemctl", "enable", subject.UNIT])
            self.assertIn("AGENT_RESTART=NOT_REQUESTED", output.getvalue())
            for call in execute.call_args_list:
                self.assertNotIn("--now", call.args[0])
                self.assertNotIn("restart", call.args[0])
                self.assertNotIn("start", call.args[0])

    def test_reject_identity_change_before_enable(self):
        with (
            patch.object(subject, "preflight", side_effect=[
                ("disabled", "agt_original", "default"),
                ("disabled", "agt_replaced", "default")]),
            patch.object(subject, "command") as execute,
            patch.object(subject.os, "geteuid", return_value=0),
            patch("sys.argv", ["helper", "--enable"]),
            redirect_stderr(io.StringIO()),
        ):
            self.assertEqual(subject.main(), 1)
            execute.assert_not_called()

    def test_non_root_cannot_enable(self):
        with (
            patch.object(subject.os, "geteuid", return_value=1000),
            patch.object(subject, "preflight") as preflight,
            patch("sys.argv", ["helper", "--enable"]),
            redirect_stderr(io.StringIO()),
        ):
            self.assertEqual(subject.main(), 1)
            preflight.assert_not_called()

    def test_configuration_rejects_template_and_wrong_paths(self):
        with tempfile.TemporaryDirectory() as tmp:
            config = Path(tmp) / "backend-agent.conf"
            content = ("BACKEND_ID=default\n"
                       "IDENTITY_PATH=" + subject.IDENTITY + "\n"
                       "ENROLLMENT_PATH=" + subject.ENROLLMENT + "\n"
                       "CONTROL_PLANE_URL=https://control-plane.example.invalid\n")
            config.write_text(content)
            with self.assertRaises(subject.NotReady):
                subject.read_config(config)
            config.write_text(content.replace(
                "control-plane.example.invalid", "suite.example.org"))
            self.assertEqual(subject.read_config(config), "default")
            config.write_text(content.replace(
                "control-plane.example.invalid", "suite.example.org").replace(
                subject.IDENTITY, "/tmp/untrusted-identity"))
            with self.assertRaises(subject.NotReady):
                subject.read_config(config)

    def test_unconsumed_enrollment_and_unprotected_identity_fail(self):
        with tempfile.TemporaryDirectory() as tmp:
            identity = Path(tmp) / "identity"
            enrollment = Path(tmp) / "enrollment"
            identity.write_text("agent_id=agt_existing\nbackend_id=default\ncredential_secret=fixture-only\n")
            os.chmod(identity, 0o600)
            with patch.object(subject.pwd, "getpwnam",
                              return_value=Mock(pw_uid=os.stat(identity).st_uid)):
                self.assertEqual(subject.check_identity(identity, enrollment),
                                 ("agt_existing", "default"))
                enrollment.write_text("pending")
                with self.assertRaises(subject.NotReady):
                    subject.check_identity(identity, enrollment)
                enrollment.unlink()
                os.chmod(identity, 0o644)
                with self.assertRaises(subject.NotReady):
                    subject.check_identity(identity, enrollment)

    def test_fresh_authoritative_status_required(self):
        now = int(time.time())
        status = {
            "present": True, "backendId": "default", "agentId": "agt_existing",
            "state": "online", "backendGeneration": 286,
            "heartbeatSequence": 13, "lastHeartbeatAt": now - 3,
            "leaseExpiresAt": now + 100, "backendHealthObservation": {
                "present": True, "backendGeneration": 286,
                "acceptedAt": now - 5}}
        with patch.object(subject, "command", return_value=json.dumps(status)):
            self.assertEqual(subject.read_status("default"), "agt_existing")
            for change in [
                {"state": "offline"},
                {"leaseExpiresAt": now - 2},
                {"backendHealthObservation": {"present": False}},
                {"backendId": "other"}
            ]:
                bad = dict(status, **change)
                with patch.object(subject, "command", return_value=json.dumps(bad)):
                    with self.assertRaises(subject.NotReady):
                        subject.read_status("default")

    def test_no_enable_for_masked_or_overridden_unit(self):
        standard = {
            "LoadState": "loaded", "UnitFileState": "disabled",
            "ActiveState": "active", "SubState": "running",
            "User": "vdr", "Group": "vdr", "DropInPaths": "",
            "FragmentPath": "/usr/lib/systemd/system/" + subject.UNIT,
            "ExecStart": "{ path=" + subject.AGENT_BIN + " ; argv[]=" +
                         subject.AGENT_BIN + " --config " + str(subject.CONFIG) + " }"}
        with (
            patch.object(subject, "properties", return_value=standard),
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "check_identity",
                         return_value=("agt_existing", "default")),
            patch.object(subject, "read_status", return_value="agt_existing"),
        ):
            self.assertEqual(subject.preflight(),
                             ("disabled", "agt_existing", "default"))
            for change in [
                {"UnitFileState": "masked"},
                {"DropInPaths": "/etc/systemd/system/override.conf"},
                {"ActiveState": "inactive"},
                {"User": "root"}
            ]:
                with patch.object(subject, "properties",
                                  return_value=dict(standard, **change)):
                    with self.assertRaises(subject.NotReady):
                        subject.preflight()


    def test_package_stages_post_enrollment_helper_without_auto_start(self):
        root = SOURCE.parents[1]
        make = (root / "mk/install.mk").read_text(encoding="utf-8")
        manifest = (root / "debian/vdr-suite-backend-agent.install").read_text(
            encoding="utf-8").splitlines()
        rules = (root / "debian/rules").read_text(encoding="utf-8")
        control = (root / "debian/control").read_text(encoding="utf-8")
        self.assertIn("dh_installsystemd --no-enable --no-start", rules)
        self.assertIn("tools/vdr_suite_backend_agent_autostart.py", make)
        self.assertIn("usr/sbin/vdr-suite-backend-agent-autostart", manifest)
        self.assertIn("usr/share/man/man8/vdr-suite-backend-agent-autostart.8",
                      manifest)
        self.assertIn("Depends: ${shlibs:Depends}, ${misc:Depends}, python3",
                      control)
        self.assertTrue((root / "docs/man/man8/vdr-suite-backend-agent-autostart.8")
                        .is_file())


if __name__ == "__main__":
    unittest.main()

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


    def test_mld_first_run_status_without_enrollment_or_service_changes(self):
        with tempfile.TemporaryDirectory() as tmp:
            config = Path(tmp) / "agent.conf"
            identity = Path(tmp) / "identity"
            enrollment = Path(tmp) / "enrollment"
            config.write_text("BACKEND_ID=default\n")
            with (
                patch.object(subject, "CONFIG", config),
                patch.object(subject, "IDENTITY", str(identity)),
                patch.object(subject, "ENROLLMENT", str(enrollment)),
                patch.object(subject, "first_run_service_state", return_value="RUNNING"),
                patch.object(subject, "read_config", return_value="default") as read,
                patch.object(subject, "command") as execute,
                patch("sys.argv", ["helper", "--setup-status"]),
                redirect_stdout(io.StringIO()) as output,
            ):
                self.assertEqual(subject.main(), 0)
                self.assertIn("LOCAL_BACKEND_SETUP=ENROLLMENT_REQUIRED",
                              output.getvalue())
                read.assert_called_once()
                execute.assert_not_called()
                self.assertFalse(identity.exists())
                self.assertFalse(enrollment.exists())

                enrollment.write_text("fixture only")
                self.assertEqual(subject.first_run_agent_state(), "ENROLLMENT_PENDING")
                enrollment.unlink()
                identity.write_text("fixture only")
                self.assertEqual(subject.first_run_agent_state(), "IDENTITY_PRESENT")
                identity.unlink()
                with patch.object(subject, "read_config",
                                  side_effect=subject.NotReady("template")):
                    self.assertEqual(subject.first_run_agent_state(),
                                     "CONFIGURATION_REQUIRED")
                config.unlink()
                self.assertEqual(subject.first_run_agent_state(),
                                 "CONFIGURATION_REQUIRED")

    def test_mld_first_run_status_requires_running_vdr_and_daemon(self):
        with patch.object(subject, "first_run_service_state",
                          side_effect=["NOT_RUNNING"]) as status:
            self.assertEqual(subject.first_run_agent_state(), "VDR_UNAVAILABLE")
            status.assert_called_once_with("vdr.service")
        with patch.object(subject, "first_run_service_state",
                          side_effect=["RUNNING", "NOT_RUNNING"]) as status:
            self.assertEqual(subject.first_run_agent_state(), "DAEMON_UNAVAILABLE")
            self.assertEqual(status.call_count, 2)
        with patch.object(subject, "first_run_service_state",
                          side_effect=["RUNNING", "RUNNING", "NOT_INSTALLED"]):
            self.assertEqual(subject.first_run_agent_state(), "AGENT_NOT_INSTALLED")

    def test_prepare_local_enrollment_uses_existing_tool_once(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            state = root / "backend-agent"
            state.mkdir(mode=0o700)
            os.chmod(state, 0o700)
            database = root / "vdr-suite.db"
            database.write_text("fixture")
            enrollment = state / "enrollment"
            identity = state / "identity"
            uid, gid = os.getuid(), os.getgid()
            agent_unit = {
                "LoadState": "loaded", "ActiveState": "inactive",
                "UnitFileState": "disabled", "User": "vdr", "Group": "vdr",
                "DropInPaths": "",
                "ExecStart": subject.AGENT_BIN + " --config " + str(subject.CONFIG)
            }
            invocations = []

            def enroll_tool(argv):
                invocations.append(argv)
                enrollment.write_text("version=1\\nfixture-only")
                os.chmod(enrollment, 0o600)
                return ""

            with (
                patch.object(subject, "DATABASE", str(database)),
                patch.object(subject, "IDENTITY", str(identity)),
                patch.object(subject, "ENROLLMENT", str(enrollment)),
                patch.object(subject, "first_run_agent_state",
                             return_value="ENROLLMENT_REQUIRED"),
                patch.object(subject, "read_config", return_value="default"),
                patch.object(subject, "properties", return_value=agent_unit),
                patch.object(subject.pwd, "getpwnam",
                             return_value=Mock(pw_uid=uid, pw_gid=gid)),
                patch.object(subject.os, "geteuid", return_value=0),
                patch.object(subject.os, "fstat",
                             return_value=Mock(st_mode=stat.S_IFREG | 0o600,
                                               st_nlink=1, st_uid=0)),
                patch.object(subject.os, "fchown") as ownership,
                patch.object(subject, "command", side_effect=enroll_tool),
                patch("sys.argv", ["helper", "--prepare-enrollment"]),
                redirect_stdout(io.StringIO()) as output,
            ):
                self.assertEqual(subject.main(), 0)
                self.assertIn("LOCAL_BACKEND_SETUP=ENROLLMENT_PREPARED",
                              output.getvalue())
                self.assertEqual(invocations, [[
                    subject.ENROLL_BIN, "--database", str(database),
                    "--backend", "default", "--output", str(enrollment)
                ]])
                ownership.assert_called_once()
                self.assertEqual(ownership.call_args.args[1:], (uid, gid))
                self.assertFalse(identity.exists())
                self.assertTrue(enrollment.is_file())
                # Existing state cannot be re-enrolled by retrying the helper.
                self.assertEqual(subject.main(), 1)
                self.assertEqual(len(invocations), 1)

    def test_prepare_local_enrollment_refuses_non_root_and_setup_error(self):
        with (
            patch.object(subject.os, "geteuid", return_value=1000),
            patch.object(subject, "first_run_agent_state") as state,
            patch.object(subject, "command") as execute,
        ):
            with self.assertRaises(subject.NotReady):
                subject.prepare_local_enrollment()
            state.assert_not_called()
            execute.assert_not_called()
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="IDENTITY_PRESENT"),
            patch.object(subject, "command") as execute,
        ):
            with self.assertRaises(subject.NotReady):
                subject.prepare_local_enrollment()
            execute.assert_not_called()

    def test_local_activation_starts_once_checks_channels_and_enables_boot(self):
        with tempfile.TemporaryDirectory() as tmp:
            enrollment = Path(tmp) / "enrollment"
            enrollment.write_text("fixture-only")
            os.chmod(enrollment, 0o600)
            vdr_uid = os.stat(enrollment).st_uid
            current_unit = {
                "LoadState": "loaded", "ActiveState": "inactive",
                "UnitFileState": "disabled", "User": "vdr", "Group": "vdr",
                "DropInPaths": "",
                "FragmentPath": "/usr/lib/systemd/system/" + subject.UNIT,
                "ExecStart": subject.AGENT_BIN + " --config " + str(subject.CONFIG)
            }
            calls = []

            def systemctl(argv):
                calls.append(argv)
                if argv[:2] == ["systemctl", "is-enabled"]:
                    return "enabled"
                if argv[:2] == ["systemctl", "is-active"]:
                    return "active"
                return ""

            with (
                patch.object(subject.os, "geteuid", return_value=0),
                patch.object(subject, "first_run_agent_state",
                             return_value="ENROLLMENT_PENDING"),
                patch.object(subject, "read_config", return_value="default"),
                patch.object(subject, "properties", return_value=current_unit),
                patch.object(subject, "channel_observation_source") as channels,
                patch.object(subject, "ENROLLMENT", str(enrollment)),
                patch.object(subject, "IDENTITY", str(Path(tmp) / "identity")),
                patch.object(subject.pwd, "getpwnam",
                             return_value=Mock(pw_uid=vdr_uid)),
                patch.object(subject, "preflight", side_effect=[
                    ("disabled", "agt_first", "default"),
                    ("disabled", "agt_first", "default")]),
                patch.object(subject, "check_channel_observation") as observation,
                patch.object(subject, "command", side_effect=systemctl),
                patch("sys.argv", ["helper", "--activate-local"]),
                redirect_stdout(io.StringIO()) as output,
            ):
                self.assertEqual(subject.main(), 0)
                self.assertIn("LOCAL_BACKEND_SETUP=READY", output.getvalue())
                self.assertEqual(calls, [
                    ["systemctl", "start", subject.UNIT],
                    ["systemctl", "enable", subject.UNIT],
                    ["systemctl", "is-enabled", subject.UNIT],
                    ["systemctl", "is-active", subject.UNIT]
                ])
                channels.assert_called_once()
                self.assertEqual(observation.call_count, 2)
                observation.assert_any_call("default", "agt_first")

    def test_local_activation_refuses_unconfigured_channels_before_start(self):
        with tempfile.TemporaryDirectory() as tmp:
            enrollment = Path(tmp) / "enrollment"
            enrollment.write_text("fixture-only")
            os.chmod(enrollment, 0o600)
            service = {
                "LoadState": "loaded", "ActiveState": "inactive",
                "UnitFileState": "disabled", "User": "vdr", "Group": "vdr",
                "DropInPaths": "",
                "FragmentPath": "/usr/lib/systemd/system/" + subject.UNIT,
                "ExecStart": subject.AGENT_BIN + " --config " + str(subject.CONFIG)
            }
            with (
                patch.object(subject.os, "geteuid", return_value=0),
                patch.object(subject, "first_run_agent_state",
                             return_value="ENROLLMENT_PENDING"),
                patch.object(subject, "read_config", return_value="default"),
                patch.object(subject, "properties", return_value=service),
                patch.object(subject, "channel_observation_source",
                             side_effect=subject.NotReady("Channel observation is not configured")),
                patch.object(subject, "command") as execute,
            ):
                with self.assertRaises(subject.NotReady):
                    subject.activate_local_agent()
                execute.assert_not_called()

    def test_local_channel_readback_requires_fresh_matching_cursor(self):
        now = int(time.time())
        good = {
            "present": True, "backendId": "default", "agentId": "agt_first",
            "backendGeneration": 12,
            "channelObservation": {
                "present": True, "backendGeneration": 12,
                "snapshotGeneration": 1, "producerSequence": 4,
                "resourceRevision": "channels-v1", "factCount": 35,
                "acceptedAt": now - 3
            }
        }
        with patch.object(subject, "command", return_value=json.dumps(good)):
            subject.check_channel_observation("default", "agt_first")
        for item in [
            dict(good, agentId="agt_wrong"),
            dict(good, channelObservation=dict(good["channelObservation"],
                                               present=False)),
            dict(good, channelObservation=dict(good["channelObservation"],
                                               backendGeneration=11)),
            dict(good, channelObservation=dict(good["channelObservation"],
                                               acceptedAt=now - 10000))
        ]:
            with patch.object(subject, "command", return_value=json.dumps(item)):
                with self.assertRaises(subject.NotReady):
                    subject.check_channel_observation("default", "agt_first")

    def test_local_channel_source_requires_declared_adapter_and_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            config = Path(tmp) / "agent.conf"
            channels = Path(tmp) / "channels.conf"
            channels.write_text("fixture")
            config.write_text("ADAPTERS=\nOBSERVATION_DOMAINS=backend-health\n"
                              f"CHANNELS_CONF_PATH={channels}\n")
            with self.assertRaises(subject.NotReady):
                subject.channel_observation_source(config)
            config.write_text(
                "ADAPTERS=channels-conf\nOBSERVATION_DOMAINS=backend-health,channels\n"
                f"CHANNELS_CONF_PATH={channels}\n")
            self.assertEqual(subject.channel_observation_source(config), channels)
            # MLD channels.conf may be a symlink to a selected channel set.
            selected = Path(tmp) / "selected.conf"
            channels.rename(selected)
            channels.symlink_to(selected)
            self.assertEqual(subject.channel_observation_source(config), channels)
            selected.unlink()
            with self.assertRaises(subject.NotReady):
                subject.channel_observation_source(config)

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

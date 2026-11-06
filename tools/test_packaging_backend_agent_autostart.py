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

    def test_prepare_local_enrollment_identifies_rejected_unit_property(self):
        # Read-only systemd property classification must not leak the unit's
        # ExecStart, credentials, environment, or attempt registration.
        valid = {
            "LoadState": "loaded", "ActiveState": "inactive",
            "UnitFileState": "disabled", "User": "vdr", "Group": "vdr",
            "DropInPaths": "",
            "ExecStart": subject.AGENT_BIN + " --config " + str(subject.CONFIG),
        }
        mismatches = (
            ("LoadState", "not-found", "load-state"),
            ("ActiveState", "active", "active-state"),
            ("UnitFileState", "static", "unit-file-state"),
            ("User", "root", "service-user"),
            ("Group", "root", "service-group"),
            ("DropInPaths", "/etc/systemd/system/secret.conf", "drop-ins"),
            ("ExecStart", "/usr/sbin/different-agent --config /some/file",
             "agent-executable"),
            ("ExecStart", subject.AGENT_BIN + " --other hidden-token",
             "agent-config-argument"),
        )
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="ENROLLMENT_REQUIRED"),
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "command") as execute,
        ):
            for key, value, failure in mismatches:
                with self.subTest(field=key, value=failure), (
                    patch.object(subject, "properties",
                                 return_value=dict(valid, **{key: value}))
                ):
                    with self.assertRaises(subject.NotReady) as context:
                        subject.prepare_local_enrollment()
                    self.assertEqual(
                        str(context.exception),
                        "installed Agent service is not ready for initial setup: "
                        + failure,
                    )
                    self.assertNotIn("hidden-token", str(context.exception))
                    self.assertNotIn("secret.conf", str(context.exception))
            execute.assert_not_called()

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

    def test_local_activation_resumes_running_agent_without_start_or_reenrollment(self):
        # The MLD failure leaves the protected enrollment file present while
        # the reviewed Agent is active. A safe retry must not start or enroll
        # a second time; it can only enable boot after authenticated readback.
        with tempfile.TemporaryDirectory() as tmp:
            enrollment = Path(tmp) / "enrollment"
            enrollment.write_text("fixture-only")
            os.chmod(enrollment, 0o600)
            agent_unit = {
                "LoadState": "loaded", "ActiveState": "active",
                "SubState": "running", "UnitFileState": "disabled",
                "User": "vdr", "Group": "vdr", "DropInPaths": "",
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
                patch.object(subject, "properties", return_value=agent_unit),
                patch.object(subject, "channel_observation_source"),
                patch.object(subject, "ENROLLMENT", str(enrollment)),
                patch.object(subject, "IDENTITY", str(Path(tmp) / "identity")),
                patch.object(subject.pwd, "getpwnam",
                             return_value=Mock(pw_uid=enrollment.stat().st_uid)),
                patch.object(subject, "preflight", return_value=(
                    "disabled", "agt_first", "default")),
                patch.object(subject, "check_channel_observation"),
                patch.object(subject, "command", side_effect=systemctl),
            ):
                self.assertEqual(subject.activate_local_agent(), "READY")
                self.assertEqual(calls, [
                    ["systemctl", "enable", subject.UNIT],
                    ["systemctl", "is-enabled", subject.UNIT],
                    ["systemctl", "is-active", subject.UNIT],
                ])
                self.assertFalse(any(call[:2] == ["systemctl", "start"]
                                     for call in calls))
                self.assertTrue(enrollment.exists())

                # An active unit without the reviewed 'running' substate
                # must still be rejected without any service mutation.
                agent_unit["SubState"] = "exited"
                calls.clear()
                with self.assertRaises(subject.NotReady):
                    subject.activate_local_agent()
                self.assertEqual(calls, [])

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

    def test_installer_onboard_prepares_once_then_activates(self):
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="ENROLLMENT_REQUIRED"),
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "channel_observation_source") as channels,
            patch.object(subject, "prepare_local_enrollment") as prepare,
            patch.object(subject, "activate_local_agent",
                         return_value="READY") as activate,
            patch("sys.argv", ["helper", "--installer-onboard"]),
            redirect_stdout(io.StringIO()) as output,
        ):
            self.assertEqual(subject.main(), 0)
            self.assertIn("LOCAL_BACKEND_SETUP=READY", output.getvalue())
            channels.assert_called_once()
            prepare.assert_called_once()
            activate.assert_called_once()

    def test_installer_onboard_pending_never_registers_twice(self):
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="ENROLLMENT_PENDING"),
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "channel_observation_source"),
            patch.object(subject, "prepare_local_enrollment") as prepare,
            patch.object(subject, "activate_local_agent",
                         return_value="READY") as activate,
        ):
            self.assertEqual(subject.installer_onboard_local(), "READY")
            prepare.assert_not_called()
            activate.assert_called_once()

    def test_installer_onboard_existing_identity_is_readback_only(self):
        calls = []
        def systemctl(argv):
            calls.append(argv)
            if argv[:2] == ["systemctl", "is-enabled"]:
                return "enabled"
            if argv[:2] == ["systemctl", "is-active"]:
                return "active"
            return ""
        unit = {
            "LoadState": "loaded", "ActiveState": "active",
            "UnitFileState": "enabled", "User": "vdr", "Group": "vdr",
            "DropInPaths": "",
            "FragmentPath": "/usr/lib/systemd/system/" + subject.UNIT,
            "ExecStart": subject.AGENT_BIN + " --config " + str(subject.CONFIG)
        }
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="IDENTITY_PRESENT"),
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "channel_observation_source"),
            patch.object(subject, "properties", return_value=unit),
            patch.object(subject, "check_identity",
                         return_value=("agt_existing", "default")),
            patch.object(subject, "prepare_local_enrollment") as prepare,
            patch.object(subject, "activate_local_agent") as activate,
            patch.object(subject, "preflight",
                         return_value=("enabled", "agt_existing", "default")),
            patch.object(subject, "check_channel_observation") as observe,
            patch.object(subject, "command", side_effect=systemctl),
        ):
            self.assertEqual(subject.installer_onboard_local(), "READY")
            prepare.assert_not_called()
            activate.assert_not_called()
            observe.assert_called_once_with("default", "agt_existing")
            self.assertEqual(calls, [
                ["systemctl", "is-enabled", subject.UNIT],
                ["systemctl", "is-active", subject.UNIT]
            ])

    def test_installer_onboard_resumes_consumed_enrollment_after_power_loss(self):
        # Enrollment was consumed and the identity stored, but the service
        # was stopped before the first Channel observation and boot enable.
        unit = {
            "LoadState": "loaded", "ActiveState": "inactive",
            "UnitFileState": "disabled", "User": "vdr", "Group": "vdr",
            "DropInPaths": "",
            "FragmentPath": "/lib/systemd/system/" + subject.UNIT,
            "ExecStart": subject.AGENT_BIN + " --config " + str(subject.CONFIG)
        }
        calls = []
        def execute(argv):
            calls.append(argv)
            if argv[:2] == ["systemctl", "is-enabled"]:
                return "enabled"
            if argv[:2] == ["systemctl", "is-active"]:
                return "active"
            return ""
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="IDENTITY_PRESENT"),
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "channel_observation_source"),
            patch.object(subject, "properties", return_value=unit),
            patch.object(subject, "check_identity",
                         return_value=("agt_original", "default")),
            patch.object(subject, "prepare_local_enrollment") as enroll,
            patch.object(subject, "activate_local_agent") as activate,
            patch.object(subject, "preflight",
                         return_value=("disabled", "agt_original", "default")) as live,
            patch.object(subject, "check_channel_observation") as channels,
            patch.object(subject, "command", side_effect=execute),
        ):
            self.assertEqual(subject.installer_onboard_local(), "READY")
            enroll.assert_not_called()
            activate.assert_not_called()
            self.assertEqual(live.call_count, 2)
            self.assertEqual(channels.call_count, 2)
            self.assertEqual(calls, [
                ["systemctl", "start", subject.UNIT],
                ["systemctl", "enable", subject.UNIT],
                ["systemctl", "is-enabled", subject.UNIT],
                ["systemctl", "is-active", subject.UNIT]
            ])

    def test_installer_onboard_refuses_unreviewed_identity_unit(self):
        unit = {
            "LoadState": "loaded", "ActiveState": "inactive",
            "UnitFileState": "disabled", "User": "root", "Group": "root",
            "DropInPaths": "",
            "FragmentPath": "/usr/lib/systemd/system/" + subject.UNIT,
            "ExecStart": subject.AGENT_BIN + " --config " + str(subject.CONFIG)
        }
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="IDENTITY_PRESENT"),
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "channel_observation_source"),
            patch.object(subject, "properties", return_value=unit),
            patch.object(subject, "command") as command,
            patch.object(subject, "prepare_local_enrollment") as enroll,
        ):
            with self.assertRaises(subject.NotReady):
                subject.installer_onboard_local()
            command.assert_not_called()
            enroll.assert_not_called()

    def test_installer_onboard_requires_channels_and_local_topology(self):
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="ENROLLMENT_REQUIRED"),
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "channel_observation_source",
                         side_effect=subject.NotReady("not configured")),
            patch.object(subject, "prepare_local_enrollment") as prepare,
        ):
            with self.assertRaises(subject.NotReady):
                subject.installer_onboard_local()
            prepare.assert_not_called()
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="VDR_UNAVAILABLE"),
            patch.object(subject, "prepare_local_enrollment") as prepare,
        ):
            with self.assertRaises(subject.NotReady):
                subject.installer_onboard_local()
            prepare.assert_not_called()
        with (
            patch.object(subject.os, "geteuid", return_value=1001),
            patch.object(subject, "first_run_agent_state") as state,
        ):
            with self.assertRaises(subject.NotReady):
                subject.installer_onboard_local()
            state.assert_not_called()

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


    def test_auto_first_run_preserves_existing_identity_service_policy(self):
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="IDENTITY_PRESENT"),
            patch.object(subject, "configure_local_first_run") as config,
            patch.object(subject, "installer_onboard_local") as onboard,
        ):
            self.assertEqual(subject.automatic_first_run(), "ALREADY_CONFIGURED")
            config.assert_not_called()
            onboard.assert_not_called()

    def test_automatic_first_run_configures_fresh_package_and_enrolls_once(self):
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         side_effect=("CONFIGURATION_REQUIRED", "ENROLLMENT_REQUIRED")),
            patch.object(subject, "configure_local_first_run", return_value=True) as setup,
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "channel_observation_source") as channels,
            patch.object(subject, "installer_onboard_local",
                         return_value="READY") as onboard,
        ):
            self.assertEqual(subject.automatic_first_run(), "READY")
            setup.assert_called_once_with()
            channels.assert_called_once_with()
            onboard.assert_called_once_with()

    def test_runtime_config_is_idempotent_and_never_reenrolls_existing_identity(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            config = root / "backend-agent.conf"
            channels = root / "channels.conf"
            channels.write_text("fixture channels")
            identity = root / "identity"
            enrollment = root / "enrollment"
            config.write_text(
                "CONTROL_PLANE_URL=https://control-plane.example.invalid\n"
                "BACKEND_ID=default\n"
                "IDENTITY_PATH=" + str(identity) + "\n"
                "ENROLLMENT_PATH=" + str(enrollment) + "\n"
                "ADAPTERS=\n"
                "OBSERVATION_DOMAINS=backend-health\n"
                "CHANNELS_CONF_PATH=\n")
            with (
                patch.object(subject, "CONFIG", config),
                patch.object(subject, "IDENTITY", str(identity)),
                patch.object(subject, "ENROLLMENT", str(enrollment)),
                patch.object(subject, "DEFAULT_CHANNEL_SOURCES", (str(channels),)),
                patch.object(subject.os, "geteuid", return_value=0),
            ):
                # This test uses fixture identity paths, so validation is
                # represented by the already tested read_config() contract.
                with patch.object(subject, "read_config", return_value="default"):
                    self.assertTrue(subject.configure_local_first_run())
                    content = config.read_text()
                    self.assertIn("CONTROL_PLANE_URL=http://127.0.0.1:18080", content)
                    self.assertIn("ADAPTERS=channels-conf", content)
                    self.assertIn("OBSERVATION_DOMAINS=backend-health,channels", content)
                    self.assertIn("CHANNELS_CONF_PATH=" + str(channels), content)
                    self.assertFalse(subject.configure_local_first_run())
                    identity.write_text("fixture existing identity")
                    self.assertFalse(subject.configure_local_first_run())
                    self.assertEqual(config.read_text(), content)

    def test_automatic_first_run_defers_without_touching_existing_system(self):
        for state in ("VDR_UNAVAILABLE", "DAEMON_UNAVAILABLE",
                      "AGENT_NOT_INSTALLED", "CONFIGURATION_REQUIRED",
                      "UNEXPECTED_AGENT_STATE"):
            with self.subTest(state=state):
                with (
                    patch.object(subject.os, "geteuid", return_value=0),
                    patch.object(subject, "first_run_agent_state", return_value=state),
                    patch.object(subject, "installer_onboard_local") as onboard,
                    patch.object(subject, "configure_local_first_run",
                                 side_effect=subject.NotReady("not configured")) as configure,
                    patch.object(subject, "command") as execute,
                ):
                    self.assertEqual(subject.automatic_first_run(), "DEFERRED_" + state)
                    if state == "CONFIGURATION_REQUIRED":
                        configure.assert_called_once_with()
                    else:
                        configure.assert_not_called()
                    onboard.assert_not_called()
                    execute.assert_not_called()

    def test_automatic_first_run_deferred_on_missing_channels(self):
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="ENROLLMENT_REQUIRED"),
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "channel_observation_source",
                         side_effect=subject.NotReady("not configured")),
            patch.object(subject, "installer_onboard_local") as onboard,
        ):
            self.assertEqual(subject.automatic_first_run(),
                             "DEFERRED_CONFIGURATION_REQUIRED")
            onboard.assert_not_called()

    def test_automatic_first_run_uses_same_existing_onboard_path(self):
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="ENROLLMENT_PENDING"),
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "channel_observation_source"),
            patch.object(subject, "installer_onboard_local",
                         return_value="READY") as onboard,
            patch("sys.argv", ["helper", "--auto-first-run"]),
            redirect_stdout(io.StringIO()) as output,
        ):
            self.assertEqual(subject.main(), 0)
            self.assertIn("LOCAL_BACKEND_SETUP=READY", output.getvalue())
            onboard.assert_called_once_with()

    def test_local_daemon_origin_match_is_exact_and_read_only(self):
        with tempfile.TemporaryDirectory() as temporary:
            config = Path(temporary) / "backend-agent.conf"
            with patch.object(subject, "CONFIG", config):
                self.assertFalse(subject.local_control_plane_configured())
                config.write_text("CONTROL_PLANE_URL=http://127.0.0.1:18080\n")
                self.assertTrue(subject.local_control_plane_configured())
                for value in ("http://127.0.0.1:18080/",
                              "http://localhost:18080",
                              "https://remote.example.test"):
                    config.write_text("CONTROL_PLANE_URL=" + value + "\n")
                    self.assertFalse(subject.local_control_plane_configured())

    def test_first_run_waits_for_local_daemon_listener_before_enrollment(self):
        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="ENROLLMENT_REQUIRED"),
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "channel_observation_source"),
            patch.object(subject, "local_control_plane_configured",
                         return_value=True),
            patch.object(subject, "wait_for_local_daemon_listener",
                         return_value=False) as listener,
            patch.object(subject, "installer_onboard_local") as onboard,
        ):
            self.assertEqual(subject.automatic_first_run(),
                             "DEFERRED_DAEMON_UNAVAILABLE")
            listener.assert_called_once_with()
            onboard.assert_not_called()

        with (
            patch.object(subject.os, "geteuid", return_value=0),
            patch.object(subject, "first_run_agent_state",
                         return_value="ENROLLMENT_PENDING"),
            patch.object(subject, "read_config", return_value="default"),
            patch.object(subject, "channel_observation_source"),
            patch.object(subject, "local_control_plane_configured",
                         return_value=True),
            patch.object(subject, "wait_for_local_daemon_listener",
                         return_value=True),
            patch.object(subject, "installer_onboard_local",
                         return_value="READY") as onboard,
        ):
            self.assertEqual(subject.automatic_first_run(), "READY")
            onboard.assert_called_once_with()

    def test_local_daemon_listener_poll_is_bounded(self):
        with (
            patch.object(subject.socket, "create_connection",
                         side_effect=OSError("unavailable")) as connect,
            patch.object(subject.time, "sleep") as sleep,
        ):
            self.assertFalse(subject.wait_for_local_daemon_listener(attempts=2))
            self.assertEqual(connect.call_count, 3)
            self.assertEqual(sleep.call_count, 2)
            connect.assert_called_with(("127.0.0.1", 18080), timeout=1)
        with patch.object(subject.socket, "create_connection") as connect:
            self.assertTrue(subject.wait_for_local_daemon_listener(attempts=2))
            connect.assert_called_once_with(("127.0.0.1", 18080), timeout=1)

    def test_automatic_first_run_does_not_run_unprivileged(self):
        with (
            patch.object(subject.os, "geteuid", return_value=1000),
            patch.object(subject, "first_run_agent_state") as detect,
            patch.object(subject, "installer_onboard_local") as onboard,
        ):
            with self.assertRaises(subject.NotReady):
                subject.automatic_first_run()
            detect.assert_not_called()
            onboard.assert_not_called()


if __name__ == "__main__":
    unittest.main()

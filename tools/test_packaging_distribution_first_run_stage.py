#!/usr/bin/env python3
"""No-root regression for shared first-run package configuration."""
from pathlib import Path
import os
import shutil
import stat
import subprocess
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

    def test_fresh_package_defaults_to_local_control_plane(self):
        stage(self.package, "", "/var/lib/vdr/channels.conf")
        self.assertIn("CONTROL_PLANE_URL=http://127.0.0.1:18080",
                      self.agent.read_text())

    def test_rejects_unconfigured_or_live_staging(self):
        for origin in ("http://127.0.0.1:8080", "https://control-plane.example.invalid"):
            with self.subTest(origin=origin), self.assertRaises(NotReady):
                stage(self.package, origin, "/var/lib/vdr/channels.conf")
        with self.assertRaises(NotReady):
            stage("/", "https://suite.example.org", "/var/lib/vdr/channels.conf")


    def test_fresh_debian_hooks_have_valid_shell_syntax_and_skip_upgrades(self):
        for filename, expected in (
            ("vdr-suite-daemon.postinst", "deb-systemd-invoke start vdr-suite-daemon.service"),
            ("vdr-suite-backend-agent.postinst",
             "deb-systemd-invoke start vdr-suite-first-run.service"),
        ):
            with self.subTest(filename=filename):
                script = ROOT / "debian" / filename
                content = script.read_text(encoding="utf-8")
                self.assertTrue(script.stat().st_mode & stat.S_IXUSR)
                self.assertIn('if [ "$1" = configure ] && [ -z "${2:-}" ]', content)
                self.assertIn(expected, content)
                self.assertIn("#DEBHELPER#", content)
                subprocess.run(["sh", "-n", str(script)], check=True)

    def test_postinst_first_install_reloads_units_before_service_start(self):
        # --no-start need not emit a debhelper daemon-reload snippet.
        # Exercise the real maintainer scripts with stubbed systemctl helpers,
        # on a fake systemd runtime: never touch host services or /run.
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            systemd = root / "systemd-running"
            systemd.mkdir()
            trace = root / "commands.log"
            environment = dict(os.environ)
            environment["TRACE"] = str(trace)
            environment["PATH"] = str(root) + os.pathsep + environment.get("PATH", "")
            for name in ("systemctl", "deb-systemd-helper", "deb-systemd-invoke"):
                stub = root / name
                stub.write_text(
                    '#!/bin/sh\nprintf "%s %s\\n" "$(basename "$0")" "$*" >> "$TRACE"\n',
                    encoding="utf-8",
                )
                stub.chmod(0o755)

            for filename, service in (
                ("vdr-suite-daemon.postinst", "vdr-suite-daemon.service"),
                ("vdr-suite-backend-agent.postinst", "vdr-suite-first-run.service"),
            ):
                with self.subTest(filename=filename):
                    content = (ROOT / "debian" / filename).read_text(encoding="utf-8")
                    # Only the mocked test copy uses an alternative runtime root.
                    content = content.replace("/run/systemd/system", str(systemd))
                    script = root / filename
                    script.write_text(content, encoding="utf-8")

                    def run_postinst(previous_version):
                        trace.write_text("", encoding="utf-8")
                        subprocess.run(
                            ["sh", str(script), "configure", previous_version],
                            env=environment, check=True,
                        )
                        return trace.read_text(encoding="utf-8").splitlines()

                    fresh = run_postinst("")
                    self.assertGreaterEqual(len(fresh), 2)
                    self.assertEqual(fresh[0], "systemctl --system daemon-reload")
                    self.assertIn("deb-systemd-invoke start " + service, fresh)
                    upgrade = run_postinst("0.14.0")
                    self.assertEqual(upgrade, ["systemctl --system daemon-reload"])

    def test_mld_cli_refuses_missing_first_run_package_payload(self):
        # Reproduce the MLD target's missing first-run units entirely in a
        # disposable DESTDIR. Never inspect or modify installed host units.
        command = [
            "python3", str(ROOT / "tools/mld66_stage_first_run.py"),
            "--stage-root", str(self.package),
            "--channels-source", "/etc/vdr/channels.conf",
        ]
        untouched = self.agent.read_bytes()
        for missing in ("vdr-suite-first-run.service",
                        "vdr-suite-first-run.path"):
            with self.subTest(missing=missing):
                systemd = self.package / "usr/lib/systemd/system"
                systemd.mkdir(parents=True, exist_ok=True)
                for name in ("vdr-suite-daemon.service",
                             "vdr-suite-backend-agent.service",
                             "vdr-suite-first-run.service",
                             "vdr-suite-first-run.path"):
                    source = ROOT / "packaging/systemd" / name
                    shutil.copyfile(source, systemd / name)
                (systemd / missing).unlink()
                helper = self.package / "usr/sbin/vdr-suite-backend-agent-autostart"
                helper.parent.mkdir(parents=True, exist_ok=True)
                helper.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
                helper.chmod(0o755)
                result = subprocess.run(
                    command, capture_output=True, text=True, check=False
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("MLD_FIRST_RUN_STAGE=BLOCKED", result.stderr)
                self.assertEqual(self.agent.read_bytes(), untouched)

        # The complete source-controlled systemd payload passes the same
        # preflight; only the staged config changes.
        shutil.copyfile(ROOT / "packaging/systemd/vdr-suite-first-run.path",
                        self.package / "usr/lib/systemd/system/vdr-suite-first-run.path")
        result = subprocess.run(
            command, capture_output=True, text=True, check=False
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("MLD_FIRST_RUN_STAGE=PASS", result.stdout)
        self.assertIn("CONTROL_PLANE_URL=http://127.0.0.1:18080",
                      self.agent.read_text())
        self.assertIn("CHANNELS_CONF_PATH=/etc/vdr/channels.conf",
                      self.agent.read_text())

    def test_common_install_contract_wires_auto_first_run_for_deb_and_manual(self):
        makefile = (ROOT / "mk/install.mk").read_text(encoding="utf-8")
        manifest = (ROOT / "debian/vdr-suite-daemon.install").read_text(
            encoding="utf-8")
        daemon_unit = (ROOT / "packaging/systemd/vdr-suite-daemon.service").read_text(
            encoding="utf-8")
        first_run_unit = (ROOT / "packaging/systemd/vdr-suite-first-run.service").read_text(
            encoding="utf-8")
        first_run_path = (ROOT / "packaging/systemd/vdr-suite-first-run.path").read_text(
            encoding="utf-8")
        helper = (ROOT / "tools/vdr_suite_backend_agent_autostart.py").read_text(
            encoding="utf-8")
        for name in ("vdr-suite-first-run.service", "vdr-suite-first-run.path"):
            with self.subTest(name=name):
                self.assertIn("packaging/systemd/" + name, makefile)
                self.assertIn("usr/lib/systemd/system/" + name, manifest)
                self.assertIn(name, daemon_unit)
        self.assertIn("ExecStart=/usr/sbin/vdr-suite-backend-agent-autostart --auto-first-run",
                      first_run_unit)
        self.assertIn("After=network-online.target vdr.service vdr-suite-daemon.service",
                      first_run_unit)
        self.assertIn("PathChanged=/etc/vdr-suite/backend-agent.conf", first_run_path)
        self.assertIn('choice.add_argument("--auto-first-run"', helper)
        self.assertIn("def automatic_first_run():", helper)


    def test_mld_readonly_caps_uses_installed_svdrp_command(self):
        # Do not probe a real VDR: isolate the PATH completely and use only
        # deterministic mock executables for both distribution variants.
        scripts = (
            "mld66-readonly-inventory.sh",
            "mld66-live-readonly-diagnostics.sh",
        )
        for filename in scripts:
            subprocess.run(["/bin/sh", "-n", str(ROOT / "tools" / filename)],
                           check=True)
        for commands, chosen in (
            (("svdrpsend",), "NATIVE"),
            (("svdrpsend.sh",), "MLD"),
            (("svdrpsend", "svdrpsend.sh"), "NATIVE"),
        ):
            with self.subTest(commands=commands):
                with tempfile.TemporaryDirectory() as temporary:
                    bin_dir = Path(temporary)
                    sed_bin = shutil.which("sed")
                    self.assertIsNotNone(sed_bin)
                    (bin_dir / "sed").symlink_to(sed_bin)
                    for name, body in (
                        ("uname", "printf 'mock-arch\n'"),
                        ("timeout", 'shift; exec "$@"'),
                    ):
                        stub = bin_dir / name
                        stub.write_text("#!/bin/sh\n" + body + "\n",
                                        encoding="utf-8")
                        stub.chmod(0o755)
                    for name in commands:
                        stub = bin_dir / name
                        marker = "NATIVE" if name == "svdrpsend" else "MLD"
                        stub.write_text(
                            "#!/bin/sh\nprintf 'SVDRP_TEST="
                            + marker + "\n'\n",
                            encoding="utf-8",
                        )
                        stub.chmod(0o755)
                    environment = {"PATH": str(bin_dir), "LANG": "C"}
                    for filename in scripts:
                        with self.subTest(script=filename):
                            completed = subprocess.run(
                                ["/bin/sh", str(ROOT / "tools" / filename), "--caps"],
                                check=True, capture_output=True, text=True,
                                env=environment, timeout=10,
                            )
                            self.assertIn("SVDRP_TEST=" + chosen,
                                          completed.stdout)
                            self.assertNotIn(
                                "SUITEBRIDGE_CAPS=FAILED_OR_UNAVAILABLE",
                                completed.stdout,
                            )

if __name__ == "__main__":
    unittest.main()

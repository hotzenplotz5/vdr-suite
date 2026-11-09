"""MT.I3.3: read-only and synthetic cross-distro Media Tools regressions.

No package, service, live database, source movie or VDR recording is changed.
A synthetic distro is NOT a successful MLD/openSUSE host acceptance.
"""
from __future__ import annotations

import json
from pathlib import Path
import subprocess
import tempfile
import unittest

import install_media_tools_payload as payload
import media_tools_install_preflight as preflight
import stage_media_tools_runtime as stage
import test_install_media_tools_payload as fixtures

ROOT = Path(__file__).resolve().parents[2]
INSTALLER = ROOT / "tools/addons/install_media_tools.sh"
UNIT = ROOT / "packaging/systemd/vdr-suite-backend-agent.service"
DISTROS = ("ubuntu", "mld", "opensuse", "opensuse-leap")


class DistributionProfileTests(unittest.TestCase):
    def test_profiles_are_read_only_and_never_claim_non_ubuntu_deployment(self):
        for distro in DISTROS:
            with self.subTest(distro=distro), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                release = fixtures.write(root, "etc/os-release",
                                         ("ID=" + distro + "\n").encode(), 0o644)
                original = release.read_bytes()
                reported = preflight.host_profile(root)
                self.assertEqual(reported["distribution_id"], distro)
                self.assertEqual(reported["mode"], "READ_ONLY")
                self.assertEqual(reported["live_deployment"], "NOT_AUTHORIZED")
                self.assertEqual(reported["service_state"], "NOT_VERIFIED")
                self.assertEqual(reported["vdr_abi"], "NOT_VERIFIED")
                self.assertEqual(reported["agent_account_and_group"], "NOT_VERIFIED")
                self.assertEqual(reported["source_root"], "NOT_VERIFIED")
                self.assertEqual(reported["video_root"], "NOT_VERIFIED")
                if distro == "ubuntu":
                    self.assertEqual(reported["existing_host_adapter"],
                                     "ubuntu-systemd-reference")
                else:
                    self.assertEqual(reported["existing_host_adapter"],
                                     "NOT_VERIFIED")
                    self.assertEqual(reported["non_ubuntu_install"], "BLOCKED")
                result = subprocess.run(["bash", str(INSTALLER), "profile", str(root)],
                                        capture_output=True, text=True, timeout=15)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("MEDIA_TOOLS_HOST_PROFILE=READ_ONLY", result.stdout)
                cli_json = result.stdout.split("MEDIA_TOOLS_HOST_PROFILE=READ_ONLY")[0]
                self.assertEqual(json.loads(cli_json)["distribution_id"], distro)
                self.assertEqual(release.read_bytes(), original)
                self.assertFalse((root / "etc/vdr-suite").exists())

    def test_release_symlink_must_resolve_to_local_usr_lib(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            fixtures.write(root, "usr/lib/os-release", b"ID=mld\n", 0o644)
            link = root / "etc/os-release"
            link.parent.mkdir()
            link.symlink_to("../usr/lib/os-release")
            self.assertEqual(preflight.host_profile(root)["distribution_id"], "mld")
            link.unlink()
            link.symlink_to("/etc/passwd")
            with self.assertRaises(preflight.PreflightError):
                preflight.host_profile(root)

    def test_invalid_distribution_identifier_is_rejected_without_execution(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            fixtures.write(root, "etc/os-release", b"ID=../../unsafe\n", 0o644)
            with self.assertRaises(preflight.PreflightError):
                preflight.host_profile(root)

    def test_agent_service_contract_and_ubuntu_only_live_entrypoint(self):
        unit = UNIT.read_text(encoding="utf-8")
        script = INSTALLER.read_text(encoding="utf-8")
        self.assertIn("Restart=on-failure", unit)
        self.assertIn("ConditionPathExists=/etc/vdr-suite/backend-agent.conf", unit)
        self.assertIn("ExecStart=/usr/sbin/vdr-suite-backend-agent --config /etc/vdr-suite/backend-agent.conf", unit)
        # Template User=vdr is NOT evidence of actual MLD/openSUSE account.
        self.assertIn("[ -f /etc/os-release ] && grep -Fqx 'ID=ubuntu' /etc/os-release", script)
        self.assertIn('if [ "${1:-}" = profile ]; then', script)


@unittest.skipUnless(Path("/usr/bin/true").is_file(), "synthetic ELF fixture missing")
class SyntheticPayloadDistributionTests(unittest.TestCase):
    def setUp(self):
        # Reuse the already tested sealed MT.I4 fixture; no second stage format.
        fixtures.PortableMediaToolsInstallTests.setUp(self)

    def test_update_is_idempotent_and_preserves_live_state_for_each_os(self):
        for distro in DISTROS:
            with self.subTest(distro=distro):
                root = Path(self.temporary.name) / ("host-" + distro)
                root.mkdir()
                (root / "root").mkdir(mode=0o700)
                keep = {
                    "etc/os-release": ("ID=" + distro + "\n").encode(),
                    "etc/vdr-suite/backend-agent.conf":
                        b"BACKEND_ID=default\nMEDIA_FFMPEG_PATH=/opt/ffmpeg/bin/ffmpeg\n",
                    "etc/vdr-suite/addon-sources/media-import.tsv":
                        b"source-fixture-opaque\n",
                    "etc/systemd/system/vdr-suite-backend-agent.service.d/20-media-tools.conf":
                        b"[Service]\nReadWritePaths=/different/video\n",
                    "var/lib/vdr-suite/vdr-suite.db": b"state must survive",
                    "srv/vdr/video.00/keep.ts": b"original recording",
                    "srv/media/input/keep.mkv": b"original source",
                }
                for rel, content in keep.items():
                    mode = 0o600 if rel.startswith("etc/vdr-suite/") else 0o644
                    fixtures.write(root, rel, content, mode)
                backend_grant = "etc/vdr-suite/addon-approvals/rectools.remote.json"
                keep[backend_grant] = b"backend-specific approval unchanged"
                fixtures.write(root, backend_grant, keep[backend_grant], 0o600)

                result = payload.install(self.stage, root, self.approved)
                self.assertIn("MEDIA_TOOLS_INSTALL=PASS", result)
                report = preflight.inspect(self.stage, root, self.approved,
                                           query_packages=False)
                self.assertEqual(report["install_state"], "identical_payload")
                self.assertEqual(report["payload_counts"]["match"],
                                 len(stage.payload_roles()) + 1)
                self.assertEqual(report["deployment"], "NOT_AUTHORIZED")
                for rel, content in keep.items():
                    self.assertEqual((root / rel).read_bytes(), content, rel)
                self.assertFalse((root / "etc/vdr-suite/addon-approvals/rectools.json").exists())
                backups = list((root / "root").iterdir())
                again = payload.install(self.stage, root, self.approved)
                self.assertIn("MEDIA_TOOLS_INSTALL=ALREADY_CURRENT", again)
                self.assertEqual(list((root / "root").iterdir()), backups)
                for rel, content in keep.items():
                    self.assertEqual((root / rel).read_bytes(), content, rel)


if __name__ == "__main__":
    unittest.main()

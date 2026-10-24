"""No-live, no-build regression for composing optional Media Tools into Suite's sealed runtime."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import stat
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "stage_media_tools_runtime", HERE / "stage_media_tools_runtime.py"
)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)
from install_runtime_stage import load_manifest, seal, verify_sealed_stage


def mode(path: Path) -> int:
    return stat.S_IMODE(path.stat().st_mode)


def private(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)
    path.chmod(0o700)


@unittest.skipIf(os.geteuid() == 0, "staging is intentionally forbidden as root")
@unittest.skipUnless(Path("/usr/bin/true").is_file(), "ELF test fixture unavailable")
class MediaToolsSealedStageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="media-tools-no-deb-stage-")
        self.addCleanup(self.temp.cleanup)
        self.work = Path(self.temp.name) / "prepared"
        self.work.mkdir()
        stage = self.work / "stage"
        daemon = stage / "usr/sbin/vdr-suite-daemon"
        daemon.parent.mkdir(parents=True)
        daemon.write_bytes(b"existing Suite daemon - not a binary used by test")
        daemon.chmod(0o755)
        frontend = stage / "usr/share/vdr-suite/web/frontend/index.html"
        frontend.parent.mkdir(parents=True)
        frontend.write_text("existing frontend\n", encoding="utf-8")
        frontend.chmod(0o644)
        for rel in ("backend-agent", "secrets", "secrets/series-artwork"):
            private(stage / "var/lib/vdr-suite" / rel)
        seal(stage)
        self.original_seal = (stage / ".vdr-suite-install-runtime-manifest.json").read_bytes()
        binaries = self.work / "build/addons"
        binaries.mkdir(parents=True)
        for name in MODULE.WORKERS:
            destination = binaries / name
            shutil.copyfile("/usr/bin/true", destination)
            destination.chmod(0o755)
        self.hash = hashlib.sha256(
            (binaries / MODULE.WORKERS[0]).read_bytes()
        ).hexdigest()
        (self.work / "candidate.txt").write_text("1" * 40 + "\n" + self.hash + "\n")
        self.target = Path(self.temp.name) / "stage-with-media-tools"

    def test_sealed_suite_composition_without_dpkg_or_compilation(self):
        MODULE.compose(self.work, self.target, self.hash)
        assert self.target.is_dir()
        dirs, files = verify_sealed_stage(self.target, load_manifest(self.target))
        assert "usr/sbin/vdr-suite-daemon" in files
        assert "usr/share/vdr-suite/addons/rectools/addon.json" in files
        assert "usr/libexec/vdr-suite/addons/rectools/addon.json" in files
        shared = self.target / "usr/share/vdr-suite/addons/rectools/addon.json"
        agent = self.target / "usr/libexec/vdr-suite/addons/rectools/addon.json"
        assert agent.read_bytes() == shared.read_bytes()
        assert mode(agent) == 0o644
        for binary in MODULE.WORKERS:
            installed = self.target / "usr/libexec/vdr-suite/addons/rectools" / binary
            assert installed.is_file() and mode(installed) == 0o755
            assert installed.read_bytes() == (
                self.work / "build/addons" / binary
            ).read_bytes()
        # This package-ready file list is part of the SAME sealed tree.
        record = MODULE.verify_media_tools_stage(self.target, self.hash)
        assert record["version"] == "0.1.0"
        assert record["package"] == "vdr-suite-addon-media-tools"
        assert record["prepared_source_sha"] == "1" * 40
        assert set(record["files"]) == set(MODULE.payload_roles())
        assert record["files"][
            "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import"
        ]["sha256"] == self.hash
        assert (self.target / MODULE.INVENTORY_REL).is_file()
        assert dirs["usr/share/vdr-suite"] == 0o755
        assert dirs["usr/share/vdr-suite/addons/rectools"] == 0o755
        assert dirs["var/lib/vdr-suite/backend-agent"] == 0o700
        assert not list(self.target.rglob("*.deb"))
        assert (self.work / "stage/.vdr-suite-install-runtime-manifest.json"
                ).read_bytes() == self.original_seal

    def test_verifier_rejects_changed_payload_even_if_suite_stage_is_resealed(self):
        MODULE.compose(self.work, self.target, self.hash)
        worker = self.target / MODULE.WORKER_REL / "vdr-suite-media-check"
        worker.write_bytes(worker.read_bytes() + b"changed")
        # Someone with stage write access can re-seal Suite content; the
        # add-on inventory still catches this independent payload mismatch.
        seal(self.target)
        with self.assertRaisesRegex(ValueError, "payload differs"):
            MODULE.verify_media_tools_stage(self.target, self.hash)

    def test_verifier_rejects_unreviewed_handler_digest(self):
        MODULE.compose(self.work, self.target, self.hash)
        with self.assertRaisesRegex(ValueError, "approval mismatch"):
            MODULE.verify_media_tools_stage(self.target, "0" * 64)

    def test_inventory_excludes_mutable_config_and_private_state(self):
        MODULE.compose(self.work, self.target, self.hash)
        record = json.loads((self.target / MODULE.INVENTORY_REL).read_text())
        paths = record["files"]
        assert all(p.startswith(("usr/share/vdr-suite/addons/rectools/",
                                 "usr/libexec/vdr-suite/addons/rectools/"))
                   for p in paths)
        assert not any(p.startswith(("etc/", "var/")) for p in paths)
        assert len(paths) == 7
        assert (self.work / "stage/.vdr-suite-install-runtime-manifest.json"
                ).read_bytes() == self.original_seal

    def test_wrong_reviewed_digest_fails_before_writes(self):
        with self.assertRaises(ValueError):
            MODULE.compose(self.work, self.target, "0" * 64)
        assert not self.target.exists()

    def test_existing_stage_is_never_overwritten(self):
        self.target.mkdir()
        with self.assertRaises(ValueError):
            MODULE.compose(self.work, self.target, self.hash)
        assert self.target.is_dir() and not list(self.target.iterdir())


    def test_mt_i2_read_only_first_install_and_verify_existing_payload(self):
        from media_tools_install_preflight import inspect
        MODULE.compose(self.work, self.target, self.hash)
        live = Path(self.temp.name) / "isolated-live"
        live.mkdir()
        report = inspect(self.target, live, self.hash, query_packages=False)
        assert report["install_state"] == "absent"
        assert report["approval"] == "NOT_VERIFIED"
        assert report["payload_counts"]["missing"] == 8
        assert not any(live.iterdir())  # no files/dirs created by preflight

        # Simulate a deploy into a synthetic root only, never /.
        payload = dict(MODULE.payload_roles())
        payload[MODULE.INVENTORY_REL.as_posix()] = "stage-inventory"
        for rel in payload:
            source = self.target / rel
            destination = live / rel
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)
            destination.chmod(mode(source))
        same = inspect(self.target, live, self.hash, query_packages=False)
        assert same["install_state"] == "identical_payload"
        assert same["payload_counts"]["match"] == 8

        worker = live / MODULE.WORKER_REL / "vdr-suite-media-check"
        worker.write_bytes(worker.read_bytes() + b"drift")
        drift = inspect(self.target, live, self.hash, query_packages=False)
        assert drift["install_state"] == "partial_or_different"
        assert drift["payload_counts"]["differs"] == 1

    def test_mt_i2_missing_payload_with_existing_approval_is_not_clean_install(self):
        from media_tools_install_preflight import inspect, APPROVAL
        MODULE.compose(self.work, self.target, self.hash)
        live = Path(self.temp.name) / "isolated-live"
        live.mkdir()
        approval = live / APPROVAL
        approval.parent.mkdir(parents=True)
        approval.write_text('{"existing":"must not overwrite"}\n')
        report = inspect(self.target, live, self.hash, query_packages=False)
        assert report["install_state"] == "existing_state_without_payload"
        assert report["auxiliary"]["agent_approval"] == "present_unverified"

    def test_mt_i2_refuses_symlinked_payload_or_ancestor(self):
        from media_tools_install_preflight import inspect, PreflightError
        MODULE.compose(self.work, self.target, self.hash)
        live = Path(self.temp.name) / "isolated-live"
        live.mkdir()
        (live / "usr").symlink_to(self.target / "usr", target_is_directory=True)
        with self.assertRaisesRegex(PreflightError, "unsafe installation ancestor"):
            inspect(self.target, live, self.hash, query_packages=False)

    def test_mt_i2_rejects_writable_installed_file(self):
        from media_tools_install_preflight import inspect, PreflightError
        MODULE.compose(self.work, self.target, self.hash)
        live = Path(self.temp.name) / "isolated-live"
        live.mkdir()
        path = live / MODULE.WORKER_REL / "vdr-suite-media-import"
        path.parent.mkdir(parents=True)
        shutil.copyfile(self.target / MODULE.WORKER_REL / "vdr-suite-media-import", path)
        path.chmod(0o777)
        with self.assertRaisesRegex(PreflightError, "writable installed file"):
            inspect(self.target, live, self.hash, query_packages=False)

    def test_mt_i2_rejects_unapproved_stage(self):
        from media_tools_install_preflight import inspect
        MODULE.compose(self.work, self.target, self.hash)
        live = Path(self.temp.name) / "isolated-live"
        live.mkdir()
        with self.assertRaisesRegex(ValueError, "approval mismatch"):
            inspect(self.target, live, "0" * 64, query_packages=False)

    def test_mt_i2_package_ownership_lookup_is_read_only(self):
        from media_tools_install_preflight import package_owners
        from unittest.mock import patch
        import subprocess
        target = "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import"
        response = subprocess.CompletedProcess(
            args=["dpkg-query", "-S", "/" + target], returncode=0,
            stdout="vdr-suite-addon-media-tools: /" + target + "\n", stderr="")
        with patch("media_tools_install_preflight.subprocess.run",
                   return_value=response) as call:
            owners = package_owners([target])
        assert owners[target].startswith("vdr-suite-addon-media-tools:")
        call.assert_called_once_with(
            ["dpkg-query", "-S", "/" + target],
            capture_output=True, text=True, check=False, timeout=5)

    def test_mt_i2_cli_check_is_read_only_and_verify_fails_closed(self):
        import subprocess
        import sys
        MODULE.compose(self.work, self.target, self.hash)
        live = Path(self.temp.name) / "isolated-live"
        live.mkdir()
        command = [sys.executable,
                   str(HERE / "media_tools_install_preflight.py"),
                   "--stage", str(self.target), "--live-root", str(live),
                   "--approved-handler-sha256", self.hash]
        before = sorted(live.rglob("*"))
        check = subprocess.run([*command[:2], "check", *command[2:]],
                               capture_output=True, text=True)
        assert check.returncode == 0, check.stderr
        assert '"install_state": "absent"' in check.stdout
        verify = subprocess.run([*command[:2], "verify", *command[2:]],
                                capture_output=True, text=True)
        assert verify.returncode != 0
        assert "installed Media Tools payload differs" in verify.stderr
        assert sorted(live.rglob("*")) == before


    def test_mt_i2_prepared_reuse_validates_all_existing_workers(self):
        from media_tools_install_preflight import prepared_state, PreflightError
        import subprocess
        target = self.work / "stage-with-media-tools"
        MODULE.compose(self.work, target, self.hash)
        before = (target / MODULE.INVENTORY_REL).read_bytes()
        state = prepared_state(self.work, self.hash)
        assert state["prepared"] == "verified_existing_artifacts"
        assert state["build"] == "REUSED_NO_COMPILATION"
        command = ["bash", str(HERE / "install_media_tools.sh"),
                   "prepare", str(self.work), self.hash]
        result = subprocess.run(command, capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        assert "MEDIA_TOOLS_PREPARED_REUSE=PASS" in result.stdout
        assert (target / MODULE.INVENTORY_REL).read_bytes() == before
        worker = self.work / "build/addons/vdr-suite-media-check"
        worker.write_bytes(worker.read_bytes() + b"modified")
        with self.assertRaisesRegex(PreflightError, "prepared worker diverges"):
            prepared_state(self.work, self.hash)



if __name__ == "__main__":
    unittest.main()

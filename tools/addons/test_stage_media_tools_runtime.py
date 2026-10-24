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


if __name__ == "__main__":
    unittest.main()

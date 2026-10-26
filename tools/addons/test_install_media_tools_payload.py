"""Portable Media Tools payload install regression on disposable synthetic roots."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import tempfile
import unittest
from unittest import mock

import install_media_tools_payload as installer
import stage_media_tools_runtime as media
from install_runtime_stage import seal

REPO = Path(__file__).resolve().parents[2]
ELF = Path("/usr/bin/true")

def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def write(root: Path, raw: str, payload: bytes, mode: int) -> Path:
    path = root / raw
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(payload)
    path.chmod(mode)
    return path

@unittest.skipUnless(ELF.is_file(), "local ELF fixture missing")
class PortableMediaToolsInstallTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="mt-portable-install-")
        self.addCleanup(self.temporary.cleanup)
        base = Path(self.temporary.name)
        self.stage = base / "stage"
        self.stage.mkdir()
        self.root = base / "fake-root"
        self.root.mkdir()
        (self.root / "root").mkdir(mode=0o700)
        write(self.root, "usr/sbin/vdr-suite-daemon", b"current-suite-not-old-stage", 0o755)
        write(self.root, "var/lib/vdr-suite/vdr-suite.db", b"DB untouched", 0o600)
        write(self.root, "etc/vdr-suite/backend-agent.conf", b"agent config untouched", 0o600)
        write(self.stage, "usr/sbin/vdr-suite-daemon", b"old-suite-stage-unused", 0o755)
        write(self.stage, "usr/share/vdr-suite/web/frontend/index.html",
              b"old-frontend-stage-unused", 0o644)
        for name in ("backend-agent", "secrets", "secrets/series-artwork"):
            private = self.stage / "var/lib/vdr-suite" / name
            private.mkdir(parents=True, exist_ok=True)
            private.chmod(0o700)
        metadata = REPO / "modules/rectools"
        for name in media.METADATA:
            target = self.stage / media.SHARED_REL / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(metadata / name, target)
            target.chmod(0o644)
        worker_bytes = ELF.read_bytes()
        for name in media.WORKERS:
            write(self.stage, (media.WORKER_REL / name).as_posix(),
                  worker_bytes, 0o755)
        shutil.copyfile(self.stage / media.SHARED_REL / "addon.json",
                        write(self.stage, (media.WORKER_REL / "addon.json").as_posix(),
                              b"", 0o644))
        self.approved = digest(self.stage / media.WORKER_REL / "vdr-suite-media-import")
        self.refresh_seal()

    def refresh_seal(self):
        paths = media.payload_roles()
        entries = {
            raw: {"role": role, "sha256": digest(self.stage / raw),
                  "size": (self.stage / raw).stat().st_size,
                  "mode": stat.S_IMODE((self.stage / raw).stat().st_mode)}
            for raw, role in paths.items()
        }
        identity = json.loads((self.stage / media.SHARED_REL / "addon.json").read_text())
        inventory = {
            "schema": 1, "module": "rectools", "package": identity["package"],
            "version": identity["version"], "prepared_source_sha": "a" * 40,
            "approved_handler_sha256": self.approved, "files": entries,
        }
        write(self.stage, media.INVENTORY_REL.as_posix(),
              (json.dumps(inventory, sort_keys=True) + "\n").encode(), 0o644)
        # The seal is the exact stage-file set; it does not authorize runtime code.
        (self.stage / ".vdr-suite-install-runtime-manifest.json").unlink(missing_ok=True)
        seal(self.stage)

    def test_first_install_only_addon_payload(self):
        result = installer.install(self.stage, self.root, self.approved)
        self.assertIn("MEDIA_TOOLS_INSTALL=PASS", result)
        self.assertIn("SERVICES=UNCHANGED", result)
        for path in (*media.payload_roles(), media.INVENTORY_REL.as_posix()):
            self.assertEqual(digest(self.root / path), digest(self.stage / path))
        self.assertEqual((self.root / "usr/sbin/vdr-suite-daemon").read_bytes(),
                         b"current-suite-not-old-stage")
        self.assertEqual((self.root / "var/lib/vdr-suite/vdr-suite.db").read_bytes(),
                         b"DB untouched")
        self.assertEqual((self.root / "etc/vdr-suite/backend-agent.conf").read_bytes(),
                         b"agent config untouched")
        self.assertFalse((self.root / "etc/vdr-suite/addon-approvals/rectools.json").exists())

    def test_repeat_install_is_idempotent_without_extra_backup(self):
        installer.install(self.stage, self.root, self.approved)
        before = list((self.root / "root").iterdir())
        result = installer.install(self.stage, self.root, self.approved)
        self.assertIn("MEDIA_TOOLS_INSTALL=ALREADY_CURRENT", result)
        self.assertEqual(before, list((self.root / "root").iterdir()))

    def test_old_identical_workers_adopt_missing_inventory(self):
        installer.install(self.stage, self.root, self.approved)
        (self.root / media.INVENTORY_REL).unlink()
        result = installer.install(self.stage, self.root, self.approved)
        self.assertIn("FILES_UPDATED=1", result)
        self.assertTrue((self.root / media.INVENTORY_REL).is_file())

    def test_approval_mismatch_fails_before_mutation(self):
        approval_path = write(self.root, "etc/vdr-suite/addon-approvals/rectools.json",
                              json.dumps({"id": "rectools",
                                          "package": "vdr-suite-addon-media-tools",
                                          "state": "approved", "sha256": "f" * 64}).encode(),
                              0o600)
        with self.assertRaisesRegex(installer.InstallError, "approval differs"):
            installer.install(self.stage, self.root, self.approved)
        self.assertTrue(approval_path.is_file())
        self.assertFalse((self.root / media.INVENTORY_REL).exists())

    def test_symlinked_payload_is_rejected(self):
        location = self.root / media.WORKER_REL / "vdr-suite-media-import"
        location.parent.mkdir(parents=True)
        location.symlink_to("/usr/bin/true")
        with self.assertRaisesRegex(installer.InstallError, "unsafe existing payload"):
            installer.install(self.stage, self.root, self.approved)

    def test_corrupt_stage_rejected(self):
        (self.stage / media.WORKER_REL / "vdr-suite-media-import").write_bytes(b"tampered")
        with self.assertRaises((ValueError, RuntimeError)):
            installer.install(self.stage, self.root, self.approved)
        self.assertFalse((self.root / media.INVENTORY_REL).exists())

    def test_quiescence_gate_is_explicit_and_os_independent(self):
        with self.assertRaisesRegex(installer.InstallError, "quiescence"):
            installer.install(self.stage, self.root, self.approved,
                              require_quiescence=True)
        result = installer.install(self.stage, self.root, self.approved,
                                   require_quiescence=True, quiesced=True)
        self.assertIn("MEDIA_TOOLS_INSTALL=PASS", result)

    def test_mid_deploy_failure_restores_previous_files(self):
        real = installer.copy_atomic
        call_count = [0]
        def failing_copy(source, target, mode):
            call_count[0] += 1
            if call_count[0] == 2:
                raise OSError("injected deployment failure")
            return real(source, target, mode)
        with mock.patch.object(installer, "copy_atomic", side_effect=failing_copy):
            with self.assertRaisesRegex(installer.InstallError,
                                        "RESTORED_PREVIOUS_FILES"):
                installer.install(self.stage, self.root, self.approved)
        for raw in media.payload_roles():
            self.assertFalse((self.root / raw).exists())
        self.assertEqual((self.root / "usr/sbin/vdr-suite-daemon").read_bytes(),
                         b"current-suite-not-old-stage")

class MediaToolsMakeRecipeTests(unittest.TestCase):
    def test_portable_make_targets_preserve_shell_substitution(self):
        source = (REPO / "mk/addons.mk").read_text(encoding="utf-8")
        # In a Make recipe, $(...) is a Make variable; $(...) is a
        # shell command substitution. Check both targets and their SHA pin.
        self.assertEqual(source.count('"$(id -u)"'), 2)
        self.assertEqual(source.count(
            '$(cat "$(MEDIA_TOOLS_WORKDIR)/source-commit.txt")'), 2)


if __name__ == "__main__":
    unittest.main()

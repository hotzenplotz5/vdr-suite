"""Disposable mixed-runtime snapshot and rollback regression; no real VDR writes."""
from __future__ import annotations

from pathlib import Path
import os
import shutil
import stat
import sys
import tempfile
import unittest
from unittest import mock

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from install_runtime_stage import (
    FRONTEND_ROOT, check, deploy, load_manifest, seal, verify_sealed_stage,
)
from media_tools_install_preflight import PreflightError
import media_tools_observed_live_update as observed


def write(root: Path, path: str, content: str, mode: int = 0o644):
    file = root / path
    file.parent.mkdir(parents=True, exist_ok=True)
    file.write_text(content, encoding="utf-8")
    file.chmod(mode)
    return file


class ObservedRuntimeSnapshotTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory(prefix="mt-observed-synthetic-")
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.new = self.root / "candidate"
        self.live = self.root / "live"
        self.old = self.root / "observed"
        self.new.mkdir()
        self.live.mkdir()

        write(self.new, "usr/sbin/vdr-suite-daemon", "new daemon", 0o755)
        write(self.new, "usr/sbin/vdr-suite-backend-agent", "new agent", 0o755)
        write(self.new, "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import",
              "new handler", 0o755)
        write(self.new, "usr/share/vdr-suite/web/frontend/index.html", "new frontend")
        write(self.new, "usr/share/vdr-suite/web/frontend/app.js", "new app")
        write(self.new, "etc/vdr-suite/backend-agent.conf", "default")

        for rel in ("var/lib/vdr-suite/backend-agent", "var/lib/vdr-suite/secrets",
                    "var/lib/vdr-suite/secrets/series-artwork"):
            (self.new / rel).mkdir(parents=True, exist_ok=True)
            (self.new / rel).chmod(0o700)
            (self.live / rel).mkdir(parents=True, exist_ok=True)
            (self.live / rel).chmod(0o700)
        seal(self.new)

        write(self.live, "usr/sbin/vdr-suite-daemon", "older daemon", 0o755)
        write(self.live, "usr/sbin/vdr-suite-backend-agent", "other agent build", 0o755)
        write(self.live, "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import",
              "hotfixed handler", 0o755)
        write(self.live, "usr/share/vdr-suite/web/frontend/index.html", "old frontend")
        write(self.live, "usr/share/vdr-suite/web/frontend/app.js", "local patch")
        write(self.live, "usr/share/vdr-suite/web/frontend/local-extra.js", "local extra")
        (self.live / FRONTEND_ROOT / "old-dir").mkdir()
        write(self.live, "etc/vdr-suite/backend-agent.conf", "private config")

    @staticmethod
    def allow_synthetic_owner(path, **kwargs):
        info = path.lstat()
        if path.is_symlink() or (
            kwargs.get("directory", False) != stat.S_ISDIR(info.st_mode)
        ):
            raise PreflightError("unacceptable synthetic file")
        return info

    def capture(self):
        with mock.patch.object(observed, "require_regular_owned",
                               side_effect=self.allow_synthetic_owner):
            observed.capture_observed_runtime(self.new, self.live, self.old)

    def test_mixed_live_files_restored_after_candidate(self):
        self.capture()
        verify_sealed_stage(self.old, load_manifest(self.old))
        check(self.old, self.live)
        assert (self.old / "usr/sbin/vdr-suite-daemon").read_text() == "older daemon"
        assert (self.old / FRONTEND_ROOT / "local-extra.js").read_text() == "local extra"
        assert (self.old / "etc/vdr-suite/backend-agent.conf").read_text() == "default"

        deploy(self.new, self.live)
        check(self.new, self.live)
        assert not (self.live / FRONTEND_ROOT / "local-extra.js").exists()
        assert (self.live / "etc/vdr-suite/backend-agent.conf").read_text() == "private config"

        deploy(self.old, self.live)
        check(self.old, self.live)
        assert (self.live / "usr/sbin/vdr-suite-backend-agent").read_text() == "other agent build"
        assert (self.live / FRONTEND_ROOT / "local-extra.js").read_text() == "local extra"
        assert (self.live / "etc/vdr-suite/backend-agent.conf").read_text() == "private config"

    def test_missing_old_file_cannot_be_invented(self):
        (self.live / "usr/sbin/vdr-suite-daemon").unlink()
        with self.assertRaises(FileNotFoundError):
            self.capture()
        assert not (self.live / "usr/sbin/vdr-suite-daemon").exists()

    def test_symlink_in_live_frontend_is_rejected(self):
        (self.live / FRONTEND_ROOT / "bad.js").symlink_to(
            self.live / FRONTEND_ROOT / "app.js"
        )
        with self.assertRaises(PreflightError):
            self.capture()

    def test_unchanged_files_share_storage_without_aliasing_changes(self):
        # The two private stages may share immutable unchanged files only;
        # replacing an old file must not modify the candidate's bytes.
        write(self.live, "usr/sbin/vdr-suite-backend-agent", "new agent", 0o755)
        self.capture()
        old_same = self.old / "usr/sbin/vdr-suite-backend-agent"
        new_same = self.new / "usr/sbin/vdr-suite-backend-agent"
        assert old_same.stat().st_ino == new_same.stat().st_ino

        old_changed = self.old / "usr/sbin/vdr-suite-daemon"
        new_changed = self.new / "usr/sbin/vdr-suite-daemon"
        assert old_changed.stat().st_ino != new_changed.stat().st_ino
        assert new_changed.read_text() == "new daemon"
        assert old_changed.read_text() == "older daemon"

    def test_input_stage_not_mutated(self):
        before = (self.new / "usr/sbin/vdr-suite-daemon").read_bytes()
        self.capture()
        assert (self.new / "usr/sbin/vdr-suite-daemon").read_bytes() == before


if __name__ == "__main__":
    unittest.main()

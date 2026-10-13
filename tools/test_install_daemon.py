#!/usr/bin/env python3
"""Isolated installer contract tests: never touch systemd, live binaries or DB."""

import contextlib
import argparse
import importlib.util
import io
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest import mock

SCRIPT = Path(__file__).with_name("install_daemon.py")
spec = importlib.util.spec_from_file_location("install_daemon_under_test", SCRIPT)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def git(root, *args):
    result = subprocess.run(["git", *args], cwd=root, capture_output=True, text=True,
                            check=True)
    return result.stdout.strip()


def commit(root, name, contents):
    file = root / name
    file.parent.mkdir(parents=True, exist_ok=True)
    file.write_text(contents)
    git(root, "add", name)
    git(root, "commit", "-qm", "fixture")
    return git(root, "rev-parse", "HEAD")


class InstallerTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="daemon-install-test-")
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name) / "repo"
        self.root.mkdir()
        git(self.root, "init", "-q")
        git(self.root, "config", "user.name", "Fixture")
        git(self.root, "config", "user.email", "fixture@example.invalid")
        (self.root / "tools").mkdir()
        shutil.copyfile(SCRIPT, self.root / "tools" / "install_daemon.py")
        (self.root / "mk").mkdir()
        (self.root / "mk" / "runtime-api-tests.mk").write_text("daemon:\n\t@true\n")
        git(self.root, "add", ".")
        git(self.root, "commit", "-qm", "bootstrap")
        self.old = git(self.root, "rev-parse", "HEAD")
        self.live = Path(self.tmp.name) / "old-daemon"
        self.live.write_bytes(b"ELF-fixture-git-" + self.old[:12].encode() + b"\n")

    def test_check_is_read_only_and_no_backup_for_unrelated_cpp(self):
        new = commit(self.root, "core/daemon/src/DeviceTrack.cpp", "// audio and subtitles\n")
        stream = io.StringIO()
        with contextlib.redirect_stdout(stream):
            self.assertEqual(mod.main(["check", "--source", str(self.root),
                                       "--sha", new, "--live", str(self.live)]), 0)
        self.assertIn("DB_BACKUP_DECISION=NOT_REQUIRED", stream.getvalue())
        self.assertIn("CHECK=PASS_NO_CHANGES", stream.getvalue())
        self.assertFalse((self.root / ".build").exists())

    def test_ddl_change_requires_review_and_never_auto_backup(self):
        new = commit(self.root, "core/vdr/src/FooRepository.cpp",
                     'void ensureSchema(){ execute("CREATE TABLE important (id INT)"); }\n')
        risk, flagged = mod.database_risk(self.root, self.old, new)
        self.assertEqual(risk, "REVIEW_REQUIRED")
        self.assertEqual(flagged, ["core/vdr/src/FooRepository.cpp"])

    def test_changed_sqlite_source_requires_review(self):
        new = commit(self.root, "core/sqlite/src/Database.cpp", "// changed database layer\n")
        risk, paths = mod.database_risk(self.root, self.old, new)
        self.assertEqual(risk, "REVIEW_REQUIRED")
        self.assertEqual(paths, ["core/sqlite/src/Database.cpp"])

    def test_unknown_binary_provenance_fails_closed_without_backup(self):
        new = commit(self.root, "core/daemon/src/DeviceTrack.cpp", "// no schema\n")
        self.live.write_bytes(b"no embedded git version")
        stderr = io.StringIO()
        with contextlib.redirect_stderr(stderr):
            self.assertEqual(mod.main(["check", "--source", str(self.root),
                                       "--sha", new, "--live", str(self.live)]), 1)
        self.assertIn("INSTALLED_SOURCE_ID_UNKNOWN", stderr.getvalue())
        self.assertFalse((self.root / ".build").exists())

    def test_build_seal_reuse_and_apply_without_seal_rejected(self):
        new = commit(self.root, "core/daemon/src/DeviceTrack.cpp", "// no schema\n")
        directory, binary, seal = mod.candidate(self.root, new)
        with self.assertRaisesRegex(mod.Rejected, "NO_SEALED_BUILD"):
            mod.verified_candidate(self.root, new)
        counter = Path(self.tmp.name) / "counts"
        fakebin = Path(self.tmp.name) / "bin"
        fakebin.mkdir()
        faker = fakebin / "make"
        faker.write_text("#!/bin/sh\n"
                         "for arg in \"$@\"; do case \"$arg\" in BUILD_DIR=*) out=$(printf '%s' \"$arg\" | cut -c11-);; esac; done\n"
                         "mkdir -p \"$out\"\n"
                         f"echo built >> '{counter}'\n"
                         f"printf '\\177ELFgit-{new[:12]} device_subtitle_delivery_invalid track-status /tracks /subtitles\\n' > \"$out/vdr-suite-daemon\"\n"
                         "chmod 755 \"$out/vdr-suite-daemon\"\n")
        faker.chmod(0o755)
        with mock.patch.object(mod, "free_budget"):
            with mock.patch.dict(os.environ, {"PATH": str(fakebin) + ":" + os.environ["PATH"]}):
                mod.build(self.root, new)
                mod.build(self.root, new)
        self.assertEqual(counter.read_text().count("built"), 1)
        self.assertEqual(mod.verified_candidate(self.root, new), binary)
        self.assertTrue(seal.is_file())
        binary.write_bytes(binary.read_bytes() + b"tamper")
        with self.assertRaisesRegex(mod.Rejected, "BUILD_SEAL_MISMATCH"):
            mod.verified_candidate(self.root, new)

    def test_interrupted_build_can_be_recovered_without_recompile(self):
        new = commit(self.root, "core/daemon/src/DeviceTrack.cpp", "// no schema\n")
        directory, binary, seal = mod.candidate(self.root, new)
        directory.mkdir(parents=True)
        binary.write_bytes(b"\x7fELFgit-" + new[:12].encode() + b" " + b" ".join(mod.FEATURES))
        binary.chmod(0o755)
        with mock.patch.object(mod, "free_budget"):
            with mock.patch.object(mod.subprocess, "run", side_effect=AssertionError("make must not run")):
                mod.build(self.root, new)
        self.assertTrue(seal.is_file())
        self.assertEqual(mod.verified_candidate(self.root, new), binary)

    def _sealed_binary(self, new):
        directory, binary, seal = mod.candidate(self.root, new)
        directory.mkdir(parents=True)
        binary.write_bytes(b"\x7fELFgit-" + new[:12].encode() + b" " + b" ".join(mod.FEATURES))
        binary.chmod(0o755)
        seal.write_text(__import__("json").dumps({"schema": 1, "source": new,
                                                     "binary_sha256": mod.digest(binary)}))
        return binary

    def test_apply_no_schema_change_never_backs_up_database(self):
        new = commit(self.root, "core/daemon/src/DeviceTrack.cpp", "// no schema\n")
        binary = self._sealed_binary(new)
        old_bytes = self.live.read_bytes()
        args = argparse.Namespace(confirm_apply=True, live=self.live,
                                  service="vdr-suite-daemon.service", db_review=None,
                                  db_review_reason=None, database=None)
        original_run = mod.run
        with (mock.patch.object(mod, "LIVE_PATH", self.live.resolve()),
              mock.patch.object(mod, "INSTALL_LOCK", Path(self.tmp.name) / "lock"),
              mock.patch.object(mod, "free_budget"),
              mock.patch.object(mod, "service_bin_identity"),
              mock.patch.object(mod, "backup_database_offline", side_effect=AssertionError("DB backup forbidden")),
              mock.patch.object(mod, "run", side_effect=lambda *a, **k: "" if a[0] == "systemctl" else original_run(*a, **k)),
              mock.patch.object(mod.time, "sleep"),
              mock.patch.object(mod.os, "geteuid", return_value=0)):
            mod.apply(self.root, new, args, self.old, "NOT_REQUIRED")
        self.assertEqual(self.live.read_bytes(), binary.read_bytes())
        self.assertEqual((binary.parent / "previous-daemon").read_bytes(), old_bytes)
        self.assertTrue(binary.is_file())

    def test_apply_failed_start_restores_old_daemon_without_db_backup(self):
        new = commit(self.root, "core/daemon/src/DeviceTrack.cpp", "// no schema\n")
        binary = self._sealed_binary(new)
        old_bytes = self.live.read_bytes()
        args = argparse.Namespace(confirm_apply=True, live=self.live,
                                  service="vdr-suite-daemon.service", db_review=None,
                                  db_review_reason=None, database=None)
        counts = {"start": 0}
        original_run = mod.run
        def fake_run(*cmd, **_kwargs):
            if cmd[:2] == ("systemctl", "start"):
                counts["start"] += 1
                if counts["start"] == 1:
                    raise mod.Rejected("INJECTED_START_FAILURE")
            return "" if cmd[0] == "systemctl" else original_run(*cmd, **_kwargs)
        with (mock.patch.object(mod, "LIVE_PATH", self.live.resolve()),
              mock.patch.object(mod, "INSTALL_LOCK", Path(self.tmp.name) / "lock"),
              mock.patch.object(mod, "free_budget"),
              mock.patch.object(mod, "service_bin_identity"),
              mock.patch.object(mod, "backup_database_offline", side_effect=AssertionError("DB backup forbidden")),
              mock.patch.object(mod, "run", side_effect=fake_run),
              mock.patch.object(mod.time, "sleep"),
              mock.patch.object(mod.os, "geteuid", return_value=0)):
            with self.assertRaisesRegex(mod.Rejected, "INJECTED_START_FAILURE"):
                mod.apply(self.root, new, args, self.old, "NOT_REQUIRED")
        self.assertEqual(self.live.read_bytes(), old_bytes)
        self.assertTrue(binary.is_file())
        self.assertEqual(counts["start"], 2)

    def test_untrusted_source_changes_rejected(self):
        new = commit(self.root, "core/daemon/src/DeviceTrack.cpp", "// no schema\n")
        (self.root / "core/daemon/src/DeviceTrack.cpp").write_text("dirty")
        with self.assertRaisesRegex(mod.Rejected, "SOURCE_TRACKED_DIRTY"):
            mod.verify_source(self.root, new)


if __name__ == "__main__":
    unittest.main()

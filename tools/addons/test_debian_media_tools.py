"""Isolated MT.I4.1 debhelper/Stage integration regressions (no live dpkg use)."""
from __future__ import annotations

from pathlib import Path
import shutil
import stat
import subprocess
import tarfile
import tempfile
import unittest
from unittest.mock import patch

from debian_media_tools import build_package_payload, verify_package_payload
from build_media_tools_cpp_deb import stage_package_paths
from install_runtime_stage import seal
from stage_media_tools_runtime import WORKERS

ROOT = Path(__file__).resolve().parents[2]


@unittest.skipUnless(Path("/usr/bin/true").is_file(), "ELF fixture unavailable")
class DebianMediaToolsTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory(prefix="mt-i41-debian-")
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.base = self.root / "base"
        for rel, data, mode in (
            ("usr/sbin/vdr-suite-daemon", b"isolated daemon fixture", 0o755),
            ("usr/share/vdr-suite/web/frontend/index.html", b"test page", 0o644),
        ):
            target = self.base / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            target.chmod(mode)
        for rel in ("backend-agent", "secrets", "secrets/series-artwork"):
            private = self.base / "var/lib/vdr-suite" / rel
            private.mkdir(parents=True, exist_ok=True)
            private.chmod(0o700)
        seal(self.base)
        self.workers = self.root / "workers"
        self.workers.mkdir()
        for name in WORKERS:
            target = self.workers / name
            shutil.copyfile("/usr/bin/true", target)
            target.chmod(0o755)
        self.package = self.root / "vdr-suite-addon-media-tools"
        self.preinst = self.root / "vdr-suite-addon-media-tools.preinst"

    def test_exact_sealed_payload_from_existing_workers(self):
        with patch("debian_media_tools.source_commit", return_value="1" * 40):
            result = build_package_payload(self.base, self.workers,
                                           self.package, self.preinst)
        paths = set(stage_package_paths())
        actual = {p.relative_to(self.package).as_posix()
                  for p in self.package.rglob("*") if p.is_file()}
        self.assertEqual(actual, paths)
        self.assertEqual(len(paths), 8)
        self.assertEqual(result["prepared_source_sha"], "1" * 40)
        for rel in paths:
            path = self.package / rel
            self.assertEqual(stat.S_IMODE(path.stat().st_mode),
                             0o755 if path.name in WORKERS else 0o644)
        self.assertFalse(any(p.startswith(("etc/", "var/")) for p in paths))
        self.assertEqual(verify_package_payload(self.package), 8)
        # Debian legal package documentation is separate from the eight
        # checksum-pinned add-on files.
        docs = self.package / "usr/share/doc/vdr-suite-addon-media-tools/copyright"
        docs.parent.mkdir(parents=True)
        docs.write_text("standalone legal metadata")
        self.assertEqual(verify_package_payload(self.package), 8)
        self.assertTrue(self.preinst.is_file())
        self.assertEqual(subprocess.run(["/bin/sh", "-n", str(self.preinst)]).returncode, 0)
        self.assertIn("manually installed path", self.preinst.read_text())
        self.assertFalse(list(self.package.rglob("*.service")))

    def test_sealed_worker_byte_change_is_detected(self):
        with patch("debian_media_tools.source_commit", return_value="1" * 40):
            build_package_payload(self.base, self.workers, self.package, self.preinst)
        worker = self.package / "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import"
        with worker.open("ab") as handle:
            handle.write(b"mutated by packaging step")
        with self.assertRaisesRegex(ValueError, "changed sealed stage"):
            verify_package_payload(self.package)

    def test_existing_binary_root_is_not_overwritten(self):
        self.package.mkdir()
        existing = self.package / "keep"
        existing.write_bytes(b"operator owned")
        with patch("debian_media_tools.source_commit", return_value="1" * 40):
            with self.assertRaisesRegex(ValueError, "not empty"):
                build_package_payload(self.base, self.workers, self.package,
                                      self.preinst)
        self.assertEqual(existing.read_bytes(), b"operator owned")

    def test_source_commit_export_substitution(self):
        # Existing sourcepackager uses git archive, so verify the archive
        # contains the true commit, not the literal $Format placeholder.
        if not shutil.which("git") or not (ROOT / ".git").exists():
            self.skipTest("Git checkout unavailable in source package")
        sha = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT,
                                      text=True).strip()
        with tempfile.TemporaryFile() as archive:
            subprocess.run(["git", "archive", "--format=tar", "HEAD",
                            ".vdr-suite-source-commit"], cwd=ROOT,
                           stdout=archive, check=True)
            archive.seek(0)
            with tarfile.open(fileobj=archive) as files:
                self.assertEqual(files.extractfile(".vdr-suite-source-commit")
                                 .read().decode().strip(), sha)


if __name__ == "__main__":
    unittest.main()

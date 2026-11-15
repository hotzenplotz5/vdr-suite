"""MT.I4.2: real dpkg operations in an isolated, disposable test root.

The normal preinst algorithm is reused with its existing test_root argument;
only its dpkg-query executable is swapped to inspect the disposable dpkg DB.
No host /usr, /etc, /var, services, packages or VDR recordings are touched.
"""
from __future__ import annotations

import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from build_media_tools_cpp_deb import collision_preinst, stage_package_paths
from debian_media_tools import build_package_payload
from install_runtime_stage import seal
from stage_media_tools_runtime import WORKERS


@unittest.skipUnless(
    all(shutil.which(name) for name in ("dpkg", "dpkg-deb", "dpkg-query"))
    and Path("/usr/bin/true").is_file(),
    "dpkg/deb or ELF fixture unavailable",
)
class MediaToolsDpkgLifecycleTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="mt-i42-dpkg-")
        self.addCleanup(temporary.cleanup)
        self.tmp = Path(temporary.name)
        self.host = self.tmp / "synthetic-root"
        self.db = self.host / "var/lib/dpkg"
        self.db.mkdir(parents=True)
        (self.db / "status").write_text("", encoding="ascii")
        self.base = self.tmp / "base-stage"
        for rel, data, mode in (
            ("usr/sbin/vdr-suite-daemon", b"fixture daemon", 0o755),
            ("usr/share/vdr-suite/web/frontend/index.html", b"fixture page", 0o644),
        ):
            file = self.base / rel
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(data)
            file.chmod(mode)
        for rel in ("backend-agent", "secrets", "secrets/series-artwork"):
            location = self.base / "var/lib/vdr-suite" / rel
            location.mkdir(parents=True, exist_ok=True)
            location.chmod(0o700)
        seal(self.base)
        self.workers = self.tmp / "prebuilt-workers"
        self.workers.mkdir()
        for name in WORKERS:
            file = self.workers / name
            shutil.copyfile("/usr/bin/true", file)
            file.chmod(0o755)
        self.query = self.tmp / "test-only-dpkg-query"
        self.query.write_text('''#!/bin/sh
set -eu
[ "${1:-}" = '-S' ] || exit 2
if [ -n "${MOCK_OWNER:-}" ]; then
  printf '%s: %s\\n' "$MOCK_OWNER" "$2"
  exit 0
fi
relative=${2#"$MT_TEST_ROOT"}
[ "$relative" != "$2" ] || exit 2
owner=$(/usr/bin/dpkg-query --admindir="$MT_TEST_DB" -S "$relative" 2>/dev/null) || exit 1
owner=${owner%%:*}
printf '%s: %s\\n' "$owner" "$2"
''', encoding="utf-8")
        self.query.chmod(0o755)
        self.preserved = {}
        for rel in (
            "etc/vdr-suite/backend-agent.conf",
            "etc/vdr-suite/addon-approvals/rectools.json",
            "var/lib/vdr-suite/vdr-suite.db",
            "srv/vdr/video.00/holiday/001.vdr",
        ):
            target = self.host / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            content = ("operator-state:" + rel).encode("utf-8")
            target.write_bytes(content)
            self.preserved[rel] = content

    def package(self, number: int) -> Path:
        stage_root = self.tmp / f"pkg{number}"
        preinst = self.tmp / f"preinst{number}"
        with patch("debian_media_tools.source_commit", return_value="1" * 40):
            build_package_payload(self.base, self.workers, stage_root, preinst)
        control = stage_root / "DEBIAN"
        control.mkdir()
        arch = subprocess.check_output(["dpkg", "--print-architecture"], text=True).strip()
        (control / "control").write_text(
            "Package: vdr-suite-addon-media-tools\n"
            + f"Version: 0.1.0~fixture{number}\nArchitecture: {arch}\n"
            + "Maintainer: Test Fixture <fixture@example.org>\n"
            + "Description: Synthetic Media Tools dpkg lifecycle fixture\n",
            encoding="utf-8",
        )
        guard = collision_preinst(stage_package_paths(), test_root=self.host)
        original = 'dpkg-query -S "$target"'
        self.assertIn(original, guard)
        # dpkg resets PATH during maintainer-script execution. Only in the
        # synthetic test package, use the isolated ownership database wrapper.
        guard = guard.replace(original, shlex.quote(str(self.query)) + ' -S "$target"')
        script = control / "preinst"
        script.write_text(guard, encoding="utf-8")
        script.chmod(0o755)
        self.assertEqual(subprocess.run(["sh", "-n", str(script)]).returncode, 0)
        result = self.tmp / f"media-tools-{number}.deb"
        subprocess.run(["dpkg-deb", "--build", "--root-owner-group",
                        str(stage_root), str(result)], check=True,
                       capture_output=True, text=True)
        return result

    def dpkg(self, *args: str, owner: str = "") -> subprocess.CompletedProcess:
        env = {**os.environ, "MT_TEST_DB": str(self.db),
               "MT_TEST_ROOT": str(self.host), "MOCK_OWNER": owner}
        return subprocess.run(
            ["dpkg", "--admindir=" + str(self.db),
             "--instdir=" + str(self.host),
             "--log=" + str(self.tmp / "dpkg-fixture.log"),
             "--force-script-chrootless", "--force-not-root", *args],
            env=env, text=True, capture_output=True, timeout=30,
        )

    def check_preserved(self):
        for rel, expected in self.preserved.items():
            with self.subTest(path=rel):
                self.assertEqual((self.host / rel).read_bytes(), expected)

    def test_manual_collision_upgrade_remove_and_purge(self):
        pkg1 = self.package(1)
        worker = self.host / "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import"
        worker.parent.mkdir(parents=True, exist_ok=True)
        worker.write_bytes(b"manual operator worker")
        refused = self.dpkg("-i", str(pkg1))
        self.assertNotEqual(refused.returncode, 0, refused.stdout)
        self.assertIn("MEDIA_TOOLS_DPKG=BLOCKED", refused.stderr)
        self.assertEqual(worker.read_bytes(), b"manual operator worker")
        self.check_preserved()
        worker.unlink()
        first = self.dpkg("-i", str(pkg1))
        self.assertEqual(first.returncode, 0, first.stderr)
        worker1 = worker.read_bytes()
        self.assertEqual(worker1, (self.workers / WORKERS[0]).read_bytes())
        self.check_preserved()

        with (self.workers / WORKERS[0]).open("ab") as stream:
            stream.write(b"second isolated worker revision")
        pkg2 = self.package(2)
        foreign = self.dpkg("-i", str(pkg2), owner="other-package")
        self.assertNotEqual(foreign.returncode, 0)
        self.assertIn("MEDIA_TOOLS_DPKG=BLOCKED", foreign.stderr)
        self.assertEqual(worker.read_bytes(), worker1)
        upgraded = self.dpkg("-i", str(pkg2))
        self.assertEqual(upgraded.returncode, 0, upgraded.stderr)
        self.assertEqual(worker.read_bytes(), (self.workers / WORKERS[0]).read_bytes())
        self.check_preserved()

        removed = self.dpkg("--remove", "vdr-suite-addon-media-tools")
        self.assertEqual(removed.returncode, 0, removed.stderr)
        for rel in stage_package_paths():
            self.assertFalse((self.host / rel).exists(), rel)
        self.check_preserved()
        purged = self.dpkg("--purge", "vdr-suite-addon-media-tools")
        self.assertEqual(purged.returncode, 0, purged.stderr)
        self.check_preserved()

    def test_symlinked_target_parent_refused_before_unpack(self):
        pkg = self.package(1)
        parent = self.host / "usr/libexec/vdr-suite/addons/rectools"
        outside = self.tmp / "outside"
        outside.mkdir()
        parent.parent.mkdir(parents=True)
        parent.symlink_to(outside, target_is_directory=True)
        refused = self.dpkg("-i", str(pkg))
        self.assertNotEqual(refused.returncode, 0)
        self.assertIn("symlinked package directory", refused.stderr)
        self.assertFalse(list(outside.iterdir()))
        self.check_preserved()


if __name__ == "__main__":
    unittest.main()

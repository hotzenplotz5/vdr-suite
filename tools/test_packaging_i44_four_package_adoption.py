"""MT.I4.4b: four-package manual adoption/rollback rehearsal, private dpkg root ONLY.

Use the production P8 preinst generators and actual isolated dpkg operations.
All data, programs, and operator settings are synthetic fixture bytes.
This does NOT implement or authorize manual takeover on the real yaVDR.
"""
from __future__ import annotations

import os
from pathlib import Path
import shlex
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent / "addons"))
from build_media_tools_cpp_deb import collision_preinst, stage_package_paths
from packaging_p8_preinst import render

PACKAGES = (
    "vdr-suite-addon-media-tools",
    "vdr-suite-daemon",
    "vdr-suite-backend-agent",
    "vdr-plugin-suitebridge",
)
PAYLOAD = {
    "vdr-suite-daemon": (
        "usr/sbin/vdr-suite-daemon",
        "usr/share/vdr-suite/web/frontend/index.html",
        "usr/lib/systemd/system/vdr-suite-daemon.service",
        "etc/default/vdr-suite-daemon",
        "etc/nginx/snippets/vdr-suite.conf",
    ),
    "vdr-suite-backend-agent": (
        "usr/sbin/vdr-suite-backend-agent",
        "usr/lib/systemd/system/vdr-suite-backend-agent.service",
        "etc/vdr-suite/backend-agent.conf",
    ),
    "vdr-plugin-suitebridge": (
        "usr/lib/vdr/plugins/libvdr-suitebridge.so.14",
        "usr/lib/tmpfiles.d/vdr-suite-live.conf",
    ),
    "vdr-suite-addon-media-tools": stage_package_paths(),
}
# These operator-managed files are NOT owned by any fixture .deb. Real
# SQLite/WAL consistency and operation-drain are explicitly out of scope.
STATE = (
    "var/lib/vdr-suite/vdr-suite.db",
    "var/lib/vdr-suite/vdr-suite.db-wal",
    "var/lib/vdr-suite/vdr-suite.db-shm",
    "var/lib/vdr-suite/backend-agent/secrets.fixture",
    "etc/vdr-suite/addon-approvals/rectools.json",
    "etc/vdr-suite/addon-approvals/rectools.import-running.lock",
    "etc/vdr-suite/addon-sources/media-import.tsv",
    "etc/systemd/system/vdr-suite-daemon.service.d/recording-artwork.conf",
    "etc/systemd/system/vdr-suite-backend-agent.service.d/20-media-tools.conf",
    "srv/vdr/video.00/holiday/001.vdr",
)


@unittest.skipUnless(
    all(shutil.which(name) for name in ("dpkg", "dpkg-deb", "dpkg-query"))
    and Path("/usr/bin/dpkg-query").is_file(),
    "requires native dpkg tools for an isolated fixture",
)
class FourPackageManualAdoptionTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="mt-i44-four-deb-")
        self.addCleanup(temporary.cleanup)
        self.tmp = Path(temporary.name)
        self.root = self.tmp / "synthetic-root"
        self.db = self.root / "var/lib/dpkg"
        self.db.mkdir(parents=True)
        (self.db / "status").write_text("", encoding="ascii")
        self.query = self.tmp / "isolated-dpkg-query"
        self.query.write_text(
            """#!/bin/sh
[ "$1" = -S ] || exit 2
if [ "$2" = -- ]; then target=$3; else target=$2; fi
case "$target" in
  "$MT_I44_ROOT"/*) ;;
  *) exit 2 ;;
esac
relative=$(printf '%s' "$target" | sed "s|^$MT_I44_ROOT||")
owner=$(/usr/bin/dpkg-query --admindir="$MT_I44_DB" -S "$relative" 2>/dev/null) || exit 1
owner=$(printf '%s' "$owner" | head -n 1)
owner=${owner%%:*}
printf '%s: %s\n' "$owner" "$target"
""", encoding="utf-8")
        self.query.chmod(0o755)

    def put(self, rel, data, mode=0o644):
        path = self.root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        path.chmod(mode)
        return path

    def run_dpkg(self, *args):
        env = dict(os.environ, MT_I44_ROOT=str(self.root),
                   MT_I44_DB=str(self.db))
        return subprocess.run(
            ["dpkg", "--admindir=" + str(self.db),
             "--instdir=" + str(self.root),
             "--log=" + str(self.tmp / "dpkg.log"),
             "--force-script-chrootless", "--force-not-root", *args],
            text=True, capture_output=True, env=env, timeout=60)

    @staticmethod
    def payload_data(package, rel):
        return ("i44-fixture:" + package + ":" + rel).encode("ascii")

    @staticmethod
    def payload_mode(rel):
        return 0o755 if ("/sbin/" in rel or "/plugins/" in rel or
                         "/libexec/" in rel and not rel.endswith(".json")) else 0o644

    def package(self, name):
        root = self.tmp / ("pkg-" + name)
        for rel in PAYLOAD[name]:
            file = root / rel
            file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(self.payload_data(name, rel))
            file.chmod(self.payload_mode(rel))
        meta = root / "DEBIAN"
        meta.mkdir()
        architecture = subprocess.check_output(
            ["dpkg", "--print-architecture"], text=True).strip()
        (meta / "control").write_text(
            "Package: " + name + "\n"
            "Version: 0.1.0~mt-i44fixture1\n"
            "Architecture: " + architecture + "\n"
            "Maintainer: Disposable Fixture <fixture@example.invalid>\n"
            "Description: Local four-package migration fixture only\n",
            encoding="ascii",
        )
        if name == "vdr-suite-addon-media-tools":
            guard = collision_preinst(stage_package_paths(), test_root=self.root)
            old = 'dpkg-query -S "$target"'
            self.assertIn(old, guard)
            guard = guard.replace(old, shlex.quote(str(self.query)) + ' -S "$target"')
        else:
            guard = render(name, test_root=str(self.root))
            old = "dpkg-query -S --"
            self.assertIn(old, guard)
            guard = guard.replace(old, shlex.quote(str(self.query)) + " -S --")
        preinst = meta / "preinst"
        preinst.write_text(guard, encoding="utf-8")
        preinst.chmod(0o755)
        syntax = subprocess.run(["/bin/sh", "-n", str(preinst)],
                                capture_output=True, text=True)
        self.assertEqual(syntax.returncode, 0, syntax.stderr)
        deb = self.tmp / (name + ".deb")
        subprocess.run(["dpkg-deb", "--build", "--root-owner-group",
                        str(root), str(deb)],
                       check=True, capture_output=True, text=True, timeout=30)
        return deb

    @staticmethod
    def snapshot_file(path):
        info = path.lstat()
        assert stat.S_ISREG(info.st_mode)
        return path.read_bytes(), stat.S_IMODE(info.st_mode)

    def snapshot(self, paths):
        return {rel: self.snapshot_file(self.root / rel) for rel in paths}

    def assert_snapshot(self, expected):
        for rel, (data, mode) in expected.items():
            with self.subTest(path=rel):
                path = self.root / rel
                self.assertTrue(path.is_file(), rel)
                self.assertFalse(path.is_symlink(), rel)
                self.assertEqual(self.snapshot_file(path), (data, mode), rel)

    def test_manual_four_package_refusal_then_fixture_adoption_and_rollback(self):
        debs = {name: self.package(name) for name in PACKAGES}
        all_paths = {
            rel for paths in PAYLOAD.values() for rel in paths
        }
        self.assertEqual(len(all_paths), sum(len(v) for v in PAYLOAD.values()))
        for name, paths in PAYLOAD.items():
            for rel in paths:
                self.put(rel, self.payload_data(name, rel),
                         self.payload_mode(rel))
        for rel in STATE:
            self.put(rel, b"" if rel.endswith(".lock") else
                     ("operator-state:" + rel).encode("ascii"))
        old_payload = self.snapshot(all_paths)
        old_state = self.snapshot(STATE)

        # Every existing unowned package payload is refused before unpack.
        for name in PACKAGES:
            attempt = self.run_dpkg("-i", str(debs[name]))
            self.assertNotEqual(attempt.returncode, 0, (name, attempt.stdout))
            self.assertIn("BLOCKED", attempt.stderr, (name, attempt.stderr))
            self.assert_snapshot(old_payload)
            self.assert_snapshot(old_state)
        print("MT_I44_FOUR_MANUAL_COLLISIONS=PASS")

        # Simulate an independently reviewed operator change only inside
        # TemporaryDirectory. This is NOT the process for a real host.
        # Temporarily park state and service overrides because P8 forbids
        # first-installation with an unmanaged live database/drop-in.
        parked = self.tmp / "parked-operator-state"
        parked.mkdir()
        for rel in (
            "var/lib/vdr-suite",
            "etc/systemd/system/vdr-suite-daemon.service.d",
            "etc/systemd/system/vdr-suite-backend-agent.service.d",
        ):
            target = self.root / rel
            saved = parked / rel
            saved.parent.mkdir(parents=True, exist_ok=True)
            shutil.move(str(target), str(saved))
        for rel in all_paths:
            (self.root / rel).unlink()

        # APT clean-host co-install was proved separately under Noble and
        # Resolute. Here we force the harder ordering: add-on before daemon
        # to exercise P8's shared /usr/share/vdr-suite ownership path.
        for name in PACKAGES:
            outcome = self.run_dpkg("-i", str(debs[name]))
            self.assertEqual(outcome.returncode, 0, (name, outcome.stderr))
        for rel in (
            "var/lib/vdr-suite",
            "etc/systemd/system/vdr-suite-daemon.service.d",
            "etc/systemd/system/vdr-suite-backend-agent.service.d",
        ):
            saved = parked / rel
            target = self.root / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.move(str(saved), str(target))
        self.assert_snapshot(old_payload)
        self.assert_snapshot(old_state)
        for name in PACKAGES:
            state = subprocess.check_output(
                ["dpkg-query", "--admindir=" + str(self.db), "-W",
                 "-f=" + "${Status}", name], text=True).strip()
            self.assertEqual(state, "install ok installed", name)
            for rel in PAYLOAD[name]:
                owned = subprocess.check_output(
                    ["dpkg-query", "--admindir=" + str(self.db), "-S",
                     "/" + rel], text=True).strip()
                self.assertEqual(owned, name + ": /" + rel)
        print("MT_I44_FOUR_SYNTHETIC_DPKG_ADOPTION=PASS")

        # Purge the four synthetic packages; independent operator state
        # survives. Then restore the saved manual payload exactly.
        removed = self.run_dpkg("--purge", *reversed(PACKAGES))
        self.assertEqual(removed.returncode, 0, removed.stderr)
        for rel in all_paths:
            self.assertFalse((self.root / rel).exists(), rel)
        self.assert_snapshot(old_state)
        for rel, (data, mode) in old_payload.items():
            self.put(rel, data, mode)
        self.assert_snapshot(old_payload)
        self.assert_snapshot(old_state)
        audit = self.run_dpkg("--audit")
        self.assertEqual(audit.returncode, 0, audit.stderr)
        self.assertEqual(audit.stdout.strip(), "")
        print("MT_I44_FOUR_SYNTHETIC_MANUAL_ROLLBACK=PASS")


if __name__ == "__main__":
    unittest.main()

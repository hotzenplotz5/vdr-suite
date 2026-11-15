#!/usr/bin/env python3
"""MT.I4.3: offline PPA preflight contract tests; no PPA calls or installations.

The mocked apt-get intentionally fails at the first simulation call. Therefore
a test success cannot claim that a real PPA package was installed or accepted.
"""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).with_name("check_published_media_tools_ppa.sh")
VERSION = "0.14.1-1~exp2~ppa2+209901010101~ubuntu24.04.1"

APT_CACHE = """#!/usr/bin/env python3
import os
version = os.environ['MOCK_VERSION']
candidate = '(none)' if os.environ.get('MOCK_MISSING') == '1' else version
origin = 'ppa.launchpadcontent.net/hotzenplotz5/vdr-suite/ubuntu'
if os.environ.get('MOCK_WRONG_ORIGIN') == '1':
    origin = 'example.invalid/not-the-vdr-suite-ppa'
print('vdr-suite-addon-media-tools:')
print('  Installed: (none)')
print('  Candidate: ' + candidate)
print('  Version table:')
print('     ' + version + ' 500')
print('        500 https://' + origin + ' noble/main amd64 Packages')
"""
DPKG_QUERY = """#!/usr/bin/env python3
import os
import sys
pkg = sys.argv[-1]
fmt = ' '.join(sys.argv[:-1])
if 'Provides' in fmt:
    print('vdr-abi-2.8.3-0yavdr')
elif pkg == 'vdr':
    print('2.8.3-0yavdr0~noble')
elif pkg == 'vdr-suite-daemon' and os.environ.get('MOCK_WRONG_VERSION') == '1':
    print('0.14.1-1~exp2~ppa2+209901010100~ubuntu24.04.1', end='')
else:
    print(os.environ['MOCK_VERSION'], end='')
"""
APT_GET = """#!/usr/bin/env python3
import os
import sys
with open(os.environ['MOCK_APT_CALLS'], 'a', encoding='utf-8') as fd:
    fd.write(' '.join(sys.argv[1:]) + '\\n')
# Deliberately fail before any installation, even if invoked incorrectly.
sys.exit(79)
"""


class PpaPreflightContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        os_release = Path("/etc/os-release").read_text(encoding="utf-8")
        if 'VERSION_CODENAME=noble' not in os_release:
            raise unittest.SkipTest("preflight shell fixture runs on Ubuntu 24.04 Noble CI")

    def run_mock(self, **switches):
        with tempfile.TemporaryDirectory(prefix="mt-i43-preflight-") as tmp:
            root = Path(tmp)
            bindir = root / "bin"
            bindir.mkdir()
            for filename, body in (
                ("apt-cache", APT_CACHE),
                ("dpkg-query", DPKG_QUERY),
                ("apt-get", APT_GET),
            ):
                target = bindir / filename
                target.write_text(body, encoding="utf-8")
                target.chmod(0o755)
            calls = root / "apt-calls.txt"
            env = os.environ.copy()
            env.update({
                "PATH": str(bindir) + os.pathsep + env["PATH"],
                "MOCK_VERSION": VERSION,
                "MOCK_APT_CALLS": str(calls),
            })
            env.update(switches)
            result = subprocess.run(
                ["bash", str(SCRIPT), "noble"],
                cwd=SCRIPT.parent.parent.parent,
                env=env,
                text=True,
                capture_output=True,
                check=False,
            )
            return result, calls.read_text(encoding="utf-8") if calls.exists() else ""

    def test_consistent_candidate_reaches_simulation_but_never_installs(self):
        result, calls = self.run_mock()
        self.assertEqual(result.returncode, 79, result.stdout + result.stderr)
        self.assertIn("install --no-install-recommends", calls)
        self.assertIn("vdr-suite-addon-media-tools=" + VERSION, calls)
        self.assertIn("-s", calls)
        self.assertNotIn("MT_I43_PUBLISHED_PPA_APT=PASS", result.stdout)

    def test_missing_published_candidate_refused_before_apt(self):
        result, calls = self.run_mock(MOCK_MISSING="1")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("MT_I43_PUBLISHED_ADDON=NOT_AVAILABLE", result.stderr)
        self.assertEqual(calls, "")

    def test_wrong_ppa_source_refused_before_apt(self):
        result, calls = self.run_mock(MOCK_WRONG_ORIGIN="1")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(calls, "")

    def test_source_version_mismatch_refused_before_apt(self):
        result, calls = self.run_mock(MOCK_WRONG_VERSION="1")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("MT_I43_SOURCE_VERSION_MISMATCH", result.stderr)
        self.assertEqual(calls, "")


if __name__ == "__main__":
    unittest.main()

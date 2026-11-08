#!/usr/bin/env python3
"""No-install regression for the managed, automatically cleaned runtime stage."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
INSTALL_MK = ROOT / "mk/install.mk"


class ManagedRuntimeInstallTests(unittest.TestCase):
    def test_managed_stage_is_removed_only_after_success(self):
        self._exercise(fail_target=None, expect_success=True)

    def test_failed_build_preserves_stage_and_cache(self):
        self._exercise(fail_target="stage-install-runtime", expect_success=False)

    def test_failed_deployment_preserves_stage_and_cache(self):
        self._exercise(fail_target="deploy-install-runtime", expect_success=False)

    def test_failed_verification_preserves_stage_and_cache(self):
        self._exercise(fail_target="check-install-runtime-deployment", expect_success=False)

    def _exercise(self, fail_target, expect_success):
        with tempfile.TemporaryDirectory(prefix="vdr-suite-managed-test-") as temp:
            root = Path(temp)
            cache = root / "stable-cache"
            cache.mkdir()
            (cache / "unchanged.o").write_bytes(b"keep")
            fake_make = root / "fake-make"
            fake_make.write_text(
                "#!/usr/bin/env python3\n"
                "from pathlib import Path\n"
                "import os, sys\n"
                "target = sys.argv[1]\n"
                "values = dict(a.split('=', 1) for a in sys.argv[2:] if '=' in a)\n"
                "stage = Path(values['RUNTIME_STAGE'])\n"
                "with (Path(os.environ['FAKE_MAKE_LOG'])).open('a') as out:\n"
                "    out.write(f'{target}|{stage}|{values.get(\"BUILD_DIR\", \"\")}\\n')\n"
                "if target == 'stage-install-runtime':\n"
                "    stage.mkdir(exist_ok=True)\n"
                "    (stage / 'sealed-stage').write_text('pass')\n"
                "else:\n"
                "    assert (stage / 'sealed-stage').read_text() == 'pass'\n"
                "if target == os.environ.get('FAIL_TARGET'):\n"
                "    sys.exit(9)\n",
                encoding="utf-8",
            )
            fake_make.chmod(0o755)
            env = dict(os.environ, TMPDIR=str(root),
                       FAKE_MAKE_LOG=str(root / "calls"),
                       FAIL_TARGET=fail_target or "")
            result = subprocess.run(
                ["make", "-f", str(INSTALL_MK), "install-runtime-managed",
                 f"MAKE={fake_make}", f"BUILD_DIR={cache}",
                 f"LIVE_ROOT={root / 'live'}"],
                cwd=ROOT, env=env, capture_output=True, text=True, check=False,
                timeout=25,
            )
            log = (root / "calls").read_text().splitlines()
            stages = {line.split("|")[1] for line in log}
            self.assertEqual(1, len(stages))
            stage = Path(next(iter(stages)))
            self.assertEqual(root, stage.parent)
            self.assertEqual(b"keep", (cache / "unchanged.o").read_bytes())
            if expect_success:
                self.assertEqual(0, result.returncode, result.stderr)
                self.assertEqual(3, len(log))
                self.assertFalse(stage.exists())
                self.assertIn("RUNTIME_TEMP_STAGE_CLEANED=YES", result.stdout)
            else:
                self.assertNotEqual(0, result.returncode)
                self.assertTrue(stage.is_dir())
                self.assertIn("RUNTIME_TEMP_STAGE_RETAINED=", result.stderr)
                self.assertEqual("pass", (stage / "sealed-stage").read_text())


if __name__ == "__main__":
    unittest.main()

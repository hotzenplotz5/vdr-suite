#!/usr/bin/env python3
"""No-root regression tests for the isolated source-only packaging helper."""
import ast
import contextlib
import io
import importlib.util
from pathlib import Path
from unittest import TestCase
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
SCRIPT = HERE / "build_vdr_suite_sourcepkg_isolated.py"
spec = importlib.util.spec_from_file_location("srcpkg", SCRIPT)
srcpkg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(srcpkg)


class SourcePackagingTests(TestCase):
    def test_source_is_valid_python(self):
        ast.parse(SCRIPT.read_text(encoding="utf-8"))

    def test_invalid_commit_argument_is_rejected(self):
        # argparse deliberately rejects the input; capture expected diagnostics
        # so a passing test run does not look like a real packaging error.
        with contextlib.redirect_stderr(io.StringIO()) as stream:
            with self.assertRaises(SystemExit) as ctx:
                srcpkg.main(["not-a-commit"])
        self.assertEqual(ctx.exception.code, 2)
        self.assertIn("expected SHA must contain exactly 40", stream.getvalue())

    def test_root_is_rejected_before_any_operations(self):
        with patch.object(srcpkg.os, "geteuid", return_value=0), self.assertRaisesRegex(
            RuntimeError, "run as non-root yavdr"
        ):
            srcpkg.preflight("0" * 40)

    def test_isolated_source_and_strict_no_deploy_contract(self):
        source = SCRIPT.read_text(encoding="utf-8")
        for needed in (
            "git\", \"ls-remote\"",
            "git\", \"status\", \"--porcelain\"",
            "RESOURCE_PREFLIGHT=PASS",
            "RESOURCE_POSTCHECK=PASS",
            "git\", \"archive\"",
            "PRISTINE_UPSTREAM_TARBALL=PASS",
            "--exclude=",
            "dpkg-buildpackage",
            "dpkg-source",
            "SOURCE_ROUNDTRIP=PASS",
            "UPLOAD=NOT_PERFORMED",
        ):
            with self.subTest(required=needed):
                self.assertIn(needed, source)
        for forbidden in ("sudo ", "apt-get ", "systemctl ", "shutil.rmtree", "dput ", "rm -rf "):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, source)

    def test_ubuntu_changelog_preview_noble_and_resolute(self):
        original = ("vdr-suite (0.14.1-1~exp1) unstable; urgency=low\n\n"
                    "  * Existing experimental Debian packaging.\n\n"
                    " -- VDR-Suite contributors <noreply@github.com>  Tue, 13 Oct 2026 00:00:00 +0200\n")
        for suite, suffix in (("noble", "24.04"), ("resolute", "26.04")):
            with self.subTest(suite=suite):
                preview, revision = srcpkg.prepare_ubuntu_preview(
                    original, "0.14.1", 1791940000, suite)
                self.assertEqual(revision, "0.14.1-1~exp1~ubuntu" + suffix + ".1")
                self.assertTrue(preview.startswith(
                    "vdr-suite (" + revision + ") " + suite + "; urgency=low\n"))
                self.assertIn("not approved for PPA.", preview)
                self.assertTrue(preview.endswith(original))
                self.assertIn("\n\n -- VDR-Suite contributors", preview)
                self.assertNotIn("\\n", preview)

    def test_unknown_series_rejected_without_mutation(self):
        source = "vdr-suite (0.14.1-1~exp1) unstable; urgency=low\n"
        with self.assertRaisesRegex(RuntimeError, "unsupported Ubuntu target"):
            srcpkg.prepare_ubuntu_preview(source, "0.14.1", 1791940000, "jammy")

    def test_no_build_before_storage_preflight(self):
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertLess(source.index("RESOURCE_PREFLIGHT=PASS"),
                        source.index("tempfile.mkdtemp"))
        self.assertLess(source.index("tempfile.mkdtemp"),
                        source.index('["dpkg-buildpackage", "-S"'))


if __name__ == "__main__":
    import unittest
    unittest.main()

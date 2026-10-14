#!/usr/bin/env python3
"""No-root regression tests for the isolated source-only packaging helper."""
import ast
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
        with self.assertRaises(SystemExit) as ctx:
            srcpkg.main(["not-a-commit"])
        self.assertEqual(ctx.exception.code, 2)

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

    def test_no_build_before_storage_preflight(self):
        source = SCRIPT.read_text(encoding="utf-8")
        self.assertLess(source.index("RESOURCE_PREFLIGHT=PASS"),
                        source.index("tempfile.mkdtemp"))
        self.assertLess(source.index("tempfile.mkdtemp"),
                        source.index('"dpkg-buildpackage"'))


if __name__ == "__main__":
    import unittest
    unittest.main()

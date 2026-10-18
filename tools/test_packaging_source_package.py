#!/usr/bin/env python3
"""No-root regression tests for the isolated source-only packaging helper."""
import ast
import contextlib
import io
import importlib.util
from pathlib import Path
from types import SimpleNamespace
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

    def test_signing_rejects_missing_ubuntu_series(self):
        with contextlib.redirect_stderr(io.StringIO()) as stream:
            with self.assertRaises(SystemExit) as ctx:
                srcpkg.main(["0" * 40, "--sign-with", "A" * 40])
        self.assertEqual(ctx.exception.code, 2)
        self.assertIn("--sign-with requires --ubuntu-preview", stream.getvalue())

    def test_signing_preflight_rejects_invalid_fingerprint_without_gpg(self):
        with self.assertRaisesRegex(RuntimeError, "40 uppercase hex"):
            srcpkg.signing_preflight("not-a-key")

    def test_signing_preflight_requires_matching_secret_and_identity(self):
        fingerprint = "A" * 40
        metadata = ("sec:u:4096:1:123:1792200000:1855272000:::::\n"
                    "fpr:::::::::" + fingerprint + ":\n"
                    "uid:u::::::::Holger Schvestka <hotzenplotz5@gmx.de>:\n")
        with patch.object(srcpkg.shutil, "which", return_value="/usr/bin/test"), \
             patch.object(srcpkg.subprocess, "run", return_value=SimpleNamespace(stdout=metadata)):
            srcpkg.signing_preflight(fingerprint)
        mismatch = metadata.replace("hotzenplotz5@gmx.de", "another@example.org")
        with patch.object(srcpkg.shutil, "which", return_value="/usr/bin/test"), \
             patch.object(srcpkg.subprocess, "run", return_value=SimpleNamespace(stdout=mismatch)):
            with self.assertRaisesRegex(RuntimeError, "does not match"):
                srcpkg.signing_preflight(fingerprint)

    def test_signatures_must_match_explicit_fingerprint(self):
        fingerprint = "A" * 40
        verified = SimpleNamespace(returncode=0,
                                   stdout="[GNUPG:] VALIDSIG " + fingerprint + " 2026-10-17\n")
        with patch.object(srcpkg.subprocess, "run", return_value=verified):
            srcpkg.verify_signature(Path("source.changes"), fingerprint)
        with patch.object(srcpkg.subprocess, "run", return_value=SimpleNamespace(
                returncode=0, stdout="[GNUPG:] VALIDSIG " + "B" * 40 + "\n")):
            with self.assertRaisesRegex(RuntimeError, "does not match"):
                srcpkg.verify_signature(Path("source.changes"), fingerprint)

    def test_signable_preview_has_real_changed_by_only_inside_preview(self):
        original = "vdr-suite (0.14.1-1~exp2) unstable; urgency=low\n"
        preview, version = srcpkg.prepare_ubuntu_preview(
            original, "0.14.1", 1791940000, "noble", srcpkg.SIGNING_IDENTITY)
        self.assertIn(" -- Holger Schvestka <hotzenplotz5@gmx.de>  ", preview)
        self.assertIn("not approved for PPA", preview)
        self.assertEqual(version, "0.14.1-1~exp2~ubuntu24.04.1")
        self.assertTrue(preview.endswith(original))

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

#!/usr/bin/env python3
"""Unit tests for the bounded codec-reference operator tool (no network/media)."""
import contextlib
import importlib.util
import io
from pathlib import Path
from unittest import TestCase, mock
import unittest


SPEC = importlib.util.spec_from_file_location(
    "codec_reference_compare",
    Path(__file__).resolve().parents[1] / "tools" / "codec_reference_compare.py")
codec = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(codec)


class CodecReferenceCompareTests(TestCase):
    def test_manifest_has_explicit_size_and_https(self):
        for name, info in codec.REFERENCES.items():
            with self.subTest(reference=name):
                self.assertTrue(info["url"].startswith("https://lafibre.info/videos/test/"))
                self.assertGreater(info["bytes"], 1_000_000)
                self.assertLess(info["bytes"], 300 * 1024 * 1024)
                self.assertIn(info["codec"], ("hevc", "h264", "mpeg2video"))

    def test_exact_framehashes_counts_only_exact_hash_pairs(self):
        md5a = "0" * 32
        md5b = "1" * 32
        sample = ("\n".join([
            "#format: frame checksums",
            "#stream#, dts, pts, duration, size, hash",
            f"0, 0, 0, 1, 123, {md5a}",
            f"0, 1, 1, 1, 123, {md5a}",
            f"0, 2, 2, 1, 123, {md5b}",
        ]) + "\n")
        output = io.StringIO()
        with mock.patch.object(codec, "command", return_value=(0, sample, "")), \
                contextlib.redirect_stdout(output):
            codec.exact_frame_hashes(Path("/ignored"), 10)
        report = output.getvalue()
        self.assertIn("decoded=3", report)
        self.assertIn("consecutive_exact_duplicate=1", report)
        self.assertIn("ratio=50.00%", report)

    def test_source_unreadable_fails_closed(self):
        with self.assertRaises(RuntimeError):
            codec.ensure_source(Path("/definitely-not-a-valid-source-file.ts"))

    def test_default_is_one_case_not_mass_scan(self):
        self.assertEqual(codec.KNOWN["medicus"].split("/")[-1], "00001.ts")
        self.assertEqual(len(codec.REFERENCES), 3)


if __name__ == "__main__":
    unittest.main()

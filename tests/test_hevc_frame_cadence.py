#!/usr/bin/env python3
"""Unit checks for HEVC cadence analyzer; no media/network access."""
import contextlib
from fractions import Fraction
import importlib.util
import io
from pathlib import Path
from unittest import TestCase
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "analyze_hevc_frame_cadence.py"
SPEC = importlib.util.spec_from_file_location("analyze_hevc_frame_cadence", SCRIPT)
module = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(module)


class FiveWindowTests(TestCase):
    def test_positions_are_non_overlapping_and_bounded(self):
        duration = 7200.0
        starts = module.get_positions(duration)
        self.assertEqual(len(starts), 5)
        self.assertTrue(all(0 <= x <= duration-15 for x in starts))
        self.assertTrue(all(b-a > 10 for a,b in zip(starts,starts[1:])))

    def test_window_overlap_fails_closed(self):
        with self.assertRaises(RuntimeError):
            module.get_positions(60.0)

    def test_md5_parse_decodes_timebase_and_hashes(self):
        sample = "#tb 0: 1/90000\n#media_type 0: video\n"
        for n, digest in enumerate(["0"*32,"0"*32,"1"*32,
                                    "2"*32,"2"*32,"3"*32,
                                    "4"*32,"5"*32,"6"*32,"7"*32]):
            sample += f"0, {n*3750}, {n*3750}, 3750, 123, {digest}\n"
        tb, frames = module.parse_framemd5(sample)
        self.assertEqual(tb, Fraction(1,90000))
        result = module.summarize_frames(tb, frames)
        self.assertEqual(result["decoded"], 10)
        self.assertEqual(result["exact_duplicate_pairs"], 2)
        self.assertEqual(result["longest_identical_run"], 2)
        self.assertEqual(result["repeat_runs"], 2)
        self.assertEqual(result["frame_pts_median_ms"], 41.667)

    def test_all_unique_frames(self):
        frames = [(i*3750, f"{i:032x}") for i in range(240)]
        report = module.summarize_frames(Fraction(1,90000), frames)
        self.assertEqual(report["exact_duplicate_pairs"], 0)
        self.assertEqual(report["longest_identical_run"], 1)

    def test_long_still_sequence_is_not_periodic(self):
        frames = [(i*3750, "a"*32 if i < 20 else f"{i:032x}")
                  for i in range(240)]
        report = module.summarize_frames(Fraction(1,90000), frames)
        self.assertEqual(report["exact_duplicate_pairs"], 19)
        self.assertEqual(report["longest_identical_run"], 20)
        self.assertEqual(report["repeat_runs"], 1)


if __name__ == "__main__":
    unittest.main()

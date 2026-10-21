#!/usr/bin/env python3
"""Offline fail-closed tests for R2/R3 real-hardware acceptance fixture guard."""
import datetime
import importlib.util
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "hw", ROOT / "tools" / "accept_api_v1_recording_actions_hw.py")
hw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hw)


def fixture(filename=None):
    started = datetime.datetime.now().strftime("%Y-%m-%d.%H.%M")
    return {
        "name": hw.FIXTURE,
        "file_name": filename or ("/srv/vdr/video/" + hw.FIXTURE +
                                  "/" + started + ".1-0.rec"),
        "number": 71, "event_start_time": 1761040800,
        "duration": 120, "filesize_mb": 40,
    }


def public():
    return [{
        "backendId": "default",
        "title": hw.FIXTURE,
        "recordingId": "rec_" + "a" * 32,
        "recordedAt": "1761040800",
        "sizeMb": 40,
    }]


class HardwareAcceptanceSafetyTests(unittest.TestCase):
    def test_only_unique_exact_fixture_is_accepted(self):
        original = fixture()
        self.assertEqual(hw.choose_fixture([original]), original)
        for records in ([], [original, fixture()],
                        [dict(original, name="Report Mainz")],
                        [dict(original, file_name="/srv/vdr/video/Other/" +
                              original["file_name"].rsplit("/", 1)[-1])]):
            with self.assertRaises(hw.Stop):
                hw.choose_fixture(records)

    def test_old_fixture_and_noncompleted_are_rejected(self):
        old = fixture("/srv/vdr/video/" + hw.FIXTURE +
                      "/2025-01-01.01.00.1-0.rec")
        with self.assertRaises(hw.Stop):
            hw.choose_fixture([old])
        with self.assertRaises(hw.Stop):
            hw.choose_fixture([dict(fixture(), filesize_mb=0)])

    def test_only_correlated_public_id_allowed(self):
        native = fixture()
        self.assertEqual(hw.correlate(public(), native, "default")["recordingId"],
                         "rec_" + "a" * 32)
        with self.assertRaises(hw.Stop):
            hw.correlate(public() * 2, native, "default")
        with self.assertRaises(hw.Stop):
            hw.correlate(public(), native, "other")

    def test_readback_fails_closed_on_old_or_ambiguous(self):
        before = fixture()
        moved_path = before["file_name"].replace(hw.FIXTURE, "MOVED")
        after = dict(before, name=hw.RENAMED, file_name=moved_path)
        self.assertEqual(hw.require_readback(before, [after], "RENAME"), after)
        for inventory in ([before, after], [], [after, after],
                          [dict(after, duration=10)]):
            with self.assertRaises(hw.Stop):
                hw.require_readback(before, inventory, "MOVE")

    def test_only_local_origin_and_public_shape(self):
        for url in ("https://127.0.0.1:18080",
                    "http://example.net:18080",
                    "http://127.0.0.1:18080/path",
                    "http://user:pass@127.0.0.1:18080"):
            with self.assertRaises(hw.Stop):
                hw.local_origin(url)
        self.assertTrue(hw.check_public_id("rec_" + "0" * 32))
        self.assertFalse(hw.check_public_id("71"))
        self.assertFalse(hw.check_public_id("rec_" + "G" * 32))


if __name__ == "__main__":
    unittest.main()

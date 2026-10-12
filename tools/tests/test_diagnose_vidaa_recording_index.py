import contextlib
import io
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import diagnose_vidaa_recording_index as diagnostic


class RecordingIndexReadOnlyTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name) / "Western"
        self.recording = self.root / "Der_Mann_den_sie_Pferd_nannten" / "2026-01-01.00.00.1-0.rec"
        self.recording.mkdir(parents=True)
        (self.recording / "info").write_text("F 25 1920 1080 p 16:9\n")
        for number in (1, 2):
            (self.recording / f"{number:05d}.ts").write_bytes(b"\0" * 188)
        self.write_index(1, 2)

    def tearDown(self):
        self.tmp.cleanup()

    def write_index(self, first, last):
        with (self.recording / "index").open("wb") as index:
            for number in (first, last):
                for _ in range(75):
                    index.write(struct.pack("<Q", number << 48))

    def evaluate(self):
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer):
            diagnostic.inspect(self.recording)
        return buffer.getvalue()

    def test_complete_index_and_two_ts_segments(self):
        index = self.recording / "index"
        before = index.read_bytes()
        result = self.evaluate()
        self.assertIn("INDEX_DURATION_SECONDS=6", result)
        self.assertIn("INDEX_END_COMPLETE=YES", result)
        self.assertIn("INDEX_TS_FULLY_MAPPED=YES", result)
        self.assertIn("HLS_SYNC_PROFILE=NOT_TESTED", result)
        self.assertEqual(index.read_bytes(), before)

    def test_duration_known_while_middle_segment_is_unmapped(self):
        (self.recording / "00003.ts").write_bytes(b"\0" * 188)
        self.write_index(1, 3)
        result = self.evaluate()
        self.assertIn("INDEX_DURATION_SECONDS=6", result)
        self.assertIn("INDEX_END_COMPLETE=YES", result)
        self.assertIn("TS_WITHOUT_INDEX_FRAMES=1", result)
        self.assertIn("INDEX_TS_FULLY_MAPPED=NO", result)

    def test_growing_recording_refuses_complete_end(self):
        (self.recording / ".timer").write_text("running")
        result = self.evaluate()
        self.assertIn("SOURCE_GROWING=YES", result)
        self.assertIn("INDEX_END_COMPLETE=NO", result)

    def test_missing_or_symlinked_index_fails_closed(self):
        (self.recording / "index").unlink()
        with self.assertRaisesRegex(RuntimeError, "INDEX_MISSING_OR_UNSAFE"):
            self.evaluate()
        (self.recording / "index").symlink_to(self.recording / "info")
        with self.assertRaisesRegex(RuntimeError, "INDEX_MISSING_OR_UNSAFE"):
            self.evaluate()

    def test_matches_only_requested_recording_directory(self):
        self.assertEqual(
            diagnostic.find_recordings([self.root], "Pferd"),
            [self.recording])
        self.assertEqual(
            diagnostic.find_recordings([self.root], "Nicht_vorhanden"), [])


if __name__ == "__main__":
    unittest.main()

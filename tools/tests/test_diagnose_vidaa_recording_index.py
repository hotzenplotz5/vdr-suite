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

    def test_yavdr_video_symlink_alias_resolves_to_real_video_directory(self):
        alias = Path(self.tmp.name) / "video"
        alias.symlink_to(self.root, target_is_directory=True)
        self.assertEqual(diagnostic.resolved_video_root(alias),
                         self.root.resolve())
        self.assertEqual(diagnostic.resolved_video_root(self.root),
                         self.root.resolve())
        self.assertEqual(
            diagnostic.find_recordings(
                [diagnostic.resolved_video_root(alias)], "Pferd"),
            [self.recording])
        self.assertIsNone(diagnostic.resolved_video_root(Path("/")))
        self.assertIsNone(diagnostic.resolved_video_root(Path("relative/video")))

    def test_two_recordings_are_examined_separately_read_only(self):
        alternate = self.root / "Der_Mann_den_sie_Pferd_nannten" / "2026-04-01.00.00.1-0.rec"
        alternate.mkdir(parents=True)
        (alternate / "info").write_text("F 25 1920 1080 p 16:9\\n")
        (alternate / "00001.ts").write_bytes(b"\\0" * 188)
        (alternate / "00002.ts").write_bytes(b"\\0" * 188)
        (alternate / "index").write_bytes((self.recording / "index").read_bytes())
        before = [(p / "index").read_bytes() for p in (self.recording, alternate)]
        matches = diagnostic.find_recordings([self.root], "Pferd")
        self.assertEqual(len(matches), 2)
        with self.assertRaisesRegex(RuntimeError, "RECORDING_NOT_UNIQUE"):
            with contextlib.redirect_stdout(io.StringIO()):
                diagnostic.diagnose_matches(matches)
        result = io.StringIO()
        with contextlib.redirect_stdout(result):
            diagnostic.diagnose_matches(matches, all_matches=True)
        output = result.getvalue()
        self.assertIn("MATCHED_RECORDINGS=2", output)
        self.assertIn("=== RECORDING_1_OF_2 ===", output)
        self.assertIn("=== RECORDING_2_OF_2 ===", output)
        self.assertEqual(output.count("INDEX_TS_FULLY_MAPPED=YES"), 2)
        self.assertIn("RESULT=READ_ONLY_COMPLETE", output)
        self.assertEqual(before, [(p / "index").read_bytes()
                                  for p in (self.recording, alternate)])

    def test_one_broken_recording_does_not_prevent_second_diagnosis(self):
        alternate = self.root / "Der_Mann_den_sie_Pferd_nannten" / "2026-04-01.00.00.1-0.rec"
        alternate.mkdir(parents=True)
        (alternate / "info").write_text("F 25 1920 1080 p 16:9\\n")
        (alternate / "00001.ts").write_bytes(b"\\0" * 188)
        matches = diagnostic.find_recordings([self.root], "Pferd")
        result = io.StringIO()
        with self.assertRaisesRegex(RuntimeError, "ONE_OR_MORE_RECORDINGS_INCOMPLETE"):
            with contextlib.redirect_stdout(result):
                diagnostic.diagnose_matches(matches, all_matches=True)
        output = result.getvalue()
        self.assertEqual(output.count("RECORDING_RESULT="), 2)
        self.assertIn("RECORDING_STOP=INDEX_MISSING_OR_UNSAFE", output)
        self.assertIn("INDEX_TS_FULLY_MAPPED=YES", output)

    def test_bounded_all_matches_rejects_five_before_inspecting(self):
        matches = [self.recording] * 5
        output = io.StringIO()
        with self.assertRaisesRegex(RuntimeError, "TOO_MANY_RECORDINGS_FOR_BOUNDED_DIAGNOSIS"):
            with contextlib.redirect_stdout(output):
                diagnostic.diagnose_matches(matches, all_matches=True)
        self.assertNotIn("INDEX_FRAMES=", output.getvalue())

    def test_matches_only_requested_recording_directory(self):
        self.assertEqual(
            diagnostic.find_recordings([self.root], "Pferd"),
            [self.recording])
        self.assertEqual(
            diagnostic.find_recordings([self.root], "Nicht_vorhanden"), [])


if __name__ == "__main__":
    unittest.main()

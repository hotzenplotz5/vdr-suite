#!/usr/bin/env python3
"""Guard the production Public-v1 flat Recording source against lazy-startup regression."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class RecordingCacheWiringTest(unittest.TestCase):
    def test_flat_collection_reads_warmed_backend_cache(self):
        source = (ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp").read_text()
        section = source.split("registerRecordingCollectionLookup(", 1)[1].split(
            "// Same Suite Genre index", 1)[0]
        flat = section.split("            // ADR-0035 intentionally skips recordings", 1)[1]
        self.assertIn("statusForBackend(request.backendId)", flat)
        self.assertIn('cacheStatus.state != "ready"', flat)
        self.assertIn("findAllForBackend(request.backendId)", flat)
        self.assertIn("cached.empty() && cacheStatus.totalCount > 0", flat)
        self.assertIn("projection.project(request.backendId, cached)", flat)
        self.assertNotIn("getRecordingsForBackend(", flat)
        self.assertIn("folderPageForBackendByPublicId(", section)

    def test_startup_snapshot_stays_lightweight(self):
        source = (ROOT / "core/vdr/include/VdrSnapshotBuilder.h").read_text()
        startup = source.split("VdrSnapshot buildStartupSnapshot() const", 1)[1].split(
            "VdrSnapshot buildSnapshot()", 1)[0]
        self.assertNotIn("buildRecordings()", startup)
        self.assertNotIn("buildEvents()", startup)


if __name__ == "__main__":
    unittest.main()

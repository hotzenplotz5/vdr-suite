#!/usr/bin/env python3
"""P7 C++ runtime SQLite-DDL writer inventory, distinct from the 33 owners.

Fail-closed file-owner review gate, not an SQL parser or production approval.
Adjacent C++ string literals are joined; constructed SQL and migration DML
remain explicit follow-up work.
"""
from pathlib import Path
import re
import unittest

from test_packaging_p7_schema_owner_inventory import EXERCISED_BY_P7_CPP

ROOT = Path(__file__).resolve().parent.parent
EXERCISED = EXERCISED_BY_P7_CPP | frozenset({
    "core/metadata/src/GenreIndexRepositorySchema.inc",
    "core/timers/src/TimerAssignmentSetRevisionRepository.cpp",
})
PENDING = frozenset({
    "core/agent/src/BackendAgentCommandDelivery.cpp",
    "core/agent/src/BackendAgentCommandReservation.cpp",
    "core/agent/src/BackendAgentNativeTimerDeleteAssignment.cpp",
    "core/agent/src/BackendAgentNativeTimerModifyAssignment.cpp",
    "core/agent/src/BackendAgentRecordingCutAssignment.cpp",
    "core/agent/src/BackendAgentRecordingMarksModifyAssignment.cpp",
    "core/agent/src/BackendAgentRecordingCutReconciliationRepository.cpp",
    "core/agent/src/BackendAgentRecordingMarksModifyReconciliation.cpp",
    "core/daemon/src/EmbeddedBackendLifecycle.cpp",
    "core/daemon/src/SeriesArtworkBackendSettingsService.cpp",
    "core/recordings/src/ManualRecordingMetadataRepositoryFacade.cpp",
    "core/timers/src/TimerAssignmentReassignmentRepository.cpp",
})
INDIRECTLY_EXERCISED = frozenset({
    "core/agent/src/BackendRuntimeGeneration.cpp",
})
ALL_CLASSIFIED = EXERCISED | PENDING | INDIRECTLY_EXERCISED

DDL = re.compile(
    r"\b(?:CREATE\s+(?:(?:TEMP|TEMPORARY|UNIQUE|VIRTUAL)\s+)*"
    r"(?:TABLE|INDEX|TRIGGER|VIEW)|ALTER\s+TABLE|"
    r"DROP\s+(?:TABLE|INDEX|TRIGGER|VIEW)|"
    r"REINDEX\b|PRAGMA\s+(?:user_version|schema_version))\b",
    re.IGNORECASE,
)


def find_runtime_ddl_owners(root=ROOT):
    found = set()
    # SQL in tests, tools and historical .sql is separately out of scope.
    for base in (root / "core", root / "api" / "rest" / "src"):
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix not in (".cpp", ".cc", ".c", ".inc", ".h", ".hpp"):
                continue
            relative = path.relative_to(root).as_posix()
            if "/tests/" in relative or (relative.startswith("core/")
                                         and "/src/" not in relative):
                continue
            source = path.read_text(encoding="utf-8")
            source = re.sub(r'"\s*"', "", source)
            if DDL.search(source):
                found.add(relative)
    return frozenset(found)


class P7RuntimeSqlWriterInventory(unittest.TestCase):
    def test_every_runtime_ddl_file_is_classified(self):
        found = find_runtime_ddl_owners()
        self.assertEqual(found, ALL_CLASSIFIED,
                         "Unknown/removed C++ runtime DDL writer: review "
                         "and classify; do not mark untested as covered")
        print('P7_RUNTIME_DDL_FILE_OWNERS_CLASSIFIED=' + str(len(found)) +
              ' PENDING=' + str(len(PENDING)))
        self.assertFalse((EXERCISED & PENDING) |
                         (EXERCISED & INDIRECTLY_EXERCISED) |
                         (PENDING & INDIRECTLY_EXERCISED))

    def test_first_additional_cpp_writer_is_rehearsed(self):
        build = (ROOT / "mk" / "metadata-tests.mk").read_text(encoding="utf-8")
        harness = (ROOT / "core" / "metadata" / "tests" /
                   "test_packaging_p7_runtime_schema.cpp").read_text(
                       encoding="utf-8")
        self.assertIn(
            "core/timers/src/TimerAssignmentSetRevisionRepository.cpp \\",
            build)
        self.assertIn("assignmentSetRevisionForIntent(", harness)
        self.assertIn("P7_TIMER_ASSIGNMENT_SET_REVISION_SCHEMA=PASS", harness)
        self.assertIn("p7_legacy_assignment", harness)
        self.assertIn('assignmentSetRevision != "7"', harness)


if __name__ == "__main__":
    unittest.main()

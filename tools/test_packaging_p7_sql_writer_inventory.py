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
    "core/daemon/src/EmbeddedBackendLifecycle.cpp",
    # CI #65: real C++ SeriesArtworkBackendSettingsService::ensureSchema.
    "core/daemon/src/SeriesArtworkBackendSettingsService.cpp",
    # CI #67: real C++ TimerAssignmentReassignmentRepository ALTER + triggers.
    "core/timers/src/TimerAssignmentReassignmentRepository.cpp",
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
    "core/recordings/src/ManualRecordingMetadataRepositoryFacade.cpp",
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

    def test_embedded_backend_lifecycle_schema_is_rehearsed(self):
        build = (ROOT / "mk" / "metadata-tests.mk").read_text(
            encoding="utf-8")
        harness = (ROOT / "core" / "metadata" / "tests" /
                   "test_packaging_p7_runtime_schema.cpp").read_text(
                       encoding="utf-8")
        rehearsal = (ROOT / "tools" /
                     "packaging_p7_runtime_schema_rehearsal.py").read_text(
                         encoding="utf-8")
        self.assertIn("core/daemon/src/EmbeddedBackendLifecycle.cpp \\", build)
        self.assertIn("embeddedBackend.ensureSchema()", harness)
        self.assertIn("INSERT INTO embedded_backend_lifecycle(", rehearsal)
        self.assertIn("P7_EMBEDDED_BACKEND_LIFECYCLE_PRESERVATION=PASS",
                      rehearsal)


    def test_series_artwork_legacy_cpp_rehearsal_is_wired(self):
        """Require real C++ coverage in P7 CI; do not claim CI has passed."""
        target = (ROOT / "mk" / "epg-metadata-tests.mk").read_text(
            encoding="utf-8")
        cpp = (ROOT / "core" / "daemon" / "tests" /
               "test_series_artwork_backend_settings_service.cpp").read_text(
                   encoding="utf-8")
        workflow = (ROOT / ".github" / "workflows" /
                    "packaging-plugin-experimental.yml").read_text(
                        encoding="utf-8")
        self.assertIn("core/daemon/src/SeriesArtworkBackendSettingsService.cpp", target)
        self.assertIn("core/daemon/tests/test_series_artwork_backend_settings_service.cpp", target)
        self.assertIn("make test-suite-bridge-epg-metadata-resolver", workflow)
        self.assertIn("INSERT INTO backend_series_artwork_overrides", cpp)
        self.assertIn("legacy-backend", cpp)
        self.assertIn("assert(service.ensureSchema());", cpp)
        self.assertIn("AND poster_reference='';", cpp)
        self.assertIn("mkdtemp(writable.data())", cpp)


    def test_timer_reassignment_legacy_cpp_rehearsal_is_wired(self):
        """Guard exact tested C++ migration, including the temp-trigger path."""
        makefile = (ROOT / "mk" /
                    "phase64-timer-reassignment-failover-tests.mk").read_text(
                        encoding="utf-8")
        cpp = (ROOT / "core" / "timers" / "tests" /
               "test_timer_assignment_reassignment_service.cpp").read_text(
                   encoding="utf-8")
        workflow = (ROOT / ".github" / "workflows" /
                    "packaging-plugin-experimental.yml").read_text(
                        encoding="utf-8")
        self.assertIn("core/timers/src/TimerAssignmentReassignmentRepository.cpp",
                      makefile)
        self.assertIn("core/timers/tests/test_timer_assignment_reassignment_service.cpp",
                      makefile)
        self.assertIn("make test-phase64-timer-reassignment-failover", workflow)
        self.assertIn("DROP COLUMN old_operation_revision;", cpp)
        self.assertIn("service.reassign(request)", cpp)
        self.assertIn("assignments.findControlledReplacement(", cpp)
        self.assertIn("FROM pragma_foreign_key_check;", cpp)
        self.assertIn("sqlite_temp_master", cpp)
        self.assertIn("P7_TIMER_REASSIGNMENT_LEGACY_COLUMN=PASS", cpp)


    def test_agent_result_evidence_legacy_cpp_rehearsal_is_wired(self):
        """Guard the actual C++ ALTER, trigger cleanup, and restored-row check."""
        makefile = (ROOT / "mk" / "agent-tests.mk").read_text(
            encoding="utf-8")
        cpp = (ROOT / "core" / "agent" / "tests" /
               "test_backend_agent_command_delivery.cpp").read_text(
                   encoding="utf-8")
        workflow = (ROOT / ".github" / "workflows" /
                    "packaging-plugin-experimental.yml").read_text(
                        encoding="utf-8")
        runtime = (ROOT / "core" / "agent" / "src" /
                   "BackendAgentCommandDelivery.cpp").read_text(
                       encoding="utf-8")
        self.assertIn("core/agent/tests/test_backend_agent_command_delivery.cpp",
                      makefile)
        self.assertIn("make test-phase63-command-delivery-runtime", workflow)
        self.assertIn("ADD COLUMN result_evidence TEXT NOT NULL DEFAULT ''", runtime)
        self.assertIn("DROP COLUMN result_evidence;", cpp)
        self.assertIn("assert(commands.ensureSchema());", cpp)
        self.assertIn("sqlite3_backup_init(", cpp)
        self.assertIn("sqlite_master WHERE name IN (", cpp)
        self.assertIn("P7_AGENT_RESULT_EVIDENCE_MIGRATION=PASS", cpp)


if __name__ == "__main__":
    unittest.main()

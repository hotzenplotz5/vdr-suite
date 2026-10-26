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
    # CI run 38766064163: real Agent Command Delivery C++ result-evidence ALTER.
    "core/agent/src/BackendAgentCommandDelivery.cpp",
    # CI #73: real C++ reservation persisted schema, and timer-delete gate.
    "core/agent/src/BackendAgentCommandReservation.cpp",
    "core/agent/src/BackendAgentNativeTimerDeleteAssignment.cpp",
    # CI #76: verified recorded cut/marks readbacks with real C++ and backup.
    "core/agent/src/BackendAgentRecordingCutReconciliationRepository.cpp",
    "core/agent/src/BackendAgentRecordingMarksModifyReconciliation.cpp",
    # CI #84: actual C++ assignment-gate schema and profile-facade tests.
    "core/agent/src/BackendAgentNativeTimerModifyAssignment.cpp",
    "core/agent/src/BackendAgentRecordingCutAssignment.cpp",
    "core/agent/src/BackendAgentRecordingMarksModifyAssignment.cpp",
    "core/recordings/src/ManualRecordingMetadataRepositoryFacade.cpp",
})
PENDING = frozenset({
    # Already present on main, not rehearsed in the historical P7 SQL harness.
    # Classified explicitly; do not misrepresent it as exercised.
    "core/security/src/DevicePlaybackAccountBindingService.cpp",
    # Canonical Media Tools/Add-on SQL owners. Several have their own focused
    # C++ unit tests, but are NOT exercised in the historical P7 migration
    # rehearsal; keep their real-package upgrade evidence outstanding.
    "core/addons/src/AddonActivationIntentRepository.cpp",
    "core/addons/media-tools/src/MediaShrinkReviewRepository.cpp",
    "core/agent/src/BackendAgentMediaImportAssignment.cpp",
    "core/agent/src/BackendAgentMediaShrinkAssignment.cpp",
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


    def test_focused_addon_sqlite_backup_tests_are_ci_wired_without_p7_credit(self):
        """Two real repository SQLite backup probes, not historical P7."""
        activation = (ROOT / "core/addons/tests/"
                      "test_addon_activation_intent_repository.cpp").read_text(
                          encoding="utf-8")
        shrink = (ROOT / "core/addons/media-tools/tests/"
                  "test_media_shrink_review_repository.cpp").read_text(
                      encoding="utf-8")
        makefile = (ROOT / "mk/addons.mk").read_text(encoding="utf-8")
        workflow = (ROOT / ".github/workflows/"
                    "addon-platform-isolated.yml").read_text(encoding="utf-8")
        self.assertIn("sqlite3_backup_init(", activation)
        self.assertIn("sqlite3_backup_init(", shrink)
        self.assertIn("P7_ADDON_ACTIVATION_BACKUP=PASS", activation)
        self.assertIn("P7_MEDIA_SHRINK_REVIEW_BACKUP=PASS", shrink)
        self.assertIn("assert(restoredIntents.ensureSchema());", activation)
        self.assertIn("assert(restoredReviews.ensureSchema());", shrink)
        self.assertIn("addon_p7_preexisting", activation)
        self.assertIn("addon_p7_preexisting", shrink)
        self.assertIn("test-addon-activation-intent:", makefile)
        self.assertIn("test-addon-media-shrink-core-review:", makefile)
        self.assertIn("make test-addon-activation-intent", workflow)
        self.assertIn("make test-addon-media-shrink-core-review", workflow)
        # Neither unit fixture is a historical P7/real-package rehearsal.
        self.assertIn(
            "core/addons/src/AddonActivationIntentRepository.cpp", PENDING)
        self.assertIn(
            "core/addons/media-tools/src/MediaShrinkReviewRepository.cpp",
            PENDING)

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


    def test_agent_reservation_schema_and_backup_are_ci_wired(self):
        makefile = (ROOT / "mk" /
                    "phase64-backend-agent-command-reservation-tests.mk").read_text(
                        encoding="utf-8")
        cpp = (ROOT / "core" / "agent" / "tests" /
               "test_backend_agent_command_reservation.cpp").read_text(
                   encoding="utf-8")
        workflow = (ROOT / ".github" / "workflows" /
                    "packaging-plugin-experimental.yml").read_text(
                        encoding="utf-8")
        self.assertIn("core/agent/src/BackendAgentCommandReservation.cpp",
                      makefile)
        self.assertIn("make test-phase64-backend-agent-command-reservation",
                      workflow)
        self.assertIn("assert(reservationRepository.ensureSchema());", cpp)
        self.assertIn("idx_backend_agent_command_reservations_operation", cpp)
        self.assertIn("sqlite3_backup_init(", cpp)
        self.assertIn("P7_AGENT_RESERVATION_SCHEMA_PRESERVATION=PASS", cpp)

    def test_agent_timer_delete_dormant_gate_is_ci_wired(self):
        makefile = (ROOT / "mk" /
                    "phase64-native-timer-delete-assignment-tests.mk").read_text(
                        encoding="utf-8")
        cpp = (ROOT / "core" / "agent" / "tests" /
               "test_backend_agent_native_timer_delete_assignment.cpp").read_text(
                   encoding="utf-8")
        workflow = (ROOT / ".github" / "workflows" /
                    "packaging-plugin-experimental.yml").read_text(
                        encoding="utf-8")
        self.assertIn("core/agent/src/BackendAgentNativeTimerDeleteAssignment.cpp",
                      makefile)
        self.assertIn("make test-phase64-native-timer-delete-assignment", workflow)
        self.assertIn("idx_backend_agent_timer_delete_operation", cpp)
        self.assertIn("trg_backend_agent_timer_delete_dormant_capability", cpp)
        self.assertIn("P7_AGENT_NATIVE_TIMER_DELETE_DORMANT_GATE=PASS", cpp)


    def test_cut_and_marks_readback_sql_migration_are_ci_wired(self):
        workflow = (ROOT / ".github" / "workflows" /
                    "packaging-plugin-experimental.yml").read_text(
                        encoding="utf-8")
        cases = (
            ("core/agent/src/BackendAgentRecordingCutReconciliationRepository.cpp",
             "core/agent/tests/test_backend_agent_recording_cut_reconciliation.cpp",
             "mk/recording-native-cut-guard.mk",
             "test-backend-agent-recording-cut-reconciliation",
             "idx_backend_agent_recording_cut_readback_operation"),
            ("core/agent/src/BackendAgentRecordingMarksModifyReconciliation.cpp",
             "core/agent/tests/test_backend_agent_recording_marks_modify_reconciliation.cpp",
             "mk/recording-native-editing-tests.mk",
             "test-backend-agent-recording-marks-modify-reconciliation",
             "idx_backend_agent_recording_marks_modify_readback_operation"),
        )
        for source, test, make_path, target, index in cases:
            with self.subTest(source=source):
                makefile = (ROOT / make_path).read_text(encoding="utf-8")
                cpp = (ROOT / test).read_text(encoding="utf-8")
                self.assertTrue((ROOT / source).is_file())
                self.assertIn(test, makefile)
                self.assertIn("make " + target, workflow)
                self.assertIn(index, cpp)
                self.assertIn("sqlite3_backup_init(", cpp)
                self.assertIn("pragma_foreign_key_check;", cpp)
                self.assertIn("assert(reopened.ensureSchema());", cpp)


    def test_remaining_agent_assignment_sql_writers_are_ci_wired(self):
        cpp = (ROOT / "core" / "agent" / "tests" /
               "test_backend_agent_command_reservation.cpp").read_text(
                   encoding="utf-8")
        makefile = (ROOT / "mk" /
                    "phase64-backend-agent-command-reservation-tests.mk").read_text(
                        encoding="utf-8")
        workflow = (ROOT / ".github" / "workflows" /
                    "packaging-plugin-experimental.yml").read_text(
                        encoding="utf-8")
        for source, schema, index in (
            ("core/agent/src/BackendAgentNativeTimerModifyAssignment.cpp",
             "ensureNativeTimerModifyAssignmentSchema",
             "idx_backend_agent_timer_modify_operation"),
            ("core/agent/src/BackendAgentRecordingCutAssignment.cpp",
             "ensureRecordingCutAssignmentSchema",
             "idx_backend_agent_recording_cut_operation"),
            ("core/agent/src/BackendAgentRecordingMarksModifyAssignment.cpp",
             "ensureRecordingMarksModifyAssignmentSchema",
             "idx_backend_agent_recording_marks_modify_operation"),
        ):
            with self.subTest(source=source):
                self.assertTrue((ROOT / source).is_file())
                self.assertIn(schema, cpp)
                self.assertIn(index, cpp)
        self.assertIn("core/agent/tests/test_backend_agent_command_reservation.cpp",
                      makefile)
        self.assertIn("make test-phase64-backend-agent-command-reservation",
                      workflow)
        self.assertIn("sqlite3_backup_init(", cpp)
        self.assertIn("P7_AGENT_ASSIGNMENT_DDL_WRITERS=PASS", cpp)

    def test_manual_person_profile_schema_and_recovery_are_ci_wired(self):
        cpp = (ROOT / "core" / "recordings" / "tests" /
               "test_manual_recording_metadata_revision.cpp").read_text(
                   encoding="utf-8")
        makefile = (ROOT / "mk" /
                    "manual-recording-metadata-tests.mk").read_text(
                        encoding="utf-8")
        workflow = (ROOT / ".github" / "workflows" /
                    "packaging-plugin-experimental.yml").read_text(
                        encoding="utf-8")
        self.assertIn(
            "core/recordings/src/ManualRecordingMetadataRepositoryFacade.cpp",
            makefile)
        self.assertIn(
            "make -f Makefile -f mk/manual-recording-metadata-tests.mk "
            "test-manual-recording-metadata-revision", workflow)
        self.assertIn('getManualRecordingMetadataForBackend("default")', cpp)
        source = (ROOT / "core" / "recordings" / "src" /
                  "ManualRecordingMetadataRepositoryFacade.cpp").read_text(
                      encoding="utf-8")
        self.assertIn("ensureManualPersonProfileSchema()", source)
        self.assertIn("idx_suite_metadata_person_profiles_path", cpp)
        self.assertIn("sqlite3_backup_init(", cpp)
        self.assertIn("p7_profile_prior", cpp)


if __name__ == "__main__":
    unittest.main()

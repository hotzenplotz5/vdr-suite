#!/usr/bin/env python3
"""P7 schema-initializer coverage contract, read-only source inspection.

This inventory covers C++ files named *Repository.cpp or
*RepositorySchema.cpp under core/. Other SQL writers (including helpers,
initialization code and nested .inc files) require a separate audit.
A file marked covered is exercised for schema idempotence and synthetic row
preservation; that does not approve manual-install migration or runtime auth.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parent.parent
# Every discovered named Repository::ensureSchema + CREATE TABLE owner must
# be classified. Adding a new owner without review breaks this focused gate.
EXERCISED_BY_P7_CPP = frozenset((
    "core/media/src/ContinueWatchingRepository.cpp",
    "core/media/src/MediaSessionRepository.cpp",
    "core/media/src/MediaTranscodeBackendSettingsRepository.cpp",
    "core/media/src/RecentlyWatchedRepository.cpp",
    "core/metadata/src/RecordingSeriesHierarchyOverrideRepository.cpp",
    "core/operations/src/MutationOperationRepository.cpp",
    "core/recordings/src/PublicRecordingIdentityRepository.cpp",
    "core/security/src/AccountabilityEventRepository.cpp",
    "core/security/src/BrowserSessionCredentialRepository.cpp",
    "core/security/src/CredentialVerifierRepository.cpp",
    "core/security/src/DeviceCredentialVerifierRepository.cpp",
    "core/security/src/DevicePairingRequestRepository.cpp",
    "core/security/src/HumanAccountRepository.cpp",
    "core/security/src/SecurityIdentityRepository.cpp",
    "core/security/src/SecurityPermissionGrantRepository.cpp",
    "core/timers/src/TimerAssignmentRepository.cpp",
    "core/timers/src/TimerIntentRepository.cpp",
    "core/vdr/src/EpgArtworkRepository.cpp",
    "core/vdr/src/EpgEventRepository.cpp",
    "core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp",
    "core/vdr/src/GlobalSearchRepository.cpp",
    "core/vdr/src/VdrChannelCacheRepository.cpp",
    "core/vdr/src/VdrRecordingCacheRepository.cpp",
))
NOT_YET_EXERCISED_BY_P7_CPP = frozenset((
    "core/agent/src/BackendAgentRepository.cpp",
    "core/agent/src/EmbeddedRecordingCutRepository.cpp",
    "core/agent/src/EmbeddedRecordingMarksRepository.cpp",
    "core/metadata/src/ManualRecordingMetadataAssignmentRepository.cpp",
    "core/security/src/FirstAdminBootstrapRepository.cpp",
    "core/security/src/HumanAccountCreationRepository.cpp",
    "core/timers/src/NativeTimerBindingRepository.cpp",
    "core/vdr/src/EpgSeriesArtworkFallbackRepository.cpp",
    "core/vdr/src/EpgSeriesArtworkProviderCacheRepository.cpp",
    "core/vdr/src/VdrRecordingNativeMetadataRepositorySchema.cpp",
))
ALL_CLASSIFIED = EXERCISED_BY_P7_CPP | NOT_YET_EXERCISED_BY_P7_CPP


def find_named_repository_schema_owners(root=ROOT):
    results = set()
    for candidate in (root / "core").glob("*/src/*Repository*.cpp"):
        if not (candidate.name.endswith("Repository.cpp") or
                candidate.name.endswith("RepositorySchema.cpp")):
            continue
        source = candidate.read_text(encoding="utf-8")
        if (re.search(r"::ensureSchema\s*\(\s*\)", source) and
                "CREATE TABLE" in source):
            results.add(candidate.relative_to(root).as_posix())
    return frozenset(results)


class P7SchemaOwnerInventoryTests(unittest.TestCase):
    def test_all_named_repository_schema_initializers_are_classified(self):
        discovered = find_named_repository_schema_owners()
        self.assertEqual(discovered, ALL_CLASSIFIED,
                         "New/removed schema owner: classify and review P7 coverage")
        self.assertFalse(EXERCISED_BY_P7_CPP & NOT_YET_EXERCISED_BY_P7_CPP)

    def test_covered_initializers_are_compiled_into_real_p7_rehearsal(self):
        build = (ROOT / "mk" / "metadata-tests.mk").read_text(
            encoding="utf-8")
        harness = (ROOT / "core" / "metadata" / "tests" /
                   "test_packaging_p7_runtime_schema.cpp").read_text(
            encoding="utf-8")
        for path in sorted(EXERCISED_BY_P7_CPP):
            with self.subTest(owner=path):
                self.assertIn(path + " \\", build)
                self.assertIn(Path(path).stem, harness)
        # This initializer lives in a C++ .inc and is compiled through an
        # established Make variable, not through a direct source path above.
        self.assertIn("$(METADATA_GENRE_SRC)", build)
        self.assertIn("GenreIndexRepository", harness)
        self.assertTrue((ROOT / "core" / "metadata" / "src" /
                         "GenreIndexRepositorySchema.inc").is_file())

    def test_uncovered_owners_remain_explicit_and_nonzero(self):
        # P7 is intentionally not a complete compatibility gate yet.
        self.assertTrue(NOT_YET_EXERCISED_BY_P7_CPP)
        self.assertEqual(len(EXERCISED_BY_P7_CPP), 23)


if __name__ == "__main__":
    unittest.main()

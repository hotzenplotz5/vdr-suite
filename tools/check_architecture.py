#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

SOURCE_DIRS = [
    ROOT / "apps",
    ROOT / "api",
    ROOT / "core",
]

SOURCE_SUFFIXES = {
    ".h",
    ".hpp",
    ".cpp",
    ".cc",
    ".cxx",
}

SQLITE_INFRASTRUCTURE_PREFIXES = [
    "core/sqlite/",
]

SQLITE_DOMAIN_REPOSITORY_PREFIXES = [
    "core/recordings/src/",
    "core/vdr/src/",
    "core/metadata/src/",
    "core/security/src/",
    "core/agent/src/",
    "core/timers/src/",
    "core/operations/src/",
    "core/media/src/",
]

SQLITE_SPLIT_REPOSITORY_FAMILIES = [
    (
        "core/vdr/src/",
        "VdrRecordingNativeMetadataRepository",
    ),
    (
        "core/media/src/",
        "MediaSessionRepository",
    ),
]

SQLITE_SPLIT_REPOSITORY_FILES = {
    "core/agent/src/BackendAgentCommandDelivery.cpp",
    "core/agent/src/BackendAgentCommandReservation.cpp",
    "core/agent/src/BackendAgentNativeTimerDeleteAssignment.cpp",
    "core/agent/src/BackendAgentRecordingMarksModifyReconciliation.cpp",
    "core/recordings/src/ManualRecordingMetadataRepositoryFacade.cpp",
}

SQLITE_ALLOWED_RUNTIME_ADAPTERS = {
    "api/rest/src/GenreBrowserApiRuntime.cpp",
    "core/daemon/src/SeriesArtworkBackendSettingsService.cpp",
}

SQLITE_ALLOWED_CONTRACT_TESTS = {
    "api/rest/tests/test_vdr_recording_folder_controller.cpp",
    "core/agent/tests/test_backend_agent_lifecycle.cpp",
    "core/daemon/tests/test_daemon_sqlite_shutdown_cancellation.cpp",
    "core/daemon/tests/test_series_artwork_backend_settings_service.cpp",
    "core/metadata/tests/test_genre_epg_refresh_fast_path.cpp",
    "core/metadata/tests/test_genre_recording_sync_noop.cpp",
    "core/metadata/tests/test_genre_write_batching.cpp",
    "core/metadata/tests/test_manual_recording_metadata_assignment_repository.cpp",
    "core/metadata/tests/test_metadata_schema_contract.cpp",
    "core/recordings/tests/test_manual_recording_metadata_revision.cpp",
    "core/vdr/tests/test_epg_artwork_repository.cpp",
    "core/vdr/tests/test_epg_event_repository.cpp",
    "core/vdr/tests/test_global_search_repository.cpp",
    "core/vdr/tests/test_vdr_recording_native_person_search_service.cpp",
}


def repo_path(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def is_source_file(path: Path) -> bool:
    return path.is_file() and path.suffix in SOURCE_SUFFIXES


def is_sqlite_infrastructure_file(path: Path) -> bool:
    rel = repo_path(path)
    return any(
        rel.startswith(prefix)
        for prefix in SQLITE_INFRASTRUCTURE_PREFIXES
    )


def is_domain_repository_implementation(path: Path) -> bool:
    rel = repo_path(path)

    if not any(
        rel.startswith(prefix)
        for prefix in SQLITE_DOMAIN_REPOSITORY_PREFIXES
    ):
        return False

    return path.name.endswith("Repository.cpp")


def is_split_repository_implementation(path: Path) -> bool:
    rel = repo_path(path)
    if rel in SQLITE_SPLIT_REPOSITORY_FILES:
        return True

    return any(
        rel.startswith(directory_prefix) and
        path.name.startswith(repository_prefix)
        for directory_prefix, repository_prefix
        in SQLITE_SPLIT_REPOSITORY_FAMILIES
    )


def is_registered_runtime_adapter(path: Path) -> bool:
    return repo_path(path) in SQLITE_ALLOWED_RUNTIME_ADAPTERS


def is_registered_sqlite_contract_test(path: Path) -> bool:
    return repo_path(path) in SQLITE_ALLOWED_CONTRACT_TESTS


def is_allowed_sqlite_file(path: Path) -> bool:
    return (
        is_sqlite_infrastructure_file(path)
        or is_domain_repository_implementation(path)
        or is_split_repository_implementation(path)
        or is_registered_runtime_adapter(path)
        or is_registered_sqlite_contract_test(path)
    )


def collect_source_files() -> list[Path]:
    result = []

    for directory in SOURCE_DIRS:
        if not directory.exists():
            continue

        for path in directory.rglob("*"):
            if is_source_file(path):
                result.append(path)

    return sorted(result)


def check_sqlite_boundary(path: Path, text: str) -> list[str]:
    errors = []

    if "sqlite3.h" not in text and "sqlite3_" not in text:
        return errors

    if is_allowed_sqlite_file(path):
        return errors

    errors.append(
        f"{repo_path(path)}: direct SQLite usage is only allowed in "
        "core/sqlite/, approved domain repository implementation units, "
        "explicitly registered runtime adapters, and explicitly registered "
        "SQLite/schema contract tests"
    )

    return errors


def check_sqlite_boundary_contract() -> list[str]:
    errors = []
    sqlite_source = "#include <sqlite3.h>\nsqlite3_stmt* stmt = nullptr;\n"

    allowed_paths = [
        "core/sqlite/src/Database.cpp",
        "core/recordings/src/RecordingRepository.cpp",
        "core/recordings/src/ManualRecordingMetadataRepositoryFacade.cpp",
        "core/vdr/src/EpgEventRepository.cpp",
        "core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp",
        "core/vdr/src/VdrRecordingCacheRepository.cpp",
        "core/vdr/src/VdrRecordingNativeMetadataRepositoryStorage.cpp",
        "core/vdr/src/VdrRecordingNativeMetadataRepositoryInternal.h",
        "core/metadata/src/MetadataEntityRepository.cpp",
        "core/security/src/SecurityIdentityRepository.cpp",
        "core/agent/src/BackendAgentCommandDelivery.cpp",
        "core/agent/src/BackendAgentCommandReservation.cpp",
        "core/agent/src/BackendAgentNativeTimerDeleteAssignment.cpp",
        "core/agent/src/BackendAgentRecordingMarksModifyReconciliation.cpp",
        "core/timers/src/TimerIntentRepository.cpp",
        "core/operations/src/MutationOperationRepository.cpp",
        "core/media/src/MediaSessionRepository.cpp",
        "core/media/src/MediaSessionRepositoryLive.cpp",
        "api/rest/src/GenreBrowserApiRuntime.cpp",
        "core/daemon/src/SeriesArtworkBackendSettingsService.cpp",
        "api/rest/tests/test_vdr_recording_folder_controller.cpp",
        "core/daemon/tests/test_series_artwork_backend_settings_service.cpp",
        "core/metadata/tests/test_manual_recording_metadata_assignment_repository.cpp",
        "core/metadata/tests/test_metadata_schema_contract.cpp",
        "core/recordings/tests/test_manual_recording_metadata_revision.cpp",
        "core/vdr/tests/test_epg_event_repository.cpp",
        "core/vdr/tests/test_vdr_recording_native_person_search_service.cpp",
    ]

    rejected_paths = [
        "core/recordings/src/RecordingActionService.cpp",
        "core/recordings/src/ManualRecordingMetadataRepositoryFacadeHelper.cpp",
        "core/vdr/src/VdrService.cpp",
        "core/vdr/src/RepositoryHelper.cpp",
        "core/metadata/src/MetadataResolver.cpp",
        "core/security/include/SecurityIdentityRepository.h",
        "core/security/src/RepositoryHelper.cpp",
        "core/agent/src/BackendAgentCommandDeliveryHelper.cpp",
        "core/agent/src/BackendAgentNativeTimerDeleteAssignmentHelper.cpp",
        "core/timers/src/TimerIntentPersistence.cpp",
        "core/timers/src/RepositoryHelper.cpp",
        "core/operations/src/MutationOperationPersistence.cpp",
        "core/operations/src/RepositoryHelper.cpp",
        "core/media/src/MediaSessionPersistence.cpp",
        "core/media/src/RepositoryHelper.cpp",
        "core/daemon/src/SeriesArtworkBackendSettingsHelper.cpp",
        "core/metadata/tests/test_metadata_identity.cpp",
        "api/rest/src/FakeRepository.cpp",
        "apps/example/src/FakeRepository.cpp",
    ]

    for rel in allowed_paths:
        path = ROOT / rel
        path_errors = check_sqlite_boundary(path, sqlite_source)
        if path_errors:
            errors.append(
                "SQLite boundary contract expected allowed path: " + rel
            )

    for rel in rejected_paths:
        path = ROOT / rel
        path_errors = check_sqlite_boundary(path, sqlite_source)
        if len(path_errors) != 1:
            errors.append(
                "SQLite boundary contract expected rejected path: " + rel
            )

    return errors


def check_protected_mutation_outcome_contract() -> list[str]:
    errors = []
    gate_path = ROOT / "core/security/include/SecurityHttpGate.h"
    server_path = ROOT / "core/http/src/TestHttpServer.cpp"

    gate_text = gate_path.read_text(encoding="utf-8")
    server_text = server_path.read_text(encoding="utf-8")

    required_gate_markers = [
        "AuthorizationDecision authorizationDecision;",
        "std::string operationId;",
        "gate.authorizationDecision = decision;",
        "gate.operationId = operationId;",
        "bool appendProtectedMutationOutcome(",
        "!gate.protectedMutation",
        "!gate.authorizationDecision.allowed",
        '"operation.succeeded"',
        '"operation.failed"',
        '"http_status_" + std::to_string(statusCode)',
        'event.outcome = succeeded ? "succeeded" : "failed";',
    ]
    for marker in required_gate_markers:
        if marker not in gate_text:
            errors.append(
                "protected mutation outcome gate missing marker: " + marker
            )

    required_server_markers = [
        "apiRouter_.handleClientPost(",
        "gate.protectedMutation &&",
        "appendProtectedMutationOutcome(",
        "outcomeAccountabilityUnavailableResponse(",
    ]
    for marker in required_server_markers:
        if marker not in server_text:
            errors.append(
                "protected mutation outcome server missing marker: " + marker
            )

    dispatch_position = server_text.find("apiRouter_.handleClientPost(")
    outcome_position = server_text.find(
        "appendProtectedMutationOutcome(",
        dispatch_position)
    final_response_position = server_text.find(
        "return finalizeResponse(",
        outcome_position)
    if not (
        dispatch_position >= 0 and
        outcome_position > dispatch_position and
        final_response_position > outcome_position
    ):
        errors.append(
            "protected mutation outcome must run after POST dispatch and before the final response"
        )

    outcome_start = gate_text.find(
        "    bool appendProtectedMutationOutcome(")
    outcome_end = gate_text.find(
        "\n    HttpServerResponse outcomeAccountabilityUnavailableResponse(",
        outcome_start)
    if outcome_start < 0 or outcome_end < 0:
        errors.append(
            "protected mutation outcome method boundary is missing"
        )
    else:
        outcome_text = gate_text[outcome_start:outcome_end]
        forbidden_outcome_inputs = [
            "request.headers",
            "request.body",
            "apiResponse.body",
            "response.body",
            "sqlite3_",
            "ensureSchema",
            "getenv",
        ]
        for token in forbidden_outcome_inputs:
            if token in outcome_text:
                errors.append(
                    "protected mutation outcome uses forbidden input: " + token
                )

    combined_text = gate_text + "\n" + server_text
    forbidden_scope_markers = [
        "/api/security/accountability/events",
        "security.audit.read",
    ]
    for marker in forbidden_scope_markers:
        if marker in combined_text:
            errors.append(
                "protected mutation outcome introduced forbidden scope: " + marker
            )

    return errors


def main() -> int:
    errors = check_sqlite_boundary_contract()
    errors.extend(check_protected_mutation_outcome_contract())

    for path in collect_source_files():
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue

        errors.extend(check_sqlite_boundary(path, text))

    if errors:
        print("Architecture check failed:")
        for error in errors:
            print(f"- {error}")
        print(f"\nTotal errors: {len(errors)}")
        return 1

    print("Architecture check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

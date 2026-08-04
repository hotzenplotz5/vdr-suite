#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    path = ROOT / relative
    if not path.is_file():
        raise AssertionError(f"missing required file: {relative}")
    return path.read_text(encoding="utf-8")


def require(content: str, needle: str, owner: str) -> None:
    if needle not in content:
        raise AssertionError(f"{owner}: missing required contract: {needle}")


def forbid(content: str, needle: str, owner: str) -> None:
    if needle in content:
        raise AssertionError(f"{owner}: forbidden provider coupling: {needle}")


def main() -> int:
    frontend = read("web/frontend/recordings2-metadata-assignment.js")
    for operation in ("search", "seasons", "episodes", "assign", "withdraw"):
        require(frontend, f"'{operation}'", "Recordings 2 assignment runtime")
    require(frontend, "VdrSuiteBrowserSession", "Recordings 2 assignment runtime")
    for forbidden in (
        "api.themoviedb.org",
        "image.tmdb.org",
        "VDR_SUITE_TMDB_READ_ACCESS_TOKEN",
        "tvscraper2.db",
    ):
        forbid(frontend, forbidden, "Recordings 2 assignment runtime")

    detail = read("web/frontend/recordings2-metadata-detail.js")
    require(detail, "VdrSuitePublicUrl", "Recordings 2 metadata detail")
    require(detail, "resolvePath", "Recordings 2 metadata detail")
    require(
        detail,
        "/frontend/recordings2-metadata-assignment.js",
        "Recordings 2 metadata detail",
    )

    runtime = read("api/rest/src/ManualRecordingMetadataApiRuntime.cpp")
    for operation in ("manual", "search", "seasons", "episodes", "assign", "withdraw"):
        require(runtime, f'route.operation == "{operation}"', "manual metadata API runtime")
    require(runtime, "MaximumBodyBytes", "manual metadata API runtime")
    require(runtime, "actorRef.empty()", "manual metadata API runtime")

    security = read("core/security/include/SecurityHttpGate.h")
    require(security, "metadata.recording.assign", "security gate")
    require(security, "manualRecordingMetadataRoute", "security gate")
    require(security, "manualMetadataBackendId", "security gate")
    require(security, "manualMetadataOperation", "security gate")

    authorization = read("core/security/include/AuthorizationService.h")
    require(authorization, "metadata.recording.assign", "authorization role policy")
    require(authorization, "role.read-only", "authorization role policy")
    require(authorization, "role.admin", "authorization role policy")

    repository = read(
        "core/metadata/src/ManualRecordingMetadataAssignmentRepository.cpp"
    )
    for table in (
        "suite_metadata_manual_assignment_values",
        "suite_metadata_manual_assignment_withdrawals",
        "suite_metadata_evidence",
        "suite_metadata_assignments",
        "suite_metadata_assignment_evidence",
        "suite_metadata_entity_external_ids",
    ):
        require(repository, table, "manual assignment repository")
    require(repository, "manual-override", "manual assignment repository")
    require(repository, "relationship_locked", "manual assignment repository")
    require(repository, '"superseded"', "manual assignment repository")
    require(repository, '"withdrawn"', "manual assignment repository")

    facade = read("core/recordings/src/ManualRecordingMetadataRepositoryFacade.cpp")
    require(facade, "vdr_recording_cache", "recording identity facade")
    require(facade, "backend_native_id", "recording identity facade")
    require(facade, "cache_key", "recording identity facade")

    read_model = read("api/rest/src/VdrRecordingFolderController.cpp")
    require(
        read_model,
        "manual.found && manual.relationshipLocked",
        "recording metadata read model",
    )
    require(
        read_model,
        "/var/cache/vdr-suite/recording-metadata/posters",
        "recording metadata read model",
    )

    provider = read("core/metadata/src/TmdbRecordingMetadataCandidateProvider.cpp")
    require(provider, "https://api.themoviedb.org/3", "TMDB candidate provider")
    require(provider, "maximumResponseBytes", "TMDB candidate provider")
    require(provider, "retryAfterSeconds", "TMDB candidate provider")

    poster = read("core/metadata/src/TmdbRecordingMetadataPosterMaterializer.cpp")
    for contract in (
        "O_NOFOLLOW",
        "O_EXCL",
        "fsync",
        "renameat",
        "maximumImageBytes",
        "image/jpeg",
        "image/png",
        "image/webp",
    ):
        require(poster, contract, "selected poster materializer")

    recordings_make = read("mk/recordings2.mk")
    require(
        recordings_make,
        "recordings2-metadata-assignment.js",
        "Recordings 2 install staging",
    )
    runtime_make = read("mk/manual-recording-metadata.mk")
    require(
        runtime_make,
        "recording-metadata/posters",
        "manual metadata cache install",
    )
    makefile = read("Makefile")
    require(
        makefile,
        "include mk/manual-recording-metadata.mk",
        "top-level build graph",
    )

    adr = read("docs/adr/ADR-0051-manual-recording-metadata-assignment.md")
    require(adr, "TVScraper remains read-only", "ADR-0051")
    require(adr, "metadata.recording.assign", "ADR-0051")
    require(adr, "relationship-locked", "ADR-0051")
    development = read("docs/development/manual-recording-metadata-assignment.md")
    require(development, "Final real-system acceptance", "acceptance document")
    require(development, "Manuelle Zuordnung entfernen", "acceptance document")

    print("manual recording metadata architecture contract ok")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"manual recording metadata architecture check failed: {error}", file=sys.stderr)
        raise SystemExit(1)

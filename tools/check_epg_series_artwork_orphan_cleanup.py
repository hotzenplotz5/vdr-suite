#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CLEANER_HEADER = ROOT / "core/vdr/include/EpgSeriesArtworkFallbackOrphanCleaner.h"
CLEANER_SOURCE = ROOT / "core/vdr/src/EpgSeriesArtworkFallbackOrphanCleaner.cpp"
REPOSITORY_HEADER = ROOT / "core/vdr/include/EpgSeriesArtworkFallbackRepository.h"
REPOSITORY_SOURCE = ROOT / "core/vdr/src/EpgSeriesArtworkFallbackRepository.cpp"
RUNTIME_CONFIG_HEADER = ROOT / "core/daemon/include/RuntimeConfig.h"
RUNTIME_CONFIG_SOURCE = ROOT / "core/daemon/src/RuntimeConfig.cpp"
RUNTIME_CONTEXT = ROOT / "core/daemon/src/DaemonRuntimeBackendContext.cpp"
INITIALIZATION = ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp"
PACKAGED_DEFAULTS = ROOT / "packaging/systemd/vdr-suite-daemon.default"
PUBLIC_SERIALIZER = ROOT / "core/vdr/src/EpgScraperMetadataPublicJsonSerializer.cpp"
PUBLIC_CONTROLLER = ROOT / "api/rest/src/EpgCacheController.cpp"

paths = (
    CLEANER_HEADER,
    CLEANER_SOURCE,
    REPOSITORY_HEADER,
    REPOSITORY_SOURCE,
    RUNTIME_CONFIG_HEADER,
    RUNTIME_CONFIG_SOURCE,
    RUNTIME_CONTEXT,
    INITIALIZATION,
    PACKAGED_DEFAULTS,
    PUBLIC_SERIALIZER,
    PUBLIC_CONTROLLER,
)

errors = []
for path in paths:
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

cleaner_header = CLEANER_HEADER.read_text(encoding="utf-8")
cleaner_source = CLEANER_SOURCE.read_text(encoding="utf-8")
repository_header = REPOSITORY_HEADER.read_text(encoding="utf-8")
repository_source = REPOSITORY_SOURCE.read_text(encoding="utf-8")
runtime_config_header = RUNTIME_CONFIG_HEADER.read_text(encoding="utf-8")
runtime_config_source = RUNTIME_CONFIG_SOURCE.read_text(encoding="utf-8")
runtime_context = RUNTIME_CONTEXT.read_text(encoding="utf-8")
initialization = INITIALIZATION.read_text(encoding="utf-8")
packaged_defaults = PACKAGED_DEFAULTS.read_text(encoding="utf-8")
public_serializer = PUBLIC_SERIALIZER.read_text(encoding="utf-8")
public_controller = PUBLIC_CONTROLLER.read_text(encoding="utf-8")

required_header = (
    "EpgSeriesArtworkFallbackOrphanCleanupConfig",
    "bool enabled = false;",
    "minimumAgeSeconds",
    "maximumFilesPerRun",
    "EpgSeriesArtworkFallbackOrphanCleanupResult",
    "std::size_t removedFiles",
)
for fragment in required_header:
    if fragment not in cleaner_header:
        errors.append(f"missing orphan cleanup contract: {fragment}")

required_cleaner = (
    "validHexComponent(",
    "O_NOFOLLOW",
    "::openat(",
    "::fstat(",
    "::fstatat(",
    "AT_SYMLINK_NOFOLLOW",
    "sameFileIdentity(",
    "referenceStateForPath(",
    "EpgSeriesArtworkFallbackPathReferenceState::Error",
    "EpgSeriesArtworkFallbackPathReferenceState::Referenced",
    '"series.png"',
    '"series.jpg"',
    "::unlinkat(",
    "AT_REMOVEDIR",
    "config.maximumFilesPerRun",
    "config.minimumAgeSeconds",
)
for fragment in required_cleaner:
    if fragment not in cleaner_source:
        errors.append(f"missing guarded orphan cleanup behavior: {fragment}")

required_repository = (
    "enum class EpgSeriesArtworkFallbackPathReferenceState",
    "Error,",
    "Unreferenced,",
    "Referenced",
    "referenceStateForPath(",
)
for fragment in required_repository:
    if fragment not in repository_header:
        errors.append(f"missing path-reference repository contract: {fragment}")

for fragment in (
    '"SELECT 1 FROM epg_series_artwork_fallback "',
    '"WHERE path=? LIMIT 1;"',
    "EpgSeriesArtworkFallbackPathReferenceState::Error",
    "EpgSeriesArtworkFallbackPathReferenceState::Unreferenced",
    "EpgSeriesArtworkFallbackPathReferenceState::Referenced",
):
    if fragment not in repository_source:
        errors.append(f"missing exact persisted path lookup: {fragment}")

required_runtime_config = (
    "orphanCleanupEnabled = false",
    "orphanCleanupMinimumAgeSeconds = 7 * 24 * 60 * 60",
    "orphanCleanupMaximumFiles = 64",
)
for fragment in required_runtime_config:
    if fragment not in runtime_config_header:
        errors.append(f"missing disabled bounded runtime default: {fragment}")

for fragment in (
    '"VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_ENABLED"',
    '"VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_MIN_AGE_SECONDS"',
    '"VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_MAX_FILES"',
    "60 * 60,",
    "365 * 24 * 60 * 60",
    "1024",
):
    if fragment not in runtime_config_source:
        errors.append(f"missing bounded runtime parsing: {fragment}")

required_runtime = (
    "runtimeFallbackConfig.orphanCleanupEnabled",
    "EpgSeriesArtworkFallbackOrphanCleaner orphanCleaner(",
    "orphanCleaner.cleanup(nowEpochSeconds)",
    "orphanCleanupResult.removedFiles",
    "orphanCleanupResult.skippedUnsafeEntries",
)
for fragment in required_runtime:
    if fragment not in runtime_context:
        errors.append(f"missing startup cleanup wiring: {fragment}")

schema_position = runtime_context.find(
    "epgSeriesArtworkFallbackRepository->ensureSchema()")
cleanup_position = runtime_context.find(
    "EpgSeriesArtworkFallbackOrphanCleaner orphanCleaner(")
materializer_position = runtime_context.find(
    "FilesystemSeriesArtworkFallbackMaterializerConfig")
if min(schema_position, cleanup_position, materializer_position) < 0:
    errors.append("startup cleanup ordering cannot be checked")
elif not schema_position < cleanup_position < materializer_position:
    errors.append(
        "orphan cleanup must run after schema readiness and before materializer construction"
    )

context_creation = initialization.find("createBackendRuntimeContext(")
agent_start = initialization.find("suiteBridgeAgentRuntime->start()")
if min(context_creation, agent_start) < 0:
    errors.append("startup concurrency boundary cannot be checked")
elif context_creation >= agent_start:
    errors.append(
        "backend context startup cleanup must complete before SuiteBridge Agent start"
    )

for fragment in (
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_ENABLED=false",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_MIN_AGE_SECONDS=604800",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ORPHAN_CLEANUP_MAX_FILES=64",
):
    if fragment not in packaged_defaults:
        errors.append(f"missing packaged offline cleanup default: {fragment}")

for forbidden in (
    "recursive_directory_iterator",
    "remove_all",
    "std::filesystem::remove",
    "weakly_canonical",
    "incoming",
    "api.themoviedb.org",
    "image.tmdb.org",
    "Authorization",
    "/api/",
):
    if forbidden in cleaner_source:
        errors.append(f"forbidden orphan cleanup coupling: {forbidden}")

for forbidden in (
    "ORPHAN_CLEANUP",
    "OrphanCleaner",
    "orphanCleanup",
):
    if forbidden in public_serializer or forbidden in public_controller:
        errors.append(f"forbidden public orphan cleanup exposure: {forbidden}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("epg series artwork orphan cleanup contract ok")

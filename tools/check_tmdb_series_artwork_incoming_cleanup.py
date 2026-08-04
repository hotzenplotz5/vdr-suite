#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CLEANER_HEADER = ROOT / "core/vdr/include/TmdbSeriesArtworkIncomingCleaner.h"
CLEANER_SOURCE = ROOT / "core/vdr/src/TmdbSeriesArtworkIncomingCleaner.cpp"
RUNTIME_CONFIG_HEADER = ROOT / "core/daemon/include/RuntimeConfig.h"
RUNTIME_CONFIG_SOURCE = ROOT / "core/daemon/src/RuntimeConfig.cpp"
RUNTIME_CONTEXT = ROOT / "core/daemon/src/DaemonRuntimeBackendContext.cpp"
INITIALIZATION = ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp"
PACKAGED_DEFAULTS = ROOT / "packaging/systemd/vdr-suite-daemon.default"
PROVIDER_SOURCE = ROOT / "core/vdr/src/TmdbSeriesArtworkProvider.cpp"
PUBLIC_SERIALIZER = ROOT / "core/vdr/src/EpgScraperMetadataPublicJsonSerializer.cpp"
PUBLIC_CONTROLLER = ROOT / "api/rest/src/EpgCacheController.cpp"

paths = (
    CLEANER_HEADER,
    CLEANER_SOURCE,
    RUNTIME_CONFIG_HEADER,
    RUNTIME_CONFIG_SOURCE,
    RUNTIME_CONTEXT,
    INITIALIZATION,
    PACKAGED_DEFAULTS,
    PROVIDER_SOURCE,
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
runtime_config_header = RUNTIME_CONFIG_HEADER.read_text(encoding="utf-8")
runtime_config_source = RUNTIME_CONFIG_SOURCE.read_text(encoding="utf-8")
runtime_context = RUNTIME_CONTEXT.read_text(encoding="utf-8")
initialization = INITIALIZATION.read_text(encoding="utf-8")
packaged_defaults = PACKAGED_DEFAULTS.read_text(encoding="utf-8")
provider_source = PROVIDER_SOURCE.read_text(encoding="utf-8")
public_serializer = PUBLIC_SERIALIZER.read_text(encoding="utf-8")
public_controller = PUBLIC_CONTROLLER.read_text(encoding="utf-8")

for fragment in (
    "TmdbSeriesArtworkIncomingCleanupConfig",
    "bool enabled = false;",
    "minimumAgeSeconds = 24 * 60 * 60",
    "maximumFilesPerRun = 64U",
    "TmdbSeriesArtworkIncomingCleanupResult",
    "removedCandidateFiles",
    "removedTemporaryFiles",
    "skippedForeignEntries",
    "skippedUnsafeEntries",
):
    if fragment not in cleaner_header:
        errors.append(f"missing incoming cleanup contract: {fragment}")

for fragment in (
    'Prefix = "tmdb-"',
    'Suffix = ".candidate"',
    'TemporarySuffix = ".tmp"',
    "lowercaseHex16(",
    "positiveSeriesId(",
    "O_NOFOLLOW",
    "O_NONBLOCK",
    "::openat(",
    "::fstat(",
    "::fstatat(",
    "AT_SYMLINK_NOFOLLOW",
    "sameIdentity(",
    "::unlinkat(",
    "config_.minimumAgeSeconds",
    "config_.maximumFilesPerRun",
    "std::sort(names.begin(), names.end())",
):
    if fragment not in cleaner_source:
        errors.append(f"missing guarded incoming cleanup behavior: {fragment}")

for forbidden in (
    "recursive_directory_iterator",
    "remove_all",
    "std::filesystem::remove",
    "weakly_canonical",
    "sqlite",
    "Database",
    "api.themoviedb.org",
    "image.tmdb.org",
    "Authorization",
    "/api/",
):
    if forbidden in cleaner_source:
        errors.append(f"forbidden incoming cleanup coupling: {forbidden}")

for fragment in (
    '"tmdb-" + std::to_string(seriesId) + "-"',
    '".candidate"',
    '"." + finalName + "."',
    '".tmp"',
):
    if fragment not in provider_source:
        errors.append(f"provider namespace drifted from cleaner contract: {fragment}")

for fragment in (
    "incomingCleanupEnabled = false",
    "incomingCleanupMinimumAgeSeconds = 24 * 60 * 60",
    "incomingCleanupMaximumFiles = 64",
):
    if fragment not in runtime_config_header:
        errors.append(f"missing disabled incoming cleanup default: {fragment}")

for fragment in (
    '"VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_ENABLED"',
    '"VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_MIN_AGE_SECONDS"',
    '"VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_MAX_FILES"',
    "60 * 60,",
    "365 * 24 * 60 * 60",
    "1024",
):
    if fragment not in runtime_config_source:
        errors.append(f"missing bounded incoming cleanup parsing: {fragment}")

for fragment in (
    "runtimeFallbackConfig.incomingCleanupEnabled",
    "TmdbSeriesArtworkIncomingCleaner incomingCleaner(",
    "incomingCleaner.cleanup(nowEpochSeconds)",
    "incomingCleanupResult.removedCandidateFiles",
    "incomingCleanupResult.removedTemporaryFiles",
    "incomingCleanupResult.skippedForeignEntries",
    "incomingCleanupResult.skippedUnsafeEntries",
):
    if fragment not in runtime_context:
        errors.append(f"missing incoming startup cleanup wiring: {fragment}")

config_position = runtime_context.find(
    "TmdbSeriesArtworkRuntimeConfig::fromEnvironment(")
cleanup_position = runtime_context.find(
    "TmdbSeriesArtworkIncomingCleaner incomingCleaner(")
provider_boundary_position = runtime_context.find(
    "std::make_unique<SeriesArtworkBackendSettingsService>(")
if min(config_position, cleanup_position, provider_boundary_position) < 0:
    errors.append("incoming startup cleanup ordering cannot be checked")
elif not config_position < cleanup_position < provider_boundary_position:
    errors.append(
        "incoming cleanup must run after root resolution and before dynamic provider construction"
    )

context_creation = initialization.find("createBackendRuntimeContext(")
agent_start = initialization.find("suiteBridgeAgentRuntime->start()")
if min(context_creation, agent_start) < 0:
    errors.append("incoming cleanup concurrency boundary cannot be checked")
elif context_creation >= agent_start:
    errors.append(
        "backend context incoming cleanup must complete before SuiteBridge Agent start"
    )

for fragment in (
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_ENABLED=false",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_MIN_AGE_SECONDS=86400",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_INCOMING_CLEANUP_MAX_FILES=64",
):
    if fragment not in packaged_defaults:
        errors.append(f"missing packaged incoming cleanup default: {fragment}")

for forbidden in (
    "INCOMING_CLEANUP",
    "IncomingCleaner",
    "incomingCleanup",
):
    if forbidden in public_serializer or forbidden in public_controller:
        errors.append(f"forbidden public incoming cleanup exposure: {forbidden}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("tmdb series artwork incoming cleanup contract ok")

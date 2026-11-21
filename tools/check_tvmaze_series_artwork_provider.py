#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/vdr/include/TvmazeSeriesArtworkProvider.h"
SOURCE = ROOT / "core/vdr/src/TvmazeSeriesArtworkProvider.cpp"
JSON_HEADER = ROOT / "core/vdr/include/TvmazeSeriesArtworkJson.h"
JSON_SOURCE = ROOT / "core/vdr/src/TvmazeSeriesArtworkJson.cpp"
RUNTIME_HEADER = ROOT / "core/daemon/include/TvmazeSeriesArtworkRuntimeConfig.h"
RUNTIME_SOURCE = ROOT / "core/daemon/src/TvmazeSeriesArtworkRuntimeConfig.cpp"
CONTEXT_HEADER = ROOT / "core/daemon/include/BackendRuntimeContext.h"
CONTEXT_SOURCE = ROOT / "core/daemon/src/DaemonRuntimeBackendContext.cpp"
SETTINGS_SERVICE = ROOT / "core/daemon/src/SeriesArtworkBackendSettingsService.cpp"
TRANSPORT_HEADER = ROOT / "core/http/include/CurlExternalArtworkHttpTransport.h"
TRANSPORT_SOURCE = ROOT / "core/http/src/CurlExternalArtworkHttpTransport.cpp"
DEFAULTS = ROOT / "packaging/systemd/vdr-suite-daemon.default"
ARCHITECTURE = ROOT / "docs/architecture/epg-series-artwork-tvmaze-provider.md"
NOTICE = ROOT / "docs/third-party-data-providers.md"

paths = (
    HEADER,
    SOURCE,
    JSON_HEADER,
    JSON_SOURCE,
    RUNTIME_HEADER,
    RUNTIME_SOURCE,
    CONTEXT_HEADER,
    CONTEXT_SOURCE,
    SETTINGS_SERVICE,
    TRANSPORT_HEADER,
    TRANSPORT_SOURCE,
    DEFAULTS,
    ARCHITECTURE,
    NOTICE,
)

errors = []
for path in paths:
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

source = SOURCE.read_text(encoding="utf-8")
json_source = JSON_SOURCE.read_text(encoding="utf-8")
runtime_header = RUNTIME_HEADER.read_text(encoding="utf-8")
runtime_source = RUNTIME_SOURCE.read_text(encoding="utf-8")
context_header = CONTEXT_HEADER.read_text(encoding="utf-8")
context_source = CONTEXT_SOURCE.read_text(encoding="utf-8")
settings_service = SETTINGS_SERVICE.read_text(encoding="utf-8")
transport_header = TRANSPORT_HEADER.read_text(encoding="utf-8")
transport_source = TRANSPORT_SOURCE.read_text(encoding="utf-8")
defaults = DEFAULTS.read_text(encoding="utf-8")
architecture = ARCHITECTURE.read_text(encoding="utf-8")
notice = NOTICE.read_text(encoding="utf-8")

for fragment in (
    'ApiBase = "https://api.tvmaze.com"',
    '"/lookup/shows?"',
    '"imdb"',
    '"thetvdb"',
    '"/shows/"',
    '"/images"',
    'result.artwork.provider = "tvmaze"',
    '"tvmaze-" + std::to_string(seriesId)',
    "O_NOFOLLOW",
    "::openat(",
    "::renameat(",
    "::fsync(",
    "maximumRetries",
    "negativeCacheTtlSeconds",
    "transientCacheTtlSeconds",
):
    if fragment not in source:
        errors.append(f"missing guarded TVmaze provider contract: {fragment}")

for forbidden in (
    "singlesearch",
    "search/shows",
    "metadata.title",
    "metadata.originalTitle",
    "bearerToken =",
    "Authorization",
    "http://static.tvmaze.com",
):
    if forbidden in source:
        errors.append(f"forbidden TVmaze provider behavior: {forbidden}")

for fragment in (
    'RelativePrefix = "/shows/"',
    'HttpsPrefix = "https://api.tvmaze.com/shows/"',
    'HttpPrefix = "http://api.tvmaze.com/shows/"',
    '"https://static.tvmaze.com/uploads/images/original_untouched/"',
    'type->string == "background"',
    'type->string != "poster"',
    'resolutions->member("original")',
    'original->member("width")',
    'original->member("height")',
):
    if fragment not in json_source:
        errors.append(f"missing strict TVmaze response contract: {fragment}")

for fragment in (
    '"api.tvmaze.com"',
    '"static.tvmaze.com"',
):
    if fragment not in transport_header:
        errors.append(f"missing exact TVmaze transport host: {fragment}")

for fragment in (
    'name == "location"',
    "response.location.clear()",
    "CURLOPT_FOLLOWLOCATION, 0L",
    "CURLOPT_PROXY, \"\"",
    "CURLOPT_NETRC, CURL_NETRC_IGNORED",
    "CURLOPT_OPENSOCKETFUNCTION",
):
    if fragment not in transport_source:
        errors.append(f"missing secure redirect/transport contract: {fragment}")

for fragment in (
    "bool selected = false",
    "retryBackoffMs = 500",
    "maximumJsonBytes = 2U * 1024U * 1024U",
):
    if fragment not in runtime_header:
        errors.append(f"missing TVmaze runtime default: {fragment}")

for fragment in (
    '"VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER"',
    '"tvmaze"',
    '"VDR_SUITE_TVMAZE_CONNECT_TIMEOUT_MS"',
    '"VDR_SUITE_TVMAZE_TOTAL_TIMEOUT_MS"',
    '"VDR_SUITE_TVMAZE_MAX_RETRIES"',
    '"VDR_SUITE_TVMAZE_RETRY_BACKOFF_MS"',
    '"VDR_SUITE_TVMAZE_NEGATIVE_CACHE_TTL_SECONDS"',
    '"VDR_SUITE_TVMAZE_TRANSIENT_CACHE_TTL_SECONDS"',
    '"VDR_SUITE_TVMAZE_MAX_JSON_BYTES"',
):
    if fragment not in runtime_source:
        errors.append(f"missing TVmaze runtime parsing: {fragment}")

if "SeriesArtworkBackendSettingsService> epgSeriesArtworkSettingsService" not in context_header:
    errors.append("missing dynamic series artwork settings ownership")

for fragment in (
    "TvmazeSeriesArtworkRuntimeConfig::fromEnvironment(",
    "settingsConfig.tvmaze.incomingRoot",
    "std::make_unique<SeriesArtworkBackendSettingsService>(",
    "fallbackProvider =",
    "epgSeriesArtworkSettingsService.get()",
):
    if fragment not in context_source:
        errors.append(f"missing dynamic TVmaze daemon wiring: {fragment}")

for fragment in (
    'settings.provider == "tvmaze"',
    "TvmazeSeriesArtworkProvider provider(",
    "provider.resolve(backendId, event, metadata)",
):
    if fragment not in settings_service:
        errors.append(f"missing backend-selected TVmaze dispatch: {fragment}")

active_defaults = {
    line.strip()
    for line in defaults.splitlines()
    if line.strip() and not line.lstrip().startswith("#")
}
for expected in (
    "VDR_SUITE_SUITE_BRIDGE_ENABLED=true",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_ENABLED=true",
    "VDR_SUITE_SERIES_ARTWORK_FALLBACK_PROVIDER=tvmaze",
    "VDR_SUITE_TVMAZE_CONNECT_TIMEOUT_MS=2000",
    "VDR_SUITE_TVMAZE_TOTAL_TIMEOUT_MS=8000",
    "VDR_SUITE_TVMAZE_MAX_RETRIES=1",
    "VDR_SUITE_TVMAZE_RETRY_BACKOFF_MS=500",
    "VDR_SUITE_TVMAZE_NEGATIVE_CACHE_TTL_SECONDS=21600",
    "VDR_SUITE_TVMAZE_TRANSIENT_CACHE_TTL_SECONDS=300",
    "VDR_SUITE_TVMAZE_MAX_JSON_BYTES=2097152",
):
    if expected not in active_defaults:
        errors.append(f"missing active packaged TVmaze default: {expected}")

if any(line.startswith("VDR_SUITE_TMDB_READ_ACCESS_TOKEN=")
       for line in active_defaults):
    errors.append("packaged defaults must not ship a TMDB token")

for fragment in (
    "token-free",
    "exact IMDb",
    "TheTVDB",
    "301",
    "TVmaze",
    "CC BY-SA",
):
    if fragment not in architecture and fragment not in notice:
        errors.append(f"missing TVmaze architecture/licensing statement: {fragment}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("tvmaze series artwork provider contract ok")

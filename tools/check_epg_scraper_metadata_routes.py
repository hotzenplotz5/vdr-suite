#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
ROUTER = ROOT / "api/rest/src/ApiRouter.cpp"
CONTROLLER = ROOT / "api/rest/include/EpgCacheController.h"
CONTROLLER_SOURCE = ROOT / "api/rest/src/EpgCacheController.cpp"
SERIALIZER = ROOT / "core/vdr/src/EpgScraperMetadataPublicJsonSerializer.cpp"
DELIVERY = ROOT / "core/vdr/src/EpgSeriesArtworkFallbackDeliveryService.cpp"
FALLBACK_REPOSITORY = ROOT / "core/vdr/src/EpgSeriesArtworkFallbackRepository.cpp"
HTTP_LISTENER = ROOT / "core/http/src/SimpleHttpListener.cpp"

errors = []
for path in (
    ROUTER,
    CONTROLLER,
    CONTROLLER_SOURCE,
    SERIALIZER,
    DELIVERY,
    FALLBACK_REPOSITORY,
    HTTP_LISTENER,
):
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

router = ROUTER.read_text(encoding="utf-8")
controller = CONTROLLER.read_text(encoding="utf-8")
controller_source = CONTROLLER_SOURCE.read_text(encoding="utf-8")
serializer = SERIALIZER.read_text(encoding="utf-8")
delivery = DELIVERY.read_text(encoding="utf-8")
fallback_repository = FALLBACK_REPOSITORY.read_text(encoding="utf-8")
http_listener = HTTP_LISTENER.read_text(encoding="utf-8")

required_router = (
    'path == "/api/epg/cache/metadata"',
    'path == "/api/epg/cache/metadata/image"',
    "epgCacheController_->getMetadata(",
    "epgCacheController_->getMetadataImage(",
    'queryParameters.get("backend")',
    'queryParameters.get("channelId")',
    'queryParameters.get("eventId")',
    'queryParameters.get("kind")',
    'queryParameters.getInt("index", -1)',
)

for fragment in required_router:
    if fragment not in router:
        errors.append(f"missing router contract: {fragment}")

metadata_route = router.find('path == "/api/epg/cache/metadata"')
metadata_image_route = router.find('path == "/api/epg/cache/metadata/image"')
window_route = router.find('path == "/api/epg/cache/window"')

if min(metadata_route, metadata_image_route, window_route) < 0:
    errors.append("metadata route ordering cannot be checked")
elif not metadata_route < metadata_image_route < window_route:
    errors.append(
        "metadata and metadata image routes must stay before the generic cache window route"
    )

required_controller = (
    "virtual ApiResponse getMetadata(",
    "virtual ApiResponse getMetadataImage(",
    "void registerScraperMetadataResolver(",
    "EpgArtworkRepository* artworkRepository_;",
    "fallbackDeliveryProviders_",
)

for fragment in required_controller:
    if fragment not in controller:
        errors.append(f"missing controller contract: {fragment}")

required_source = (
    "findMetadataJson(",
    "findMetadataImage(",
    r'\"available\":false,\"status\":\"pending\"',
    "EpgArtworkController::serveValidatedPath(",
    "loadSeriesArtworkFallback(",
    'kind == "preferred" && index == 0',
)
for fragment in required_source:
    if fragment not in controller_source:
        errors.append(f"missing SQLite metadata read contract: {fragment}")

primary_delivery = controller_source.find(
    "EpgArtworkController::serveValidatedPath(")
fallback_delivery = controller_source.find("loadSeriesArtworkFallback(")
if min(primary_delivery, fallback_delivery) < 0:
    errors.append("primary/fallback delivery priority cannot be checked")
elif primary_delivery >= fallback_delivery:
    errors.append("primary TVScraper artwork must be attempted before fallback delivery")

required_serializer = (
    "/api/epg/cache/metadata/image",
    "publicFallbackAvailable(",
    "artwork.managed",
    "EpgScraperArtworkOrigin::ExternalFallback",
    "primaryArtworkAvailable",
    "fallbackArtworkAvailable",
)
for fragment in required_serializer:
    if fragment not in serializer:
        errors.append(f"missing public fallback selection contract: {fragment}")

required_delivery = (
    "EpgArtworkReferenceOrigin::ExternalFallback",
    "O_NOFOLLOW",
    "::openat(",
    "::fstat(",
    "S_ISREG",
    '"image/png"',
    '"image/jpeg"',
    "maximumBytes",
    "maximumDimension",
    "maximumPixels",
    "extensionMatches(",
)
for fragment in required_delivery:
    if fragment not in delivery:
        errors.append(f"missing secure fallback delivery contract: {fragment}")

required_repository = (
    "origin TEXT NOT NULL DEFAULT 'external-fallback'",
    "EpgArtworkReferenceOrigin::ExternalFallback",
    "validProviderName(",
    "safeAbsolutePath(",
    "resolvedAt > 0",
)
for fragment in required_repository:
    if fragment not in fallback_repository:
        errors.append(f"missing fallback persistence contract: {fragment}")

for fragment in (
    'stream << "Cache-Control: no-cache\\r\\n";',
    'stream << "X-Content-Type-Options: nosniff\\r\\n";',
):
    if fragment not in http_listener:
        errors.append(f"missing HTTP response security header contract: {fragment}")

for forbidden in (
    "EpgScraperMetadataResolverRegistry scraperMetadataResolverRegistry_;",
    "EpgScraperMetadataPublicJsonSerializer scraperMetadataJsonSerializer_;",
    "resolver->resolve(",
    "tvscraper.db",
    "sqlite3_open",
    "GetScraperVideo",
):
    if forbidden in router or forbidden in controller or forbidden in controller_source:
        errors.append(f"forbidden public route coupling: {forbidden}")

for forbidden in (
    "api.themoviedb.org",
    "image.tmdb.org",
    "readAccessToken",
    "Authorization:",
    "/var/cache/",
):
    if forbidden in serializer or forbidden in controller_source:
        errors.append(f"forbidden public metadata leak: {forbidden}")

for forbidden in (
    "std::ifstream",
    "weakly_canonical",
):
    if forbidden in delivery:
        errors.append(f"forbidden fallback file access shortcut: {forbidden}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("epg scraper metadata routes contract ok")

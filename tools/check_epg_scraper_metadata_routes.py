#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
ROUTER = ROOT / "api/rest/src/ApiRouter.cpp"
CONTROLLER = ROOT / "api/rest/include/EpgCacheController.h"

errors = []
for path in (ROUTER, CONTROLLER):
    if not path.is_file():
        errors.append(f"missing file: {path.relative_to(ROOT)}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

router = ROUTER.read_text(encoding="utf-8")
controller = CONTROLLER.read_text(encoding="utf-8")

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
    "EpgScraperMetadataResolverRegistry scraperMetadataResolverRegistry_;",
    "EpgScraperMetadataPublicJsonSerializer scraperMetadataJsonSerializer_;",
)

for fragment in required_controller:
    if fragment not in controller:
        errors.append(f"missing controller contract: {fragment}")

for forbidden in (
    "tvscraper.db",
    "sqlite3_open",
    "GetScraperVideo",
):
    if forbidden in router or forbidden in controller:
        errors.append(f"forbidden public route coupling: {forbidden}")

if errors:
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    raise SystemExit(1)

print("epg scraper metadata routes contract ok")

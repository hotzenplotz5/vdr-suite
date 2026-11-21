#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
DOMAIN = ROOT / "core/vdr/include/TeletextDomain.h"
RESOLVER_HEADER = ROOT / "core/vdr/include/SuiteBridgeTeletextResolver.h"
RESOLVER_SOURCE = ROOT / "core/vdr/src/SuiteBridgeTeletextResolver.cpp"
TEST = ROOT / "core/vdr/tests/test_suite_bridge_teletext_resolver.cpp"

errors: list[str] = []

for path in (DOMAIN, RESOLVER_HEADER, RESOLVER_SOURCE, TEST):
    if not path.is_file():
        errors.append(f"missing Phase 67 Teletext domain file: {path.relative_to(ROOT)}")

domain = DOMAIN.read_text(encoding="utf-8") if DOMAIN.is_file() else ""
header = RESOLVER_HEADER.read_text(encoding="utf-8") if RESOLVER_HEADER.is_file() else ""
source = RESOLVER_SOURCE.read_text(encoding="utf-8") if RESOLVER_SOURCE.is_file() else ""
test = TEST.read_text(encoding="utf-8") if TEST.is_file() else ""

for fragment in (
    "struct TeletextProviderEvidence",
    "struct TeletextServiceRef",
    "std::uint64_t backendGeneration = 0",
    "struct TeletextPageRef",
    "struct TeletextCell",
    "struct TeletextPageSnapshot",
    "std::vector<TeletextCell> cells",
):
    if fragment not in domain:
        errors.append(f"missing Teletext domain contract: {fragment}")

for fragment in (
    '#include "ISuiteBridgeTeletextTransport.h"',
    "class SuiteBridgeTeletextResolver",
    "discoverService(",
    "readPage(",
):
    if fragment not in header:
        errors.append(f"missing Teletext resolver contract: {fragment}")

for fragment in (
    "transport_.discoverTeletext()",
    "transport_.requestTeletextPage(request)",
    'wire.capability != "broadcast.teletext.page"',
    "service.backendGeneration == 0",
    "wire.channel != service.channelId",
    "wire.cells.size() != kTeletextCellCount",
    "wire.text.size() != kTeletextRows",
    "provider.providerGeneration = wire.serviceEpoch",
    "provider.observedAt = wire.observedAt",
):
    if fragment not in source:
        errors.append(f"missing normalized Teletext resolver behavior: {fragment}")

for forbidden in (
    "PLUG suitebridge",
    "OsdTeletext::",
    "OSDTELETEXT_",
    "/var/cache/vdr/vtx",
    "libvdr-osdteletext",
    "vdr-plugin-suite-bridge/",
    "<vdr/",
):
    if forbidden in domain or forbidden in header or forbidden in source:
        errors.append(f"Teletext domain leaks provider/transport detail: {forbidden}")

for fragment in (
    'live.textRows[0] == "Börse fällt und lügt nicht"',
    "live.cells.size() == 1000",
    "TeletextSourceState::Cached",
    'missing.result == "page_not_found"',
    'malformed.error == "teletext_page_payload_invalid"',
    "invalid.backendGeneration = 0",
):
    if fragment not in test:
        errors.append(f"missing Teletext resolver regression coverage: {fragment}")

if errors:
    for error in errors:
        print(f"ERROR: {error}")
    sys.exit(1)

print("Phase 67 Teletext normalized domain boundary contract ok")

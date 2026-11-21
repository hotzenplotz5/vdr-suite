#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
adapter = (root / "suitebridge_teletext_adapter.cpp").read_text()
contract = (root / "suitebridge_teletext_provider_contract.h").read_text()

for token in (
    "cPluginManager::CallFirstService",
    "OSDTELETEXT_SERVICE_CAPABILITIES_V1",
    "OSDTELETEXT_SERVICE_GET_PAGE_V1",
    "provider_schema_incompatible",
    "page_schema_incompatible",
):
    if token not in adapter:
        raise SystemExit(f"missing teletext adapter wiring: {token}")

for token in (
    '"OsdTeletext::Capabilities-v1"',
    '"OsdTeletext::GetPage-v1"',
    "OSDTELETEXT_PAGE_ROWS 25U",
    "OSDTELETEXT_PAGE_COLUMNS 40U",
    "OSDTELETEXT_SUBPAGE_AUTO 0xFFFFU",
):
    if token not in contract:
        raise SystemExit(f"missing teletext provider ABI token: {token}")

for forbidden in ("/var/cache/vdr/vtx", "openForReading(", "getFilename("):
    if forbidden in adapter or forbidden in contract:
        raise SystemExit(f"forbidden disk-cache coupling: {forbidden}")

print("suitebridge teletext adapter wiring: PASS")

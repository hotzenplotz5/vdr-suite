#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
adapter = (root / "suitebridge_hbbtv_adapter.cpp").read_text()
contract = (root / "suitebridge_hbbtv_provider_contract.h").read_text()
command = (root / "suitebridge_hbbtv_command.cpp").read_text()

for token in (
    "cPluginManager::CallFirstService",
    "VDRWEB_SERVICE_HBBTV_DISCOVERY_V1",
    "VDRWEB_SERVICE_HBBTV_RUNTIME_V1",
    "provider_schema_incompatible",
    "provider_application_count_invalid",
    "provider_runtime_payload_invalid",
    "provider_runtime_identity_mismatch",
):
    if token not in adapter:
        raise SystemExit(f"missing HbbTV adapter wiring: {token}")

for token in (
    '"VdrWeb::HbbtvDiscovery-v1"',
    '"VdrWeb::HbbtvRuntime-v1"',
    '"VdrWeb::HbbtvPresentation-v1"',
    "VDRWEB_HBBTV_SERVICE_SCHEMA_V1 1U",
    "VDRWEB_HBBTV_RUNTIME_SCHEMA_V1 1U",
    "VDRWEB_HBBTV_MAX_APPLICATIONS 16U",
    "9ee1697a435e01058df6890323bf979a1ad2fd87",
    "1d30a97e2cf7343a6294de443c15cde7fcb831cb",
    "34ded5090fbad021338c491355566dbdb4d98f9d",
):
    if token not in contract:
        raise SystemExit(f"missing HbbTV provider ABI token: {token}")

for token in (
    "broadcast.hbbtv.discovery",
    "broadcast.hbbtv.runtime",
    "broadcast.hbbtv.presentation",
    "vdr-plugin-web",
    "HBBAPPS",
    "HBBRUN",
    "HBBPRES",
):
    if token not in command:
        raise SystemExit(f"missing HbbTV private command token: {token}")

for forbidden in (
    "cefbrowser",
    "HBBTV_URLS",
    "socket.ini",
    "LoadUrl",
    "RedButton",
    "ProcessKey",
):
    if forbidden in adapter or forbidden in contract:
        raise SystemExit(f"forbidden browser/provider coupling: {forbidden}")

print("suitebridge HbbTV adapter wiring: PASS")

#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "runtime": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "runtime_test": ROOT / "api/rest/tests/test_public_api_contract_runtime.cpp",
    "api_response": ROOT / "api/rest/include/DashboardController.h",
    "http": ROOT / "core/http/src/TestHttpServer.cpp",
    "adr": ROOT / "docs/adr/ADR-0048-public-api-versioning-error-compatibility-contract.md",
    "doc": ROOT / "docs/development/phase-69e-compatibility-policy-foundation.md",
    "current": ROOT / "docs/CURRENT.md",
    "status": ROOT / "docs/development/current-status.md",
    "roadmap": ROOT / "docs/planning/roadmap.md",
    "kickoff": ROOT / "docs/development/phase-69-public-api-kickoff.md",
    "inventory": ROOT / "tools/check_phase69_public_api_inventory.py",
}

def read(name):
    path = FILES[name]
    if not path.is_file():
        raise SystemExit(
            f"missing Phase 69.E compatibility-policy file: {path.relative_to(ROOT)}"
        )
    return path.read_text(encoding="utf-8")

def require(text, token, label):
    if token not in text:
        raise SystemExit(f"missing {label}: {token}")

runtime = read("runtime")
for token in (
    "compatibilityPolicy",
    "responseEvolution",
    "additive",
    "breakingChanges",
    "new-major",
    "unknownRequestFields",
    "legacyUnversioned",
    "public-api.compatibility-policy",
    "public-api.deprecation-metadata",
    "policyVersion",
    "supportedApiMajors",
    "sunset-announced",
    "deprecationHeaders",
    "deprecatedAliases",
):
    require(runtime, token, "public compatibility discovery contract")

runtime_test = read("runtime_test")
for token in (
    "compatibilityPolicy",
    "responseEvolution",
    "breakingChanges",
    "public-api.compatibility-policy",
    "public-api.deprecation-metadata",
    "deprecatedAliases",
    'headers.find("Deprecation")',
    'headers.find("Sunset")',
    'headers.find("Link")',
):
    require(runtime_test, token, "compatibility regression coverage")

require(
    read("api_response"),
    "std::map<std::string, std::string> headers;",
    "safe response-header carrier",
)
require(
    read("http"),
    "apiResponse.headers",
    "HTTP response-header propagation",
)

adr = read("adr")
for token in (
    "supported\n-> deprecated\n-> sunset announced\n-> removed",
    "Breaking cleanup normally occurs in v2.",
    "Request objects are closed by default: unknown fields are rejected",
    "The compatibility matrix records which API majors each server release supports.",
):
    require(adr, token, "ADR-0048 policy authority")

doc = read("doc")
for token in (
    "**IMPLEMENTED CANDIDATE — first bounded 69.E slice.**",
    "responseEvolution",
    "breakingChanges",
    "public-api.compatibility-policy",
    "public-api.deprecation-metadata",
    "deprecatedAliases",
    "Compatibility matrix",
    "remove, redirect or deprecate an",
    "existing route.",
    "Home",
    "LiveTV",
):
    require(doc, token, "documented 69.E foundation")

current = read("current")
require(
    current,
    "Current active runtime slice:\n69.E - Compatibility and deprecation policy",
    "CURRENT active 69.E",
)
require(
    current,
    "[Phase 69.E Compatibility Policy Foundation]",
    "CURRENT candidate link",
)

status = read("status")
require(
    status,
    "Current active runtime slice: **69.E - Compatibility and deprecation policy**",
    "current-status active 69.E",
)
require(
    status,
    "[Phase 69.E Compatibility Policy Foundation]",
    "current-status candidate link",
)

roadmap = read("roadmap")
require(
    roadmap,
    "Status: **Active.** First bounded candidate:",
    "roadmap 69.E candidate",
)
require(
    roadmap,
    "[Phase 69.E Compatibility Policy Foundation]",
    "roadmap candidate link",
)

kickoff = read("kickoff")
require(
    kickoff,
    "## 69.E compatibility policy foundation",
    "kickoff 69.E progress",
)

inventory = read("inventory")
for token in (
    '"/api/v1",',
    '"/api/v1/capabilities",',
):
    require(inventory, token, "existing public route inventory")

print("Phase 69.E compatibility-policy foundation guard passed.")
print(
    "Boundary: existing /api/v1 discovery only; versioned policy/capability "
    "metadata added; no legacy alias deprecated, redirected or removed."
)

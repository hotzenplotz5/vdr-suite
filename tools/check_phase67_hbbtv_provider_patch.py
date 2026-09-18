#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
patch_path = root / "patches" / "vdr-plugin-web-hbbtv-discovery-v1.patch"
patch = patch_path.read_text()

required = (
    "VdrWeb::HbbtvDiscovery-v1",
    "VdrSuiteHbbtvDiscoveryStore::BeginChannel",
    "VdrSuiteHbbtvDiscoveryStore::EndChannel",
    "VdrSuiteHbbtvDiscoveryStore::Upsert",
    "VdrSuiteHbbtvDiscoveryStore::Read",
    "applicationId",
    "controlCode",
    "priority",
    "urlBase",
    "urlLocation",
    "urlExtension",
    "strcmp(Id, VDRWEB_SERVICE_HBBTV_DISCOVERY_V1)",
)

for token in required:
    if token not in patch:
        raise SystemExit(f"missing provider patch token: {token}")

for forbidden in (
    "LoadUrl",
    "ProcessKey",
    "RedButton",
    "ExecuteJavascript",
    "WebApp-Url-v1.0",
):
    if forbidden in patch:
        raise SystemExit(f"provider patch must remain discovery-only: {forbidden}")

if "34ded5090fbad021338c491355566dbdb4d98f9d" in patch:
    raise SystemExit("upstream pin belongs in the SuiteBridge mirror, not duplicated in patch text")

# Verify unified-diff hunk counters so git apply does not fail on malformed metadata.
lines = patch.splitlines()
for index, line in enumerate(lines):
    match = re.match(
        r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@",
        line,
    )
    if not match:
        continue

    declared_old = int(match.group(2) or "1")
    declared_new = int(match.group(4) or "1")
    actual_old = 0
    actual_new = 0

    cursor = index + 1
    while cursor < len(lines):
        current = lines[cursor]
        if current.startswith("@@ ") or current.startswith("diff --git "):
            break
        if current.startswith("\\ No newline"):
            cursor += 1
            continue
        if current.startswith("+") and not current.startswith("+++"):
            actual_new += 1
        elif current.startswith("-") and not current.startswith("---"):
            actual_old += 1
        elif current.startswith(" "):
            actual_old += 1
            actual_new += 1
        cursor += 1

    if declared_old != actual_old or declared_new != actual_new:
        raise SystemExit(
            "malformed provider patch hunk: "
            f"{line} declared old/new {declared_old}/{declared_new}, "
            f"actual {actual_old}/{actual_new}"
        )

print("vdr-plugin-web HbbTV discovery provider patch: PASS")

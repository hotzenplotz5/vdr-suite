#!/usr/bin/env python3
from pathlib import Path
import re
import sys

SOURCE = Path("core/daemon/src/DaemonRuntime.cpp")

REQUIRED_RESETS = [
    "epgController_",
    "epgSearchResultJsonSerializer_",
    "epgSearchService_",
    "epgQueryService_",
    "epgSearchNativeFuzzyStartupRestoreService_",
    "epgSearchNativeFuzzyCapabilityFreshnessPolicy_",
    "epgSearchNativeFuzzyCapabilityDetector_",
    "epgSearchNativeFuzzyCapabilityRepository_",
    "searchTimerAutomationPreviewController_",
    "searchTimerAutomationDryRunResultJsonSerializer_",
    "searchTimerAutomationReadOnlyService_",
    "runtimeDiagnosticsJsonSerializer_",
]


def extract_shutdown_body(source: str) -> str:
    match = re.search(r"void\s+DaemonRuntime::shutdown\s*\(\)\s*\{", source)
    if not match:
        raise RuntimeError("DaemonRuntime::shutdown() not found")

    body_start = match.end()
    depth = 1
    index = body_start

    while index < len(source):
        character = source[index]
        if character == "{":
            depth += 1
        elif character == "}":
            depth -= 1
            if depth == 0:
                return source[body_start:index]
        index += 1

    raise RuntimeError("DaemonRuntime::shutdown() body not closed")


def main() -> int:
    source = SOURCE.read_text(encoding="utf-8")
    shutdown_body = extract_shutdown_body(source)

    missing = []
    for member in REQUIRED_RESETS:
        expected = f"{member}.reset();"
        if expected not in shutdown_body:
            missing.append(expected)

    if missing:
        print("DaemonRuntime shutdown reset check failed:")
        for item in missing:
            print(f"- missing {item}")
        return 1

    print("DaemonRuntime shutdown reset check passed.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print(f"DaemonRuntime shutdown reset check failed: {error}")
        sys.exit(1)

#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

CHECKS = [
    (
        "core/recordings/include/RecordingActionRequest.h",
        [
            "bool dryRun = true;",
            "std::string backendId;",
            "std::string recordingId;",
            "std::map<std::string, std::string> parameters;",
        ],
    ),
    (
        "core/recordings/include/RecordingActionBackendPolicy.h",
        [
            "bool readOnly = false;",
            "bool executionAllowed = false;",
            "policy.readOnly = true;",
            "policy.executionAllowed = false;",
            "restfulApiMutationPolicy",
            "policy.executionAllowed = true;",
        ],
    ),
    (
        "core/recordings/include/RecordingActionSafetyService.h",
        [
            "if (context.backendReadOnly)",
            "recording action execution is blocked by read-only backend config",
            "if (!context.dryRun && !context.executionAllowed)",
            "real recording action execution is disabled",
            "recording is currently in use",
            "recording action capability is missing",
            "recording action permission is denied",
        ],
    ),
    (
        "core/recordings/include/RecordingActionExecutionService.h",
        [
            "evaluateSafety(request, policy)",
            "if (!safety.canExecute)",
            "recording action execution blocked by safety policy",
        ],
    ),
    (
        "core/recordings/src/RestfulApiRecordingActionExecutor.cpp",
        [
            "case RecordingActionType::Move:",
            "case RecordingActionType::Rename:",
            "case RecordingActionType::Delete:",
            "RESTfulAPI recording action type not supported",
        ],
    ),
    (
        "core/recordings/include/RestfulApiRecordingActionRequestBuilder.h",
        [
            "findParameter(payload.parameters, \"backendNativeId\")",
            "findParameter(payload.parameters, \"recordingPath\")",
            "return payload.recordingId;",
            "/recordings/move.json",
            "/recordings/delete.json",
            "\"copy_only\":false",
        ],
    ),
    (
        "docs/development/phase-55.6-recording-operations-audit-and-safety-policy.md",
        [
            "Allowed by default",
            "Not allowed by default",
            "real recording move",
            "real recording rename",
            "real recording delete",
            "backendNativeId",
            "executionAllowed true",
            "post-action recording list refresh/readback planned",
            "No production default should silently open real recording mutation.",
        ],
    ),
]


def main() -> int:
    problems = []

    for relative_path, required_fragments in CHECKS:
        path = ROOT / relative_path
        if not path.exists():
            problems.append(f"missing file: {relative_path}")
            continue

        text = path.read_text(encoding="utf-8")
        for fragment in required_fragments:
            if fragment not in text:
                problems.append(f"{relative_path}: missing fragment: {fragment}")

    if problems:
        print("Recording mutation safety policy check failed:")
        for problem in problems:
            print("- " + problem)
        return 1

    print("Recording mutation safety policy check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
from pathlib import Path

ROOT = Path("/home/yavdr/vdr-suite")


def replace_once(relative_path: str, old: str, new: str) -> None:
    path = ROOT / relative_path
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f"{relative_path}: expected exactly one replacement match, found {count}"
        )
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"updated {relative_path}")


replace_once(
    "core/recordings/include/RestfulApiRecordingActionRequestBuilder.h",
    """    static std::string renameTarget(
        const RecordingActionJobPayload& payload,
        const std::string& newName)
    {
        const std::string parent =
            recordingParentFolder(payload);

        if (parent.empty()) {
            return newName;
        }

        return parent + "/" + newName;
    }
""",
    """    static std::string renameTarget(
        const RecordingActionJobPayload& payload,
        const std::string& newName)
    {
        if (newName.find('/') != std::string::npos ||
            newName.find('~') != std::string::npos) {
            return newName;
        }

        const std::string parent =
            recordingParentFolder(payload);

        if (parent.empty()) {
            return newName;
        }

        return parent + "/" + newName;
    }
"""
)

replace_once(
    "api/rest/tests/test_recording_action_execution_controller.cpp",
    """#include <cassert>
#include <memory>
#include <string>
#include <vector>
""",
    """#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>
"""
)

replace_once(
    "api/rest/tests/test_recording_action_execution_controller.cpp",
    """    int refreshCount = 0;
    resolvedBodyController.setAfterSuccessfulExecutionCallback(
        [&refreshCount]() {
            ++refreshCount;
        });
""",
    """    std::atomic<int> refreshCount{0};
    resolvedBodyController.setAfterSuccessfulExecutionCallback(
        [&refreshCount]() {
            refreshCount.fetch_add(1);
        });
"""
)

replace_once(
    "api/rest/tests/test_recording_action_execution_controller.cpp",
    """    assert(capturingAdapter->lastPayload.parameters.at("backendNativeId") == recording.backendNativeId);
    assert(capturingAdapter->lastPayload.parameters.at("recordingTitle") == recording.title);
    assert(refreshCount == 1);
""",
    """    assert(capturingAdapter->lastPayload.parameters.at("backendNativeId") == recording.backendNativeId);
    assert(capturingAdapter->lastPayload.parameters.at("recordingTitle") == recording.title);

    for (int attempt = 0;
         attempt < 100 && refreshCount.load() == 0;
         ++attempt)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }

    assert(refreshCount.load() == 1);
"""
)

print("recording MOVE follow-up fix applied")

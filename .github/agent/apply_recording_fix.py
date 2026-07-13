from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    file_path = Path(path)
    text = file_path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(
            f"expected exactly one match in {path}, found {count}: {old[:120]!r}"
        )
    file_path.write_text(text.replace(old, new, 1), encoding="utf-8")


replace_once(
    "api/rest/include/RecordingActionExecutionController.h",
    """    void setAfterSuccessfulExecutionCallback(\n        std::function<void()> callback);\n""",
    """    void setAfterSuccessfulExecutionCallback(\n        std::function<bool(const RecordingActionRequest&)> callback);\n""",
)
replace_once(
    "api/rest/include/RecordingActionExecutionController.h",
    """    std::function<void()> afterSuccessfulExecution_;\n""",
    """    std::function<bool(const RecordingActionRequest&)> afterSuccessfulExecution_;\n""",
)
replace_once(
    "api/rest/include/RecordingActionExecutionController.h",
    """    bool refreshAfterSuccessfulExecution(\n        const RecordingActionExecutionResult& result) const;\n""",
    """    bool refreshAfterSuccessfulExecution(\n        const RecordingActionExecutionResult& result,\n        const RecordingActionRequest& resolvedRequest) const;\n""",
)

replace_once(
    "api/rest/src/RecordingActionExecutionController.cpp",
    """#include <thread>\n\n""",
    """,
)
replace_once(
    "api/rest/src/RecordingActionExecutionController.cpp",
    """void RecordingActionExecutionController::setAfterSuccessfulExecutionCallback(\n    std::function<void()> callback)\n""",
    """void RecordingActionExecutionController::setAfterSuccessfulExecutionCallback(\n    std::function<bool(const RecordingActionRequest&)> callback)\n""",
)
replace_once(
    "api/rest/src/RecordingActionExecutionController.cpp",
    """bool RecordingActionExecutionController::refreshAfterSuccessfulExecution(\n    const RecordingActionExecutionResult& result) const\n{\n    if (!result.success)\n    {\n        return false;\n    }\n\n    if (!afterSuccessfulExecution_)\n    {\n        return false;\n    }\n\n    const std::function<void()> callback = afterSuccessfulExecution_;\n\n    std::thread([callback]() {\n        callback();\n    }).detach();\n\n    return true;\n}\n""",
    """bool RecordingActionExecutionController::refreshAfterSuccessfulExecution(\n    const RecordingActionExecutionResult& result,\n    const RecordingActionRequest& resolvedRequest) const\n{\n    if (!result.success)\n    {\n        return false;\n    }\n\n    if (!afterSuccessfulExecution_)\n    {\n        return false;\n    }\n\n    return afterSuccessfulExecution_(resolvedRequest);\n}\n""",
)
replace_once(
    "api/rest/src/RecordingActionExecutionController.cpp",
    """        const bool snapshotRefreshed =\n            refreshAfterSuccessfulExecution(result);\n""",
    """        const bool snapshotRefreshed =\n            refreshAfterSuccessfulExecution(\n                result,\n                resolvedRequest);\n""",
)
replace_once(
    "api/rest/src/RecordingActionExecutionController.cpp",
    """    const bool snapshotRefreshed =\n        refreshAfterSuccessfulExecution(result);\n""",
    """    const bool snapshotRefreshed =\n        refreshAfterSuccessfulExecution(\n            result,\n            resolvedRequest);\n""",
)

replace_once(
    "core/daemon/src/DaemonRuntime.cpp",
    """    recordingActionExecutionController_->setAfterSuccessfulExecutionCallback(\n        [this]() {\n            for (const auto& backendRuntimeContext : backendRuntimeContexts_) {\n                const std::vector<VdrRecording> recordings =\n                    backendRuntimeContext->snapshotBuilder->buildRecordings();\n\n                snapshotCacheService_->updateRecordingsForBackend(\n                    backendRuntimeContext->backendId,\n                    recordings);\n\n                if (vdrRecordingCacheRepository_) {\n                    vdrRecordingCacheRepository_->replaceRecordingsForBackend(\n                        backendRuntimeContext->backendId,\n                        recordings);\n                    vdrRecordingCacheRepository_->markRefreshFinished(\n                        backendRuntimeContext->backendId,\n                        static_cast<int>(recordings.size()));\n                }\n            }\n        });\n""",
    """    recordingActionExecutionController_->setAfterSuccessfulExecutionCallback(\n        [this](const RecordingActionRequest& request) {\n            const std::string backendId =\n                request.backendId.empty()\n                    ? \"default\"\n                    : request.backendId;\n\n            for (const auto& backendRuntimeContext : backendRuntimeContexts_) {\n                if (!backendRuntimeContext ||\n                    backendRuntimeContext->backendId != backendId ||\n                    !backendRuntimeContext->snapshotBuilder) {\n                    continue;\n                }\n\n                if (vdrRecordingCacheRepository_ &&\n                    !vdrRecordingCacheRepository_->markRefreshStarted(backendId)) {\n                    return false;\n                }\n\n                try {\n                    const std::vector<VdrRecording> recordings =\n                        backendRuntimeContext->snapshotBuilder->buildRecordings();\n\n                    snapshotCacheService_->updateRecordingsForBackend(\n                        backendId,\n                        recordings);\n\n                    if (vdrRecordingCacheRepository_) {\n                        if (!vdrRecordingCacheRepository_->replaceRecordingsForBackend(\n                                backendId,\n                                recordings)) {\n                            vdrRecordingCacheRepository_->markRefreshFailed(\n                                backendId,\n                                \"recording action cache replacement failed\");\n                            return false;\n                        }\n\n                        if (!vdrRecordingCacheRepository_->markRefreshFinished(\n                                backendId,\n                                static_cast<int>(recordings.size()))) {\n                            vdrRecordingCacheRepository_->markRefreshFailed(\n                                backendId,\n                                \"recording action cache status update failed\");\n                            return false;\n                        }\n                    }\n\n                    recordingCacheDirtyHint_.store(false);\n                    externalVdrChangeHint_.store(true);\n                    return true;\n                }\n                catch (const std::exception& error) {\n                    if (vdrRecordingCacheRepository_) {\n                        vdrRecordingCacheRepository_->markRefreshFailed(\n                            backendId,\n                            error.what());\n                    }\n                    return false;\n                }\n                catch (...) {\n                    if (vdrRecordingCacheRepository_) {\n                        vdrRecordingCacheRepository_->markRefreshFailed(\n                            backendId,\n                            \"recording action cache refresh failed\");\n                    }\n                    return false;\n                }\n            }\n\n            return false;\n        });\n""",
)

replace_once(
    "api/rest/tests/test_recording_action_execution_controller.cpp",
    """#include <atomic>\n#include <cassert>\n#include <chrono>\n#include <memory>\n#include <string>\n#include <thread>\n#include <vector>\n""",
    """#include <cassert>\n#include <memory>\n#include <string>\n#include <vector>\n""",
)
replace_once(
    "api/rest/tests/test_recording_action_execution_controller.cpp",
    """    std::atomic<int> refreshCount{0};\n    resolvedBodyController.setAfterSuccessfulExecutionCallback(\n        [&refreshCount]() {\n            refreshCount.fetch_add(1);\n        });\n""",
    """    int refreshCount = 0;\n    RecordingActionRequest refreshedRequest;\n    resolvedBodyController.setAfterSuccessfulExecutionCallback(\n        [&refreshCount, &refreshedRequest](\n            const RecordingActionRequest& callbackRequest) {\n            ++refreshCount;\n            refreshedRequest = callbackRequest;\n            return true;\n        });\n""",
)
replace_once(
    "api/rest/tests/test_recording_action_execution_controller.cpp",
    """    for (int attempt = 0;\n         attempt < 100 && refreshCount.load() == 0;\n         ++attempt)\n    {\n        std::this_thread::sleep_for(\n            std::chrono::milliseconds(10));\n    }\n\n    assert(refreshCount.load() == 1);\n""",
    """    assert(refreshCount == 1);\n    assert(refreshedRequest.backendId == \"living-room\");\n    assert(refreshedRequest.recordingId == \"recording-3\");\n    assert(\n        refreshedRequest.parameters.at(\"backendNativeId\") ==\n        recording.backendNativeId);\n""",
)

replace_once(
    "web/frontend/modules/recordings.js",
    """function recordingBrowserLoadServerFolder(path, offset) {\n  if (!recordingBrowserFolderLoader) {\n    return;\n  }\n\n  recordingBrowserCancelFolderRefreshTimer();\n  recordingBrowserRenderFolderLoading(path);\n\n  recordingBrowserFolderLoader(path || '', Number(offset) || 0)\n    .then(renderServerRecordingFolder)\n    .catch(recordingBrowserRenderFolderLoadError);\n}\n""",
    """function recordingBrowserLoadServerFolder(path, offset) {\n  if (!recordingBrowserFolderLoader) {\n    return;\n  }\n\n  recordingBrowserCancelFolderRefreshTimer();\n  recordingBrowserRenderFolderLoading(path);\n\n  recordingBrowserFolderLoader(path || '', Number(offset) || 0)\n    .then(renderServerRecordingFolder)\n    .catch(recordingBrowserRenderFolderLoadError);\n}\n\nfunction recordingBrowserRefreshFolderAfterSuccessfulAction(result, folderData) {\n  if (!result ||\n      result.success !== true ||\n      result.snapshotRefreshed !== true ||\n      !recordingBrowserFolderLoader) {\n    return false;\n  }\n\n  const folderPath = folderData && typeof folderData === 'object'\n    ? String(folderData.path || '')\n    : '';\n\n  recordingBrowserLoadServerFolder(folderPath, 0);\n  return true;\n}\n""",
)
replace_once(
    "web/frontend/modules/recordings.js",
    """function recordingBrowserCreateRenameButton(recording, resultBox) {\n""",
    """function recordingBrowserCreateRenameButton(recording, folderData, resultBox) {\n""",
)
replace_once(
    "web/frontend/modules/recordings.js",
    """      .then(result => {\n        recordingBrowserRenderActionResult(resultBox, 'Umbenennen', result, null);\n      })\n""",
    """      .then(result => {\n        recordingBrowserRenderActionResult(resultBox, 'Umbenennen', result, null);\n        recordingBrowserRefreshFolderAfterSuccessfulAction(result, folderData);\n      })\n""",
)
replace_once(
    "web/frontend/modules/recordings.js",
    """function createServerRecordingActionPanel(recording) {\n""",
    """function createServerRecordingActionPanel(recording, folderData) {\n""",
)
replace_once(
    "web/frontend/modules/recordings.js",
    """  actions.appendChild(recordingBrowserCreateRenameButton(\n    recording,\n    resultBox\n  ));\n""",
    """  actions.appendChild(recordingBrowserCreateRenameButton(\n    recording,\n    folderData,\n    resultBox\n  ));\n""",
)
replace_once(
    "web/frontend/modules/recordings.js",
    """  item.appendChild(createServerRecordingActionPanel(recording));\n""",
    """  item.appendChild(createServerRecordingActionPanel(recording, folderData));\n""",
)
replace_once(
    "web/frontend/modules/recordings.js",
    """        if (result && result.success) {\n          recordingBrowserScheduleMoveParentFolderReload(targetPath);\n        }\n""",
    """        if (result && result.success) {\n          if (result.snapshotRefreshed === true) {\n            recordingBrowserLoadServerFolder(\n              recordingBrowserMoveParentPath(targetPath),\n              0\n            );\n          } else {\n            recordingBrowserScheduleMoveParentFolderReload(targetPath);\n          }\n        }\n""",
)

Path(".github/agent/apply_recording_fix.py").unlink()
Path(".github/workflows/agent-recording-fix.yml").unlink()
try:
    Path(".github/agent").rmdir()
except OSError:
    pass

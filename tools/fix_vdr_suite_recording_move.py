#!/usr/bin/env python3
from pathlib import Path

ROOT = Path('/home/yavdr/vdr-suite')


def replace_once(relative_path: str, old: str, new: str) -> None:
    path = ROOT / relative_path
    text = path.read_text(encoding='utf-8')
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f'{relative_path}: expected exactly one replacement match, found {count}'
        )
    path.write_text(text.replace(old, new, 1), encoding='utf-8')
    print(f'updated {relative_path}')


replace_once(
    'core/recordings/include/RestfulApiRecordingActionRequestBuilder.h',
    '''#include <map>\n#include <string>\n''',
    '''#include <algorithm>\n#include <map>\n#include <string>\n#include <vector>\n'''
)

replace_once(
    'core/recordings/include/RestfulApiRecordingActionRequestBuilder.h',
    '''    static std::string normalizedRecordingFilesystemPath(\n        const RecordingActionJobPayload& payload)\n    {\n        const std::string source =\n            recordingPath(payload);\n\n        const std::vector<std::string> segments =\n            splitRecordingPathSegments(source);\n\n        if (segments.empty()) {\n            return source;\n        }\n\n        if (source.rfind("/srv/vdr/video/", 0) == 0) {\n            return "/srv/vdr/video/" + joinRecordingPathSegments(segments);\n        }\n\n        return "/" + joinRecordingPathSegments(segments);\n    }\n\n''',
    '''    static std::string recordingLogicalLeafName(\n        const RecordingActionJobPayload& payload)\n    {\n        std::string logicalName =\n            findParameter(payload.parameters, "recordingTitle");\n\n        if (logicalName.empty()) {\n            return recordingLeafName(\n                normalizedRecordingPath(payload));\n        }\n\n        std::replace(\n            logicalName.begin(),\n            logicalName.end(),\n            '~',\n            '/');\n\n        while (!logicalName.empty() &&\n               logicalName.back() == '/') {\n            logicalName.pop_back();\n        }\n\n        const std::size_t separator =\n            logicalName.find_last_of('/');\n\n        if (separator == std::string::npos) {\n            return logicalName;\n        }\n\n        return logicalName.substr(separator + 1);\n    }\n\n'''
)

replace_once(
    'core/recordings/include/RestfulApiRecordingActionRequestBuilder.h',
    '''    static std::string moveTarget(\n        const std::string& targetPath,\n        const RecordingActionJobPayload& payload)\n    {\n        const std::string source =\n            normalizedRecordingPath(payload);\n\n        const std::string leaf =\n            recordingLeafName(source);\n\n        if (targetPath.empty() || leaf.empty()) {\n            return targetPath;\n        }\n''',
    '''    static std::string moveTarget(\n        const std::string& targetPath,\n        const RecordingActionJobPayload& payload)\n    {\n        const std::string leaf =\n            recordingLogicalLeafName(payload);\n\n        if (targetPath.empty() || leaf.empty()) {\n            return targetPath;\n        }\n'''
)

replace_once(
    'core/recordings/include/RestfulApiRecordingActionRequestBuilder.h',
    '''        body += "\\\"source\\\":" + jsonQuote(normalizedRecordingFilesystemPath(payload));\n''',
    '''        body += "\\\"source\\\":" + jsonQuote(recordingPath(payload));\n'''
)

replace_once(
    'api/rest/src/RecordingActionValidationRequestParser.cpp',
    '''    if (!backendNativeId.empty())\n    {\n        request.parameters["backendNativeId"] = backendNativeId;\n    }\n\n    return request;\n''',
    '''    if (!backendNativeId.empty())\n    {\n        request.parameters["backendNativeId"] = backendNativeId;\n    }\n\n    const std::string recordingTitle =\n        getValue(values, "recordingTitle");\n\n    if (!recordingTitle.empty())\n    {\n        request.parameters["recordingTitle"] = recordingTitle;\n    }\n\n    return request;\n'''
)

replace_once(
    'web/frontend/modules/recordings.js',
    '''  const recordingId = recordingBrowserFirstValue(recording, ['recordingId', 'id', 'nativeId'], '');\n  const recordingPath = recordingBrowserFirstValue(recording, ['path', 'fileName', 'directory'], '');\n  const backendNativeId = recordingBrowserFirstValue(recording, ['backendNativeId', 'nativePath'], '');\n  const extra = overrides && typeof overrides === 'object' ? overrides : {};\n''',
    '''  const recordingId = recordingBrowserFirstValue(recording, ['recordingId', 'id', 'nativeId'], '');\n  const recordingPath = recordingBrowserFirstValue(recording, ['path', 'fileName', 'directory'], '');\n  const backendNativeId = recordingBrowserFirstValue(recording, ['backendNativeId', 'nativePath'], '');\n  const recordingTitle = recordingBrowserFirstValue(recording, ['title', 'name', 'displayName'], '');\n  const extra = overrides && typeof overrides === 'object' ? overrides : {};\n'''
)

replace_once(
    'web/frontend/modules/recordings.js',
    '''  if (String(backendNativeId || '').trim() !== '') {\n    payload.backendNativeId = String(backendNativeId);\n  }\n\n  Object.keys(extra).forEach(key => {\n''',
    '''  if (String(backendNativeId || '').trim() !== '') {\n    payload.backendNativeId = String(backendNativeId);\n  }\n\n  if (String(recordingTitle || '').trim() !== '') {\n    payload.recordingTitle = String(recordingTitle);\n  }\n\n  Object.keys(extra).forEach(key => {\n'''
)

replace_once(
    'api/rest/src/RecordingActionExecutionController.cpp',
    '''RecordingActionRequest RecordingActionExecutionController::resolveBackendNativeId(\n    const RecordingActionRequest& request) const\n{\n    if (snapshotReadService_ == nullptr)\n    {\n        return request;\n    }\n\n    if (request.parameters.find("backendNativeId") != request.parameters.end())\n    {\n        return request;\n    }\n\n    if (request.backendId.empty() || request.recordingId.empty())\n    {\n        return request;\n    }\n\n    RecordingActionRequest resolved = request;\n\n    const std::vector<VdrRecording> recordings =\n        snapshotReadService_->getRecordingsForBackend(request.backendId);\n\n    for (const VdrRecording& recording : recordings)\n    {\n        if (recording.id == request.recordingId && !recording.backendNativeId.empty())\n        {\n            resolved.parameters["backendNativeId"] = recording.backendNativeId;\n            return resolved;\n        }\n    }\n\n    return resolved;\n}\n''',
    '''RecordingActionRequest RecordingActionExecutionController::resolveBackendNativeId(\n    const RecordingActionRequest& request) const\n{\n    if (snapshotReadService_ == nullptr)\n    {\n        return request;\n    }\n\n    const auto backendNativeId =\n        request.parameters.find("backendNativeId");\n    const bool needsBackendNativeId =\n        backendNativeId == request.parameters.end() ||\n        backendNativeId->second.empty();\n\n    const auto recordingTitle =\n        request.parameters.find("recordingTitle");\n    const bool needsRecordingTitle =\n        recordingTitle == request.parameters.end() ||\n        recordingTitle->second.empty();\n\n    if (!needsBackendNativeId && !needsRecordingTitle)\n    {\n        return request;\n    }\n\n    if (request.backendId.empty() || request.recordingId.empty())\n    {\n        return request;\n    }\n\n    RecordingActionRequest resolved = request;\n\n    const std::vector<VdrRecording> recordings =\n        snapshotReadService_->getRecordingsForBackend(request.backendId);\n\n    for (const VdrRecording& recording : recordings)\n    {\n        if (recording.id != request.recordingId)\n        {\n            continue;\n        }\n\n        if (needsBackendNativeId &&\n            !recording.backendNativeId.empty())\n        {\n            resolved.parameters["backendNativeId"] =\n                recording.backendNativeId;\n        }\n\n        if (needsRecordingTitle &&\n            !recording.title.empty())\n        {\n            resolved.parameters["recordingTitle"] =\n                recording.title;\n        }\n\n        return resolved;\n    }\n\n    return resolved;\n}\n'''
)

replace_once(
    'apps/tools/restfulapi_recording_action_real_move_smoke.cpp',
    '''    std::string source;\n    std::string target;\n    bool execute = false;\n''',
    '''    std::string source;\n    std::string target;\n    std::string title;\n    bool execute = false;\n'''
)

replace_once(
    'apps/tools/restfulapi_recording_action_real_move_smoke.cpp',
    '''        << "--source '<vdr-recording-source>' "\n        << "--target 'Archive/Target' [--execute]\\n\\n"\n''',
    '''        << "--source '<absolute-vdr-file-name>' "\n        << "--target 'Archive/Target' "\n        << "--title '<logical-vdr-recording-name>' [--execute]\\n\\n"\n'''
)

replace_once(
    'apps/tools/restfulapi_recording_action_real_move_smoke.cpp',
    '''        else if (argument == "--target" && index + 1 < argc)\n        {\n            options.target = argv[++index];\n        }\n        else\n''',
    '''        else if (argument == "--target" && index + 1 < argc)\n        {\n            options.target = argv[++index];\n        }\n        else if (argument == "--title" && index + 1 < argc)\n        {\n            options.title = argv[++index];\n        }\n        else\n'''
)

replace_once(
    'apps/tools/restfulapi_recording_action_real_move_smoke.cpp',
    '''    request.parameters["recordingPath"] = options.source;\n    request.parameters["targetPath"] = options.target;\n''',
    '''    request.parameters["recordingPath"] = options.source;\n    request.parameters["targetPath"] = options.target;\n    request.parameters["recordingTitle"] = options.title;\n'''
)

replace_once(
    'apps/tools/restfulapi_recording_action_real_move_smoke.cpp',
    '''    payload.parameters["recordingPath"] = options.source;\n    payload.parameters["targetPath"] = options.target;\n''',
    '''    payload.parameters["recordingPath"] = options.source;\n    payload.parameters["targetPath"] = options.target;\n    payload.parameters["recordingTitle"] = options.title;\n'''
)

replace_once(
    'apps/tools/restfulapi_recording_action_real_move_smoke.cpp',
    '''    if (options.source.empty() || options.target.empty())\n''',
    '''    if (options.source.empty() ||\n        options.target.empty() ||\n        options.title.empty())\n'''
)

replace_once(
    'core/recordings/tests/test_restfulapi_move_tilde_mapping_regression.cpp',
    '''    payload.recordingId =\n        "Tagesschau/2026-06-17.20.00.10-0.rec";\n    payload.type = RecordingActionType::Move;\n    payload.dryRun = true;\n    payload.parameters["recordingPath"] =\n        "Tagesschau/2026-06-17.20.00.10-0.rec";\n    payload.parameters["targetPath"] =\n        "Archiv/Tagesschau";\n''',
    '''    payload.recordingId = "7983";\n    payload.type = RecordingActionType::Move;\n    payload.dryRun = true;\n    payload.parameters["recordingPath"] =\n        "/Recordings_on_yavdr(nfs)/Ghibli/Die_letzten_Glühwürmchen__1988_/2026-04-18.21.32.1-0.rec";\n    payload.parameters["backendNativeId"] =\n        "/srv/vdr/video/Recordings_on_yavdr(nfs)/Ghibli/Die_letzten_Glühwürmchen__1988_/2026-04-18.21.32.1-0.rec";\n    payload.parameters["recordingTitle"] =\n        "Ghibli/Die letzten Glühwürmchen (1988)";\n    payload.parameters["targetPath"] =\n        "__vdr_suite_move_probe__";\n'''
)

replace_once(
    'core/recordings/tests/test_restfulapi_move_tilde_mapping_regression.cpp',
    '''    assert(request.body.find("\\\"source\\\":\\\"Tagesschau/2026-06-17.20.00.10-0.rec\\\"") != std::string::npos);\n    assert(request.body.find("\\\"target\\\":\\\"Archiv~Tagesschau~Tagesschau\\\"") != std::string::npos);\n    assert(request.body.find("\\\"copy_only\\\":false") != std::string::npos);\n\n    assert(request.body.find("Archiv/Tagesschau") == std::string::npos);\n    assert(request.body.find("Archiv~~Tagesschau") == std::string::npos);\n    assert(request.body.find("/api/") == std::string::npos);\n''',
    '''    assert(request.body.find(\n        "\\\"source\\\":\\\"/srv/vdr/video/Recordings_on_yavdr(nfs)/Ghibli/Die_letzten_Glühwürmchen__1988_/2026-04-18.21.32.1-0.rec\\\"")\n        != std::string::npos);\n    assert(request.body.find(\n        "\\\"target\\\":\\\"__vdr_suite_move_probe__~Die letzten Glühwürmchen (1988)\\\"")\n        != std::string::npos);\n    assert(request.body.find("\\\"copy_only\\\":false") != std::string::npos);\n\n    assert(request.body.find(\n        "\\\"source\\\":\\\"/srv/vdr/video/Ghibli/") == std::string::npos);\n    assert(request.body.find(\n        "__vdr_suite_move_probe__~Die_letzten_Glühwürmchen__1988_") == std::string::npos);\n    assert(request.body.find("/api/") == std::string::npos);\n'''
)

replace_once(
    'api/rest/tests/test_recording_action_validation_request_parser.cpp',
    '''            "\\\"targetPath\\\":\\\"/srv/vdr/video/archive\\\","\n            "\\\"recordingPath\\\":\\\"Movies/Tatort/2026-06-16.20.15.1-0.rec\\\""\n''',
    '''            "\\\"targetPath\\\":\\\"/srv/vdr/video/archive\\\","\n            "\\\"recordingPath\\\":\\\"Movies/Tatort/2026-06-16.20.15.1-0.rec\\\","\n            "\\\"recordingTitle\\\":\\\"Movies/Tatort am Abend\\\""\n'''
)

replace_once(
    'api/rest/tests/test_recording_action_validation_request_parser.cpp',
    '''    assert(\n        moveRequest.parameters.at("recordingPath") ==\n        "Movies/Tatort/2026-06-16.20.15.1-0.rec");\n''',
    '''    assert(\n        moveRequest.parameters.at("recordingPath") ==\n        "Movies/Tatort/2026-06-16.20.15.1-0.rec");\n    assert(\n        moveRequest.parameters.at("recordingTitle") ==\n        "Movies/Tatort am Abend");\n'''
)

replace_once(
    'api/rest/tests/test_recording_action_execution_controller.cpp',
    '''    recording.backendNativeId = "/srv/vdr/video/Movies/Test/2026-06-19.20.15.1-0.rec";\n''',
    '''    recording.backendNativeId = "/srv/vdr/video/Movies/Test/2026-06-19.20.15.1-0.rec";\n    recording.title = "Movies/Test Recording";\n'''
)

replace_once(
    'api/rest/tests/test_recording_action_execution_controller.cpp',
    '''    assert(capturingAdapter->lastPayload.parameters.at("backendNativeId") == recording.backendNativeId);\n    assert(refreshCount == 1);\n''',
    '''    assert(capturingAdapter->lastPayload.parameters.at("backendNativeId") == recording.backendNativeId);\n    assert(capturingAdapter->lastPayload.parameters.at("recordingTitle") == recording.title);\n    assert(refreshCount == 1);\n'''
)

print('recording MOVE contract fix applied')

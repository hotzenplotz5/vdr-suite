#pragma once

#include "DashboardController.h"
#include "RecordingActionRequest.h"
#include "RecordingActionRequestPreviewResultJsonSerializer.h"
#include "RecordingActionRequestPreviewService.h"
#include "RestfulApiRecordingActionBackendConfig.h"

#include <string>

class RecordingActionValidationRequestParser;

class RecordingActionPreviewController
{
public:
    RecordingActionPreviewController(
        RecordingActionRequestPreviewService& previewService,
        RecordingActionRequestPreviewResultJsonSerializer& jsonSerializer);

    RecordingActionPreviewController(
        RecordingActionRequestPreviewService& previewService,
        RecordingActionRequestPreviewResultJsonSerializer& jsonSerializer,
        RecordingActionValidationRequestParser& requestParser);

    ApiResponse preview(
        const RecordingActionRequest& request,
        const RestfulApiRecordingActionBackendConfig& config);

    ApiResponse previewBody(
        const std::string& body,
        const RestfulApiRecordingActionBackendConfig& config);

private:
    RecordingActionRequestPreviewService& previewService_;
    RecordingActionRequestPreviewResultJsonSerializer& jsonSerializer_;
    RecordingActionValidationRequestParser* requestParser_;
};

#pragma once

#include "RecordingActionJobPayloadFactory.h"
#include "RecordingActionRequest.h"
#include "RecordingActionRequestPreviewResult.h"
#include "RecordingActionValidationService.h"
#include "RestfulApiRecordingActionBackendConfig.h"
#include "RestfulApiRecordingActionRequestBuilder.h"

class RecordingActionRequestPreviewService
{
public:
    RecordingActionRequestPreviewResult previewRestfulApiRequest(
        const RecordingActionRequest& request,
        const RestfulApiRecordingActionBackendConfig& config) const
    {
        const RecordingActionValidationResult validation =
            validationService_.validate(request);

        if (!validation.valid)
        {
            return RecordingActionRequestPreviewResult::failed(
                request.type,
                request.backendId,
                request.recordingId,
                "recording action request preview validation failed",
                validation.errors);
        }

        const RecordingActionJobPayload payload =
            payloadFactory_.create(request, validation);

        RestfulApiRecordingActionRequestBuilder builder;

        HttpRequest httpRequest;

        switch (request.type)
        {
            case RecordingActionType::Delete:
                httpRequest =
                    builder.buildDeleteRequest(config, payload);
                break;

            case RecordingActionType::Move:
                httpRequest =
                    builder.buildMoveRequest(config, payload);
                break;

            case RecordingActionType::Rename:
                httpRequest =
                    builder.buildRenameRequest(config, payload);
                break;

            default:
                return RecordingActionRequestPreviewResult::failed(
                    request.type,
                    request.backendId,
                    request.recordingId,
                    "recording action request preview action not supported",
                    {"unsupported recording action type for restfulapi request preview"});
        }

        RecordingActionRequestPreviewResult result =
            RecordingActionRequestPreviewResult::ok(
                request.type,
                request.backendId,
                request.recordingId,
                httpRequest.method,
                httpRequest.url,
                httpRequest.headers,
                httpRequest.body);

        result.warnings = validation.warnings;

        return result;
    }

private:
    RecordingActionValidationService validationService_;
    RecordingActionJobPayloadFactory payloadFactory_;
};

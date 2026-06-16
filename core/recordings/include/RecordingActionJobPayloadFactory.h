#pragma once

#include "RecordingActionJobPayload.h"
#include "RecordingActionRequest.h"
#include "RecordingActionValidationResult.h"

class RecordingActionJobPayloadFactory
{
public:
    RecordingActionJobPayload create(
        const RecordingActionRequest& request,
        const RecordingActionValidationResult& validation) const
    {
        RecordingActionJobPayload payload;

        payload.backendId = request.backendId;
        payload.recordingId = request.recordingId;
        payload.type = request.type;
        payload.dryRun = request.dryRun;
        payload.parameters = request.parameters;

        payload.requiredCapabilities =
            validation.requiredCapabilities;
        payload.requiredPermissions =
            validation.requiredPermissions;
        payload.warnings =
            validation.warnings;

        payload.jobType =
            jobTypeForAction(request.type);

        return payload;
    }

private:
    static std::string jobTypeForAction(
        RecordingActionType type)
    {
        switch (type)
        {
            case RecordingActionType::Move:
                return "recording.move";
            case RecordingActionType::Rename:
                return "recording.rename";
            case RecordingActionType::Delete:
                return "recording.delete";
            case RecordingActionType::MetadataRefresh:
                return "recording.metadata.refresh";
            case RecordingActionType::Check:
                return "recording.check";
            case RecordingActionType::Repair:
                return "recording.repair";
            case RecordingActionType::Shrink:
                return "recording.shrink";
            case RecordingActionType::Cut:
                return "recording.cut";
            case RecordingActionType::Pes2Ts:
                return "recording.pes2ts";
            case RecordingActionType::Unknown:
            default:
                return "recording.unknown";
        }
    }
};

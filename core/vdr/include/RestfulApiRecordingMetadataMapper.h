#pragma once

#include "VdrRecordingMetadata.h"

#include <string>

class RestfulApiRecordingMetadataMapper
{
public:
    // Maps the JSON object for one RESTfulAPI recording. The mapper accepts
    // native event text plus the optional additional_media object. Returned
    // artwork references remain source scoped and temporary until Phase 61.
    static VdrRecordingMetadata mapRecordingObject(
        const std::string& recordingObjectJson);
};

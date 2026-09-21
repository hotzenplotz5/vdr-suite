#pragma once

#include <string>

enum class VdrRecordingNativeCutStateAvailability
{
    Available,
    RecordingNotFound,
    IdentityAmbiguous,
    TransportError,
    InvalidPayload
};

struct VdrRecordingNativeCutState
{
    static constexpr const char* Protocol = "vdr-suite-rcut-state/1";

    VdrRecordingNativeCutStateAvailability availability =
        VdrRecordingNativeCutStateAvailability::InvalidPayload;
    bool found = false;
    std::string recordingKey;
    std::string reason;
    bool ready = false;
    bool marksReadable = false;
    std::string marksRevision;
    bool marksFilePresent = false;
    int markCount = 0;
    int sequenceCount = 0;
    int inUseFlags = 0;
    int handlerUsage = 0;
    std::string editedRecordingKey;
    bool editedDestinationExists = false;
    bool editedRecordingFound = false;
    std::string diagnostic;
};

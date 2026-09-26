#include "SuiteBridgeSvdrpTransport.h"

#include "VdrRecordingNativeIdentity.h"

#include <string>

namespace vdrsuite::agent
{

SuiteBridgeRecordingMetadataCommandReply
SuiteBridgeSvdrpTransport::requestRecordingMetadata(
    const std::string& recordingKey)
{
    SuiteBridgeRecordingMetadataCommandReply result;
    if (!VdrRecordingNativeIdentity::isValidKey(recordingKey))
    {
        return result;
    }

    const SuiteBridgeCommandReply reply = executeRequest(
        "PLUG suitebridge RMETA " + recordingKey + "\r\n");

    result.replyCode = reply.replyCode;
    result.payload = reply.payload;
    switch (reply.transportStatus)
    {
        case SuiteBridgeTransportStatus::Success:
            result.transportStatus = SuiteBridgeReadTransportStatus::Success;
            break;
        case SuiteBridgeTransportStatus::Unavailable:
            result.transportStatus = SuiteBridgeReadTransportStatus::Unavailable;
            break;
        case SuiteBridgeTransportStatus::Timeout:
            result.transportStatus = SuiteBridgeReadTransportStatus::Timeout;
            break;
        case SuiteBridgeTransportStatus::Failed:
            result.transportStatus = SuiteBridgeReadTransportStatus::Failed;
            break;
    }
    result.transportSucceeded =
        result.transportStatus == SuiteBridgeReadTransportStatus::Success &&
        reply.replyCode == 250;
    return result;
}

}

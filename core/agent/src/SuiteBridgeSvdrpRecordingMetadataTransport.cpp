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
    result.transportSucceeded =
        reply.transportStatus == SuiteBridgeTransportStatus::Success &&
        reply.replyCode == 250;
    return result;
}

}

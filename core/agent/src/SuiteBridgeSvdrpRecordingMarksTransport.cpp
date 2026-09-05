#include "SuiteBridgeSvdrpTransport.h"

#include "VdrRecordingNativeIdentity.h"

#include <string>

namespace vdrsuite::agent
{

SuiteBridgeRecordingMarksCommandReply
SuiteBridgeSvdrpTransport::requestRecordingMarks(
    const std::string& recordingKey)
{
    SuiteBridgeRecordingMarksCommandReply result;
    if (!VdrRecordingNativeIdentity::isValidKey(recordingKey))
    {
        return result;
    }

    const SuiteBridgeCommandReply reply = executeRequest(
        "PLUG suitebridge RMARKS " + recordingKey + "\r\n");

    result.replyCode = reply.replyCode;
    result.payload = reply.payload;
    result.transportSucceeded =
        reply.transportStatus == SuiteBridgeTransportStatus::Success &&
        reply.replyCode == 250;
    return result;
}

}

#include "SuiteBridgeSvdrpTransport.h"

#include "VdrRecordingNativeIdentity.h"

namespace vdrsuite::agent
{

::SuiteBridgeRecordingCutStateCommandReply
SuiteBridgeSvdrpTransport::requestRecordingCutState(
    const std::string& recordingKey)
{
    ::SuiteBridgeRecordingCutStateCommandReply result;
    if (!VdrRecordingNativeIdentity::isValidKey(recordingKey))
        return result;

    const SuiteBridgeCommandReply reply = executeRequest(
        "PLUG suitebridge RCUT " + recordingKey + "\r\n");
    result.replyCode = reply.replyCode;
    result.payload = reply.payload;
    result.transportSucceeded =
        reply.transportStatus == SuiteBridgeTransportStatus::Success &&
        reply.replyCode == 250;
    return result;
}

}

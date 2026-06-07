#include "VdrSnapshotReadJsonSerializer.h"

#include <sstream>

std::string VdrSnapshotReadJsonSerializer::serializeStatus(
    const VdrStatus& status) const
{
    std::ostringstream json;

    json
        << "{" 
        << "\"enabled\":" << (status.enabled ? "true" : "false") << ","
        << "\"mode\":\"" << status.mode << "\"," 
        << "\"host\":\"" << status.host << "\"," 
        << "\"port\":" << status.port << ","
        << "\"state\":\"" << status.state << "\""
        << "}";

    return json.str();
}

std::string VdrSnapshotReadJsonSerializer::serializeRecordings(
    const std::vector<VdrRecording>&) const
{
    return "{\"recordings\":[]}";
}

std::string VdrSnapshotReadJsonSerializer::serializeTimers(
    const std::vector<VdrTimer>&) const
{
    return "{\"timers\":[]}";
}

std::string VdrSnapshotReadJsonSerializer::serializeChannels(
    const std::vector<VdrChannel>&) const
{
    return "{\"channels\":[]}";
}

std::string VdrSnapshotReadJsonSerializer::serializeEvents(
    const std::vector<VdrEvent>&) const
{
    return "{\"events\":[]}";
}

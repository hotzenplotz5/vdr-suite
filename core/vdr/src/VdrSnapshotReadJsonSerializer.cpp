#include "VdrSnapshotReadJsonSerializer.h"

#include <sstream>

static const char* boolToJson(
    bool value)
{
    return value ? "true" : "false";
}

std::string VdrSnapshotReadJsonSerializer::serializeStatus(
    const VdrStatus& status) const
{
    std::ostringstream json;

    json
        << "{" 
        << "\"enabled\":" << boolToJson(status.enabled) << ","
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
    const std::vector<VdrChannel>& channels) const
{
    std::ostringstream json;

    json << "{\"channels\":[";

    for (std::size_t index = 0;
         index < channels.size();
         ++index)
    {
        const auto& channel = channels[index];

        if (index > 0)
        {
            json << ",";
        }

        json
            << "{"
            << "\"id\":\"" << channel.id << "\"," 
            << "\"number\":" << channel.number << ","
            << "\"name\":\"" << channel.name << "\"," 
            << "\"provider\":\"" << channel.provider << "\"," 
            << "\"group\":\"" << channel.group << "\"," 
            << "\"radio\":" << boolToJson(channel.radio) << ","
            << "\"encrypted\":" << boolToJson(channel.encrypted) << ","
            << "\"enabled\":" << boolToJson(channel.enabled)
            << "}";
    }

    json << "]}";

    return json.str();
}

std::string VdrSnapshotReadJsonSerializer::serializeEvents(
    const std::vector<VdrEvent>&) const
{
    return "{\"events\":[]}";
}

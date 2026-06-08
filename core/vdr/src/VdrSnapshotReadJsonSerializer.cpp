#include "VdrSnapshotReadJsonSerializer.h"

#include <sstream>
#include <cstddef>

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
    const std::vector<VdrRecording>& recordings) const
{
    std::ostringstream json;

    json << "{\"recordings\":[";

    for (std::size_t index = 0;
         index < recordings.size();
         ++index)
    {
        const auto& recording = recordings[index];

        if (index > 0)
        {
            json << ",";
        }

        json
            << "{"
            << "\"id\":\"" << recording.id << "\"," 
            << "\"title\":\"" << recording.title << "\"," 
            << "\"path\":\"" << recording.path << "\"," 
            << "\"startTime\":\"" << recording.startTime << "\"," 
            << "\"durationSeconds\":" << recording.durationSeconds << ","
            << "\"sizeMb\":" << recording.sizeMb
            << "}";
    }

    json << "]}";

    return json.str();
}

std::string VdrSnapshotReadJsonSerializer::serializeTimers(
    const std::vector<VdrTimer>& timers) const
{
    std::ostringstream json;

    json << "{\"timers\":[";

    for (std::size_t index = 0;
         index < timers.size();
         ++index)
    {
        const auto& timer = timers[index];

        if (index > 0)
        {
            json << ",";
        }

        json
            << "{"
            << "\"id\":\"" << timer.id << "\"," 
            << "\"channelId\":\"" << timer.channelId << "\"," 
            << "\"eventId\":\"" << timer.eventId << "\"," 
            << "\"title\":\"" << timer.title << "\"," 
            << "\"subtitle\":\"" << timer.subtitle << "\"," 
            << "\"startTime\":\"" << timer.startTime << "\"," 
            << "\"endTime\":\"" << timer.endTime << "\"," 
            << "\"priority\":" << timer.priority << ","
            << "\"lifetime\":" << timer.lifetime << ","
            << "\"enabled\":" << boolToJson(timer.enabled) << ","
            << "\"recording\":" << boolToJson(timer.recording)
            << "}";
    }

    json << "]}";

    return json.str();
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
    const std::vector<VdrEvent>& events) const
{
    std::ostringstream json;

    json << "{\"events\":[";

    for (std::size_t index = 0;
         index < events.size();
         ++index)
    {
        const auto& event = events[index];

        if (index > 0)
        {
            json << ",";
        }

        json
            << "{"
            << "\"id\":\"" << event.id << "\"," 
            << "\"channelId\":\"" << event.channelId << "\"," 
            << "\"title\":\"" << event.title << "\"," 
            << "\"subtitle\":\"" << event.subtitle << "\"," 
            << "\"description\":\"" << event.description << "\"," 
            << "\"startTime\":\"" << event.startTime << "\"," 
            << "\"endTime\":\"" << event.endTime << "\"," 
            << "\"durationSeconds\":" << event.durationSeconds << ","
            << "\"contentDescriptors\":[";

        for (std::size_t descriptorIndex = 0;
             descriptorIndex < event.contentDescriptors.size();
             ++descriptorIndex)
        {
            if (descriptorIndex > 0)
            {
                json << ",";
            }

            json
                << "\""
                << event.contentDescriptors[descriptorIndex]
                << "\"";
        }

        json
            << "],"
            << "\"parentalRating\":" << event.parentalRating
            << "}";
    }

    json << "]}";

    return json.str();
}

std::string VdrSnapshotReadJsonSerializer::serializeHealth(
    bool snapshotAvailable,
    const VdrStatus& status,
    std::size_t channelCount,
    std::size_t eventCount,
    std::size_t timerCount,
    std::size_t recordingCount) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"snapshotAvailable\":" << boolToJson(snapshotAvailable) << ","
        << "\"state\":\"" << status.state << "\","
        << "\"mode\":\"" << status.mode << "\","
        << "\"host\":\"" << status.host << "\","
        << "\"port\":" << status.port << ","
        << "\"channelCount\":" << channelCount << ","
        << "\"eventCount\":" << eventCount << ","
        << "\"timerCount\":" << timerCount << ","
        << "\"recordingCount\":" << recordingCount
        << "}";

    return json.str();
}

std::string VdrSnapshotReadJsonSerializer::serializeSnapshotSummary(
    bool snapshotAvailable,
    std::size_t channelCount,
    std::size_t eventCount,
    std::size_t timerCount,
    std::size_t recordingCount) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"snapshotAvailable\":" << boolToJson(snapshotAvailable) << ","
        << "\"channelCount\":" << channelCount << ","
        << "\"eventCount\":" << eventCount << ","
        << "\"timerCount\":" << timerCount << ","
        << "\"recordingCount\":" << recordingCount
        << "}";

    return json.str();
}

std::string VdrSnapshotReadJsonSerializer::serializeCapabilities(
    const VdrCapabilitySet& capabilities) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"snapshotRead\":" << boolToJson(capabilities.snapshotRead) << ","
        << "\"statusRead\":" << boolToJson(capabilities.statusRead) << ","
        << "\"healthRead\":" << boolToJson(capabilities.healthRead) << ","
        << "\"recordingsRead\":" << boolToJson(capabilities.recordingsRead) << ","
        << "\"timersRead\":" << boolToJson(capabilities.timersRead) << ","
        << "\"channelsRead\":" << boolToJson(capabilities.channelsRead) << ","
        << "\"eventsRead\":" << boolToJson(capabilities.eventsRead)
        << "}";

    return json.str();
}

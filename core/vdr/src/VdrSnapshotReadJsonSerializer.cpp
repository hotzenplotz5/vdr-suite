#include "VdrSnapshotReadJsonSerializer.h"

#include <cstddef>
#include <sstream>
#include <string>

namespace
{
const char* boolToJson(bool value)
{
    return value ? "true" : "false";
}

std::string jsonEscape(const std::string& value)
{
    std::string escaped;

    for (const char ch : value)
    {
        switch (ch)
        {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(ch) >= 0x20)
                {
                    escaped.push_back(ch);
                }
                break;
        }
    }

    return escaped;
}

void appendJsonString(
    std::ostringstream& json,
    const std::string& value)
{
    json << "\"" << jsonEscape(value) << "\"";
}
}

std::string VdrSnapshotReadJsonSerializer::serializeStatus(
    const VdrStatus& status) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"enabled\":" << boolToJson(status.enabled) << ","
        << "\"mode\":";
    appendJsonString(json, status.mode);
    json << ",\"host\":";
    appendJsonString(json, status.host);
    json
        << ",\"port\":" << status.port
        << ",\"state\":";
    appendJsonString(json, status.state);
    json << "}";

    return json.str();
}

std::string VdrSnapshotReadJsonSerializer::serializeRecordings(
    const std::vector<VdrRecording>& recordings) const
{
    std::ostringstream json;
    json << "{\"recordings\":[";

    for (std::size_t index = 0; index < recordings.size(); ++index)
    {
        const VdrRecording& recording = recordings[index];

        if (index > 0)
        {
            json << ",";
        }

        json << "{\"id\":";
        appendJsonString(json, recording.id);
        json << ",\"title\":";
        appendJsonString(json, recording.title);
        json << ",\"path\":";
        appendJsonString(json, recording.path);
        json << ",\"startTime\":";
        appendJsonString(json, recording.startTime);
        json
            << ",\"durationSeconds\":" << recording.durationSeconds
            << ",\"sizeMb\":" << recording.sizeMb
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

    for (std::size_t index = 0; index < timers.size(); ++index)
    {
        const VdrTimer& timer = timers[index];

        if (index > 0)
        {
            json << ",";
        }

        json << "{\"id\":";
        appendJsonString(json, timer.id);
        json << ",\"timerId\":";
        appendJsonString(json, timer.id);
        json << ",\"channelId\":";
        appendJsonString(json, timer.channelId);
        json << ",\"channelName\":";
        appendJsonString(json, timer.channelName);
        json << ",\"eventId\":";
        appendJsonString(json, timer.eventId);
        json << ",\"title\":";
        appendJsonString(json, timer.title);
        json << ",\"directory\":";
        appendJsonString(json, timer.directory);
        json << ",\"subtitle\":";
        appendJsonString(json, timer.subtitle);
        json << ",\"aux\":";
        appendJsonString(json, timer.aux);
        json << ",\"day\":";
        appendJsonString(json, timer.day);
        json << ",\"weekdays\":";
        appendJsonString(json, timer.weekdays);
        json << ",\"startTime\":";
        appendJsonString(json, timer.startTime);
        json << ",\"endTime\":";
        appendJsonString(json, timer.endTime);
        json
            << ",\"flags\":" << timer.flags
            << ",\"priority\":" << timer.priority
            << ",\"lifetime\":" << timer.lifetime
            << ",\"enabled\":" << boolToJson(timer.enabled)
            << ",\"active\":" << boolToJson(timer.enabled)
            << ",\"vps\":" << boolToJson(timer.vps)
            << ",\"recording\":" << boolToJson(timer.recording)
            << ",\"pending\":" << boolToJson(timer.pending)
            << "}";
    }

    json << "]}";
    return json.str();
}

std::string VdrSnapshotReadJsonSerializer::serializeTimerConflictReport(
    const VdrTimerConflictReport& report) const
{
    std::ostringstream json;

    json << "{\"source\":";
    appendJsonString(json, report.source);
    json
        << ",\"available\":" << boolToJson(report.available)
        << ",\"checkAdvised\":" << boolToJson(report.checkAdvised)
        << ",\"count\":" << report.count
        << ",\"total\":" << report.total;

    if (!report.error.empty())
    {
        json << ",\"error\":";
        appendJsonString(json, report.error);
    }

    json << ",\"conflicts\":[";

    for (std::size_t conflictIndex = 0;
         conflictIndex < report.conflicts.size();
         ++conflictIndex)
    {
        const VdrTimerConflict& conflict = report.conflicts[conflictIndex];

        if (conflictIndex > 0)
        {
            json << ",";
        }

        json << "{\"raw\":";
        appendJsonString(json, conflict.raw);
        json
            << ",\"conflictTime\":" << conflict.conflictTime
            << ",\"entries\":[";

        for (std::size_t entryIndex = 0;
             entryIndex < conflict.entries.size();
             ++entryIndex)
        {
            const VdrTimerConflictEntry& entry = conflict.entries[entryIndex];

            if (entryIndex > 0)
            {
                json << ",";
            }

            json
                << "{\"timerIndex\":" << entry.timerIndex
                << ",\"percentage\":" << entry.percentage
                << ",\"concurrentTimerIndices\":[";

            for (std::size_t index = 0;
                 index < entry.concurrentTimerIndices.size();
                 ++index)
            {
                if (index > 0)
                {
                    json << ",";
                }
                json << entry.concurrentTimerIndices[index];
            }

            json << "]";

            if (!entry.remoteServer.empty())
            {
                json << ",\"remoteServer\":";
                appendJsonString(json, entry.remoteServer);
            }

            json << "}";
        }

        json << "]}";
    }

    json << "]}";
    return json.str();
}

std::string VdrSnapshotReadJsonSerializer::serializeSearchTimers(
    const std::vector<SearchTimer>& searchTimers) const
{
    std::ostringstream json;
    json << "{\"searchTimers\":[";

    for (std::size_t index = 0; index < searchTimers.size(); ++index)
    {
        const SearchTimer& searchTimer = searchTimers[index];

        if (index > 0)
        {
            json << ",";
        }

        json << "{\"backendId\":";
        appendJsonString(json, searchTimer.backendId());
        json << ",\"backendNativeId\":";
        appendJsonString(json, searchTimer.backendNativeId());
        json << ",\"name\":";
        appendJsonString(json, searchTimer.name());
        json << ",\"query\":";
        appendJsonString(json, searchTimer.query());
        json
            << ",\"active\":" << boolToJson(searchTimer.isActive())
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

    for (std::size_t index = 0; index < channels.size(); ++index)
    {
        const VdrChannel& channel = channels[index];

        if (index > 0)
        {
            json << ",";
        }

        json << "{\"id\":";
        appendJsonString(json, channel.id);
        json << ",\"number\":" << channel.number << ",\"name\":";
        appendJsonString(json, channel.name);
        json << ",\"provider\":";
        appendJsonString(json, channel.provider);
        json << ",\"group\":";
        appendJsonString(json, channel.group);
        json
            << ",\"radio\":" << boolToJson(channel.radio)
            << ",\"encrypted\":" << boolToJson(channel.encrypted)
            << ",\"enabled\":" << boolToJson(channel.enabled)
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

    for (std::size_t index = 0; index < events.size(); ++index)
    {
        const VdrEvent& event = events[index];

        if (index > 0)
        {
            json << ",";
        }

        json << "{\"id\":";
        appendJsonString(json, event.id);
        json << ",\"channelId\":";
        appendJsonString(json, event.channelId);
        json << ",\"title\":";
        appendJsonString(json, event.title);
        json << ",\"subtitle\":";
        appendJsonString(json, event.subtitle);
        json << ",\"description\":";
        appendJsonString(json, event.description);
        json << ",\"startTime\":";
        appendJsonString(json, event.startTime);
        json << ",\"endTime\":";
        appendJsonString(json, event.endTime);
        json
            << ",\"durationSeconds\":" << event.durationSeconds
            << ",\"contentDescriptors\":[";

        for (std::size_t descriptorIndex = 0;
             descriptorIndex < event.contentDescriptors.size();
             ++descriptorIndex)
        {
            if (descriptorIndex > 0)
            {
                json << ",";
            }
            appendJsonString(json, event.contentDescriptors[descriptorIndex]);
        }

        json
            << "],\"parentalRating\":" << event.parentalRating
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
    std::size_t recordingCount,
    const std::string& backendId) const
{
    std::ostringstream json;

    json << "{\"backendId\":";
    appendJsonString(json, backendId);
    json
        << ",\"snapshotAvailable\":" << boolToJson(snapshotAvailable)
        << ",\"state\":";
    appendJsonString(json, status.state);
    json << ",\"mode\":";
    appendJsonString(json, status.mode);
    json << ",\"host\":";
    appendJsonString(json, status.host);
    json
        << ",\"port\":" << status.port
        << ",\"channelCount\":" << channelCount
        << ",\"eventCount\":" << eventCount
        << ",\"timerCount\":" << timerCount
        << ",\"recordingCount\":" << recordingCount
        << "}";

    return json.str();
}

std::string VdrSnapshotReadJsonSerializer::serializeSnapshotSummary(
    bool snapshotAvailable,
    std::size_t channelCount,
    std::size_t eventCount,
    std::size_t timerCount,
    std::size_t recordingCount,
    const std::string& backendId) const
{
    std::ostringstream json;

    json << "{\"backendId\":";
    appendJsonString(json, backendId);
    json
        << ",\"snapshotAvailable\":" << boolToJson(snapshotAvailable)
        << ",\"channelCount\":" << channelCount
        << ",\"eventCount\":" << eventCount
        << ",\"timerCount\":" << timerCount
        << ",\"recordingCount\":" << recordingCount
        << "}";

    return json.str();
}

std::string VdrSnapshotReadJsonSerializer::serializeSnapshots(
    const std::vector<VdrSnapshot>& snapshots) const
{
    std::ostringstream json;
    json << "{\"snapshots\":[";

    for (std::size_t index = 0; index < snapshots.size(); ++index)
    {
        const VdrSnapshot& snapshot = snapshots[index];

        if (index > 0)
        {
            json << ",";
        }

        json << "{\"backendId\":";
        appendJsonString(json, snapshot.backendId);
        json
            << ",\"snapshotAvailable\":true"
            << ",\"channelCount\":" << snapshot.channels.size()
            << ",\"eventCount\":" << snapshot.events.size()
            << ",\"timerCount\":" << snapshot.timers.size()
            << ",\"recordingCount\":" << snapshot.recordings.size()
            << "}";
    }

    json << "]}";
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

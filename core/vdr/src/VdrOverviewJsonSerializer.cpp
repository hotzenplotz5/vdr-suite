#include "VdrOverviewJsonSerializer.h"

#include <sstream>

std::string VdrOverviewJsonSerializer::serialize(
    const VdrOverview& overview) const
{
    std::ostringstream json;

    json
        << "{"

        << "\"status\":{"
        << "\"enabled\":" << (overview.status.enabled ? "true" : "false") << ","
        << "\"mode\":\"" << overview.status.mode << "\","
        << "\"host\":\"" << overview.status.host << "\","
        << "\"port\":" << overview.status.port << ","
        << "\"state\":\"" << overview.status.state << "\""
        << "},"

        << "\"channels\":{"
        << "\"totalChannels\":" << overview.totalChannels << ","
        << "\"radioChannels\":" << overview.radioChannels << ","
        << "\"encryptedChannels\":" << overview.encryptedChannels
        << "},"

        << "\"events\":{"
        << "\"totalEvents\":" << overview.totalEvents
        << "},"

        << "\"timers\":{"
        << "\"totalTimers\":" << overview.totalTimers << ","
        << "\"activeTimers\":" << overview.activeTimers << ","
        << "\"recordingTimers\":" << overview.recordingTimers << ","
        << "\"hasNextTimer\":" << (overview.hasNextTimer ? "true" : "false") << ","
        << "\"nextTimer\":{"
        << "\"id\":\"" << overview.nextTimer.id << "\","
        << "\"channelId\":\"" << overview.nextTimer.channelId << "\","
        << "\"eventId\":\"" << overview.nextTimer.eventId << "\","
        << "\"title\":\"" << overview.nextTimer.title << "\","
        << "\"subtitle\":\"" << overview.nextTimer.subtitle << "\","
        << "\"startTime\":\"" << overview.nextTimer.startTime << "\","
        << "\"endTime\":\"" << overview.nextTimer.endTime << "\","
        << "\"priority\":" << overview.nextTimer.priority << ","
        << "\"lifetime\":" << overview.nextTimer.lifetime << ","
        << "\"enabled\":" << (overview.nextTimer.enabled ? "true" : "false") << ","
        << "\"recording\":" << (overview.nextTimer.recording ? "true" : "false")
        << "}"
        << "},"

        << "\"recordings\":{"
        << "\"totalRecordings\":" << overview.totalRecordings << ","
        << "\"hasLatestRecording\":" << (overview.hasLatestRecording ? "true" : "false") << ","
        << "\"latestRecording\":{"
        << "\"id\":\"" << overview.latestRecording.id << "\","
        << "\"title\":\"" << overview.latestRecording.title << "\","
        << "\"path\":\"" << overview.latestRecording.path << "\","
        << "\"startTime\":\"" << overview.latestRecording.startTime << "\","
        << "\"durationSeconds\":" << overview.latestRecording.durationSeconds << ","
        << "\"sizeMb\":" << overview.latestRecording.sizeMb
        << "}"
        << "}"

        << "}";

    return json.str();
}

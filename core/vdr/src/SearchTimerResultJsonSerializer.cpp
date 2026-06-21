#include "SearchTimerResultJsonSerializer.h"

#include <sstream>
#include <string>

namespace {

std::string escapeJsonString(
    const std::string& value)
{
    std::ostringstream escaped;

    for (char c : value) {
        switch (c) {
        case '\\':
            escaped << "\\\\";
            break;
        case '"':
            escaped << "\\\"";
            break;
        case '\n':
            escaped << "\\n";
            break;
        case '\r':
            escaped << "\\r";
            break;
        case '\t':
            escaped << "\\t";
            break;
        default:
            escaped << c;
            break;
        }
    }

    return escaped.str();
}

std::string stateToJson(
    SearchTimerState state)
{
    if (state == SearchTimerState::Active) {
        return "active";
    }

    if (state == SearchTimerState::Inactive) {
        return "inactive";
    }

    return "unknown";
}

} // namespace

std::string SearchTimerResultJsonSerializer::serialize(
    const SearchTimerResult& result) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"totalCount\":" << result.totalCount() << ","
        << "\"returnedCount\":" << result.returnedCount() << ","
        << "\"limit\":" << result.limit() << ","
        << "\"offset\":" << result.offset() << ","
        << "\"searchtimers\":[";

    for (std::size_t index = 0;
         index < result.items().size();
         ++index)
    {
        const SearchTimer& timer = result.items().at(index);

        if (index > 0) {
            json << ",";
        }

        json
            << "{"
            << "\"backendId\":\"" << escapeJsonString(timer.backendId()) << "\","
            << "\"backendNativeId\":\"" << escapeJsonString(timer.backendNativeId()) << "\","
            << "\"name\":\"" << escapeJsonString(timer.name()) << "\","
            << "\"query\":\"" << escapeJsonString(timer.query()) << "\","
            << "\"state\":\"" << stateToJson(timer.state()) << "\","
            << "\"recordingOptions\":{"
            << "\"directory\":\"" << escapeJsonString(timer.recordingOptions().directory()) << "\","
            << "\"priority\":" << timer.recordingOptions().priority() << ","
            << "\"lifetime\":" << timer.recordingOptions().lifetime()
            << "},"
            << "\"scheduleOptions\":{"
            << "\"marginStartMinutes\":" << timer.scheduleOptions().marginStartMinutes() << ","
            << "\"marginStopMinutes\":" << timer.scheduleOptions().marginStopMinutes() << ","
            << "\"useVps\":" << (timer.scheduleOptions().useVps() ? "true" : "false")
            << "},"
            << "\"filterOptions\":{"
            << "\"useChannel\":" << (timer.filterOptions().useChannel() ? "true" : "false") << ","
            << "\"useDayOfWeek\":" << (timer.filterOptions().useDayOfWeek() ? "true" : "false") << ","
            << "\"useDuration\":" << (timer.filterOptions().useDuration() ? "true" : "false") << ","
            << "\"durationMinMinutes\":" << timer.filterOptions().durationMinMinutes() << ","
            << "\"durationMaxMinutes\":" << timer.filterOptions().durationMaxMinutes()
            << "},"
            << "\"comparisonOptions\":{"
            << "\"compareTitle\":" << (timer.comparisonOptions().compareTitle() ? "true" : "false") << ","
            << "\"compareSubtitle\":" << (timer.comparisonOptions().compareSubtitle() ? "true" : "false") << ","
            << "\"compareSummary\":" << (timer.comparisonOptions().compareSummary() ? "true" : "false") << ","
            << "\"compareCategories\":" << (timer.comparisonOptions().compareCategories() ? "true" : "false") << ","
            << "\"compareTime\":" << (timer.comparisonOptions().compareTime() ? "true" : "false")
            << "},"
            << "\"repeatOptions\":{"
            << "\"avoidRepeats\":" << (timer.repeatOptions().avoidRepeats() ? "true" : "false") << ","
            << "\"allowedRepeats\":" << timer.repeatOptions().allowedRepeats() << ","
            << "\"repeatsWithinDays\":" << timer.repeatOptions().repeatsWithinDays()
            << "},"
            << "\"channelOptions\":{"
            << "\"channels\":\"" << escapeJsonString(timer.channelOptions().channels()) << "\","
            << "\"channelMin\":" << timer.channelOptions().channelMin() << ","
            << "\"channelMax\":" << timer.channelOptions().channelMax()
            << "},"
            << "\"seriesOptions\":{"
            << "\"useSeriesRecording\":" << (timer.seriesOptions().useSeriesRecording() ? "true" : "false") << ","
            << "\"keepRecordings\":" << timer.seriesOptions().keepRecordings() << ","
            << "\"deleteMode\":" << timer.seriesOptions().deleteMode() << ","
            << "\"searchTimerAction\":" << timer.seriesOptions().searchTimerAction()
            << "},"
            << "\"blacklistOptions\":{"
            << "\"blacklistMode\":" << timer.blacklistOptions().blacklistMode() << ","
            << "\"blacklistIds\":\"" << escapeJsonString(timer.blacklistOptions().blacklistIds()) << "\""
            << "},"
            << "\"matchOptions\":{"
            << "\"mode\":" << timer.matchOptions().mode() << ","
            << "\"matchCase\":" << (timer.matchOptions().matchCase() ? "true" : "false") << ","
            << "\"tolerance\":" << timer.matchOptions().tolerance() << ","
            << "\"summaryMatch\":" << timer.matchOptions().summaryMatch()
            << "}"
            << "}";
    }

    json
        << "]"
        << "}";

    return json.str();
}
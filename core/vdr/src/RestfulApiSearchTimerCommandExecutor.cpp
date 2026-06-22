#include "RestfulApiSearchTimerCommandExecutor.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "IHttpClient.h"
#include "SearchTimer.h"
#include "SearchTimerCreateRequest.h"
#include "SearchTimerCreateResult.h"
#include "SearchTimerDeleteRequest.h"
#include "SearchTimerDeleteResult.h"
#include "SearchTimerUpdateRequest.h"
#include "SearchTimerUpdateResult.h"

#include <cctype>
#include <sstream>
#include <string>

namespace
{
std::string urlEncode(
    const std::string& value)
{
    std::ostringstream encoded;

    for (const unsigned char character : value)
    {
        if (std::isalnum(character) ||
            character == '-' ||
            character == '_' ||
            character == '.' ||
            character == '~')
        {
            encoded << character;
        }
        else
        {
            const char* digits = "0123456789ABCDEF";
            encoded << '%';
            encoded << digits[(character >> 4) & 0x0F];
            encoded << digits[character & 0x0F];
        }
    }

    return encoded.str();
}

std::string jsonEscape(
    const std::string& value)
{
    std::ostringstream escaped;

    for (const char character : value)
    {
        switch (character)
        {
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
                escaped << character;
                break;
        }
    }

    return escaped.str();
}

std::string buildSearchTimerBody(
    const std::string& query,
    const bool active,
    const std::string& directory,
    const int priority,
    const int lifetime,
    const int marginStartMinutes,
    const int marginStopMinutes,
    const bool useVps,
    const int useChannel,
    const std::string& channels,
    const std::string& channelMin,
    const std::string& channelMax,
    const bool useTime,
    const int startTime,
    const int stopTime,
    const bool useDuration,
    const int durationMinMinutes,
    const int durationMaxMinutes,
    const bool useDayOfWeek,
    const int dayOfWeek,
    const bool avoidRepeats,
    const int allowedRepeats,
    const int repeatsWithinDays,
    const bool compareTitle,
    const bool compareSubtitle,
    const bool compareSummary,
    const bool compareCategories,
    const bool compareTime,
    const bool useSeriesRecording,
    const int keepRecordings,
    const int deleteMode,
    const int searchTimerAction,
    const int blacklistMode,
    const std::string& blacklistIds,
    const int matchMode,
    const bool matchCase,
    const int matchTolerance,
    const int summaryMatch,
    const bool useExtendedEpgInfo,
    const std::string& extendedEpgInfo,
    const bool ignoreMissingEpgCategories,
    const std::string& contentDescriptors)
{
    std::ostringstream body;

    body
        << "{"
        << "\"search\":\"" << jsonEscape(query) << "\","
        << "\"use_title\":true,"
        << "\"use_subtitle\":true,"
        << "\"use_description\":true,"
        << "\"use_channel\":" << useChannel << ","
        << "\"channels\":\"" << jsonEscape(channels) << "\","
        << "\"channel_min\":\"" << jsonEscape(channelMin) << "\","
        << "\"channel_max\":\"" << jsonEscape(channelMax) << "\","
        << "\"use_time\":" << (useTime ? "1" : "0") << ","
        << "\"start_time\":" << startTime << ","
        << "\"stop_time\":" << stopTime << ","
        << "\"use_duration\":" << (useDuration ? "1" : "0") << ","
        << "\"duration_min\":" << durationMinMinutes << ","
        << "\"duration_max\":" << durationMaxMinutes << ","
        << "\"use_dayofweek\":" << (useDayOfWeek ? "1" : "0") << ","
        << "\"dayofweek\":" << dayOfWeek << ","
        << "\"avoid_repeats\":" << (avoidRepeats ? "1" : "0") << ","
        << "\"allowed_repeats\":" << allowedRepeats << ","
        << "\"repeats_within_days\":" << repeatsWithinDays << ","
        << "\"compare_title\":" << (compareTitle ? "1" : "0") << ","
        << "\"compare_subtitle\":" << (compareSubtitle ? "1" : "0") << ","
        << "\"compare_summary\":" << (compareSummary ? "1" : "0") << ","
        << "\"compare_categories\":" << (compareCategories ? "1" : "0") << ","
        << "\"compare_time\":" << (compareTime ? "1" : "0") << ","
        << "\"use_series_recording\":" << (useSeriesRecording ? "1" : "0") << ","
        << "\"keep_recs\":" << keepRecordings << ","
        << "\"del_mode\":" << deleteMode << ","
        << "\"search_timer_action\":" << searchTimerAction << ","
        << "\"blacklist_mode\":" << blacklistMode << ","
        << "\"blacklist_ids\":\"" << jsonEscape(blacklistIds) << "\","
        << "\"mode\":" << matchMode << ","
        << "\"match_case\":" << (matchCase ? "1" : "0") << ","
        << "\"tolerance\":" << matchTolerance << ","
        << "\"summary_match\":" << summaryMatch << ","
        << "\"use_ext_epg_info\":" << (useExtendedEpgInfo ? "1" : "0") << ","
        << "\"ext_epg_info\":\"" << jsonEscape(extendedEpgInfo) << "\","
        << "\"ignore_missing_epg_cats\":" << (ignoreMissingEpgCategories ? "1" : "0") << ","
        << "\"content_descriptors\":\"" << jsonEscape(contentDescriptors) << "\","
        << "\"directory\":\"" << jsonEscape(directory) << "\","
        << "\"priority\":" << priority << ","
        << "\"lifetime\":" << lifetime << ","
        << "\"margin_start\":" << marginStartMinutes << ","
        << "\"margin_stop\":" << marginStopMinutes << ","
        << "\"use_vps\":" << (useVps ? "1" : "0") << ","
        << "\"use_as_searchtimer\":" << (active ? "1" : "0")
        << "}";

    return body.str();
}

std::string extractReturnedId(
    const std::string& body)
{
    const std::string marker = "Id:";
    const std::size_t markerPosition =
        body.find(marker);

    if (markerPosition == std::string::npos)
    {
        return "";
    }

    std::size_t idStart =
        markerPosition + marker.size();

    while (idStart < body.size() &&
           std::isspace(static_cast<unsigned char>(body[idStart])))
    {
        ++idStart;
    }

    std::size_t idEnd = idStart;

    while (idEnd < body.size() &&
           std::isdigit(static_cast<unsigned char>(body[idEnd])))
    {
        ++idEnd;
    }

    return body.substr(idStart, idEnd - idStart);
}
}

RestfulApiSearchTimerCommandExecutor::RestfulApiSearchTimerCommandExecutor(
    IHttpClient& httpClient)
    : httpClient_(httpClient)
{
}

SearchTimerCreateResult RestfulApiSearchTimerCommandExecutor::create(
    const SearchTimerCreateRequest& request)
{
    HttpRequest httpRequest;
    httpRequest.method = "POST";
    httpRequest.url = "/searchtimers";
    httpRequest.headers["Accept"] = "text/plain";
    httpRequest.headers["Content-Type"] = "application/json";
    httpRequest.body =
        buildSearchTimerBody(
            request.query,
            request.active,
            request.directory,
            request.priority,
            request.lifetime,
            request.marginStartMinutes,
            request.marginStopMinutes,
            request.useVps,
            request.useChannel,
            request.channels,
            request.channelMin,
            request.channelMax,
            request.useTime,
            request.startTime,
            request.stopTime,
            request.useDuration,
            request.durationMinMinutes,
            request.durationMaxMinutes,
            request.useDayOfWeek,
            request.dayOfWeek,
            request.avoidRepeats,
            request.allowedRepeats,
            request.repeatsWithinDays,
            request.compareTitle,
            request.compareSubtitle,
            request.compareSummary,
            request.compareCategories,
            request.compareTime,
            request.useSeriesRecording,
            request.keepRecordings,
            request.deleteMode,
            request.searchTimerAction,
            request.blacklistMode,
            request.blacklistIds,
            request.matchMode,
            request.matchCase,
            request.matchTolerance,
            request.summaryMatch,
            request.useExtendedEpgInfo,
            request.extendedEpgInfo,
            request.ignoreMissingEpgCategories,
            request.contentDescriptors);

    const HttpResponse response =
        httpClient_.execute(httpRequest);

    if (response.statusCode != 200)
    {
        return SearchTimerCreateResult::failed(
            "RESTfulAPI searchtimer create failed",
            {
                "status=" + std::to_string(response.statusCode),
                "requestBody=" + httpRequest.body,
                "responseBody=" + response.body
            });
    }

    const std::string createdId =
        extractReturnedId(response.body);

    if (createdId.empty())
    {
        return SearchTimerCreateResult::failed(
            "RESTfulAPI searchtimer create did not return an id",
            {response.body});
    }

    return SearchTimerCreateResult::ok(
        SearchTimer::create(
            SearchTimerId::fromBackendNativeId(
                request.backendId,
                createdId),
            request.name,
            request.query,
            request.active
                ? SearchTimerState::Active
                : SearchTimerState::Inactive),
        "searchtimer created through RESTfulAPI");
}

SearchTimerUpdateResult RestfulApiSearchTimerCommandExecutor::update(
    const SearchTimerUpdateRequest& request)
{
    HttpRequest httpRequest;
    httpRequest.method = "PUT";
    httpRequest.url = "/searchtimers/";
    httpRequest.url += urlEncode(request.backendNativeId);
    httpRequest.headers["Accept"] = "text/plain";
    httpRequest.headers["Content-Type"] = "application/json";
    httpRequest.body =
        buildSearchTimerBody(
            request.query,
            request.active,
            request.directory,
            request.priority,
            request.lifetime,
            request.marginStartMinutes,
            request.marginStopMinutes,
            request.useVps,
            request.useChannel,
            request.channels,
            request.channelMin,
            request.channelMax,
            request.useTime,
            request.startTime,
            request.stopTime,
            request.useDuration,
            request.durationMinMinutes,
            request.durationMaxMinutes,
            request.useDayOfWeek,
            request.dayOfWeek,
            request.avoidRepeats,
            request.allowedRepeats,
            request.repeatsWithinDays,
            request.compareTitle,
            request.compareSubtitle,
            request.compareSummary,
            request.compareCategories,
            request.compareTime,
            request.useSeriesRecording,
            request.keepRecordings,
            request.deleteMode,
            request.searchTimerAction,
            request.blacklistMode,
            request.blacklistIds,
            request.matchMode,
            request.matchCase,
            request.matchTolerance,
            request.summaryMatch,
            request.useExtendedEpgInfo,
            request.extendedEpgInfo,
            request.ignoreMissingEpgCategories,
            request.contentDescriptors);

    const HttpResponse response =
        httpClient_.execute(httpRequest);

    if (response.statusCode != 200)
    {
        return SearchTimerUpdateResult::failed(
            "RESTfulAPI searchtimer update failed",
            {response.body});
    }

    const std::string updatedId =
        extractReturnedId(response.body);

    const std::string effectiveUpdatedId =
        updatedId.empty()
            ? request.backendNativeId
            : updatedId;

    return SearchTimerUpdateResult::ok(
        SearchTimer::create(
            SearchTimerId::fromBackendNativeId(
                request.backendId,
                effectiveUpdatedId),
            request.name,
            request.query,
            request.active
                ? SearchTimerState::Active
                : SearchTimerState::Inactive),
        "searchtimer updated through RESTfulAPI");
}

SearchTimerDeleteResult RestfulApiSearchTimerCommandExecutor::remove(
    const SearchTimerDeleteRequest& request)
{
    HttpRequest httpRequest;
    httpRequest.method = "DELETE";
    httpRequest.url = "/searchtimers/";
    httpRequest.url += urlEncode(request.backendNativeId);
    httpRequest.headers["Accept"] = "text/plain";

    const HttpResponse response =
        httpClient_.execute(httpRequest);

    if (response.statusCode != 200)
    {
        return SearchTimerDeleteResult::failed(
            "RESTfulAPI searchtimer delete failed",
            {response.body});
    }

    return SearchTimerDeleteResult::ok(
        request.backendId,
        request.backendNativeId,
        "searchtimer deleted through RESTfulAPI");
}

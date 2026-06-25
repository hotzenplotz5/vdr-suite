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
    const std::string& contentDescriptors,
    const bool useInFavorites,
    const std::string& activeFrom,
    const std::string& activeUntil,
    const bool pauseOnRecordings,
    const int switchMinutesBefore,
    const bool unmuteSoundOnSwitch,
    const int deleteRecordingsAfterDays,
    const int deleteAfterCountRecordings,
    const int deleteAfterDaysOfFirstRecording)
{
    std::ostringstream body;

    body
        << "{"
        << "\"search\":\"" << jsonEscape(query) << "\","
        << "\"use_title\":" << (compareTitle ? "1" : "0") << ","
        << "\"use_subtitle\":" << (compareSubtitle ? "1" : "0") << ","
        << "\"use_description\":" << (compareSummary ? "1" : "0") << ","
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
        << "\"use_in_favorites\":" << (useInFavorites ? "1" : "0") << ","
        << "\"use_as_searchtimer_from\":\"" << jsonEscape(activeFrom) << "\","
        << "\"use_as_searchtimer_til\":\"" << jsonEscape(activeUntil) << "\","
        << "\"pause_on_recs\":" << (pauseOnRecordings ? "1" : "0") << ","
        << "\"switch_min_before\":" << switchMinutesBefore << ","
        << "\"unmute_sound_on_switch\":" << (unmuteSoundOnSwitch ? "1" : "0") << ","
        << "\"del_recs_after_days\":" << deleteRecordingsAfterDays << ","
        << "\"del_after_count_recs\":" << deleteAfterCountRecordings << ","
        << "\"del_after_days_of_first_rec\":" << deleteAfterDaysOfFirstRecording << ","
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

std::string extractJsonStringField(
    const std::string& object,
    const std::string& fieldName)
{
    const std::string fieldMarker =
        "\"" + fieldName + "\"";

    const std::size_t fieldPosition =
        object.find(fieldMarker);

    if (fieldPosition == std::string::npos)
    {
        return "";
    }

    const std::size_t colonPosition =
        object.find(':', fieldPosition + fieldMarker.size());

    if (colonPosition == std::string::npos)
    {
        return "";
    }

    std::size_t valuePosition =
        object.find_first_not_of(" \t\n\r", colonPosition + 1);

    if (valuePosition == std::string::npos ||
        object[valuePosition] != '"')
    {
        return "";
    }

    std::ostringstream value;
    bool escaped = false;

    for (++valuePosition; valuePosition < object.size(); ++valuePosition)
    {
        const char character = object[valuePosition];

        if (escaped)
        {
            switch (character)
            {
                case 'n':
                    value << '\n';
                    break;
                case 'r':
                    value << '\r';
                    break;
                case 't':
                    value << '\t';
                    break;
                default:
                    value << character;
                    break;
            }

            escaped = false;
            continue;
        }

        if (character == '\\')
        {
            escaped = true;
            continue;
        }

        if (character == '"')
        {
            return value.str();
        }

        value << character;
    }

    return "";
}

std::string extractJsonIntegerField(
    const std::string& object,
    const std::string& fieldName)
{
    const std::string fieldMarker =
        "\"" + fieldName + "\"";

    const std::size_t fieldPosition =
        object.find(fieldMarker);

    if (fieldPosition == std::string::npos)
    {
        return "";
    }

    const std::size_t colonPosition =
        object.find(':', fieldPosition + fieldMarker.size());

    if (colonPosition == std::string::npos)
    {
        return "";
    }

    std::size_t valuePosition =
        object.find_first_not_of(" \t\n\r", colonPosition + 1);

    if (valuePosition == std::string::npos)
    {
        return "";
    }

    if (object[valuePosition] == '"')
    {
        ++valuePosition;
    }

    const std::size_t valueStart = valuePosition;

    while (valuePosition < object.size() &&
           std::isdigit(static_cast<unsigned char>(object[valuePosition])))
    {
        ++valuePosition;
    }

    if (valuePosition == valueStart)
    {
        return "";
    }

    return object.substr(valueStart, valuePosition - valueStart);
}

bool findNextJsonObject(
    const std::string& body,
    std::size_t& position,
    std::string& object)
{
    const std::size_t start =
        body.find('{', position);

    if (start == std::string::npos)
    {
        return false;
    }

    int depth = 0;
    bool insideString = false;
    bool escaped = false;

    for (std::size_t index = start; index < body.size(); ++index)
    {
        const char character = body[index];

        if (escaped)
        {
            escaped = false;
            continue;
        }

        if (character == '\\' && insideString)
        {
            escaped = true;
            continue;
        }

        if (character == '"')
        {
            insideString = !insideString;
            continue;
        }

        if (insideString)
        {
            continue;
        }

        if (character == '{')
        {
            ++depth;
            continue;
        }

        if (character == '}')
        {
            --depth;

            if (depth == 0)
            {
                object = body.substr(start, index - start + 1);
                position = start + 1;
                return true;
            }
        }
    }

    return false;
}

std::string extractUniqueSearchTimerIdByExactQuery(
    const std::string& body,
    const std::string& query)
{
    std::size_t position = 0;
    std::string object;
    std::string foundId;
    int matches = 0;

    while (findNextJsonObject(body, position, object))
    {
        if (object.find("\"searchtimers\"") != std::string::npos)
        {
            continue;
        }

        if (extractJsonStringField(object, "search") != query)
        {
            continue;
        }

        const std::string id =
            extractJsonIntegerField(object, "id");

        if (id.empty())
        {
            continue;
        }

        foundId = id;
        ++matches;
    }

    return matches == 1 ? foundId : "";
}

std::string readBackCreatedIdByExactQuery(
    IHttpClient& httpClient,
    const std::string& query)
{
    HttpRequest readbackRequest;
    readbackRequest.method = "GET";
    readbackRequest.url = "/searchtimers.json";
    readbackRequest.headers["Accept"] = "application/json";

    const HttpResponse readbackResponse =
        httpClient.execute(readbackRequest);

    if (readbackResponse.statusCode != 200)
    {
        return "";
    }

    return extractUniqueSearchTimerIdByExactQuery(
        readbackResponse.body,
        query);
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
            request.contentDescriptors,
            request.useInFavorites,
            request.activeFrom,
            request.activeUntil,
            request.pauseOnRecordings,
            request.switchMinutesBefore,
            request.unmuteSoundOnSwitch,
            request.deleteRecordingsAfterDays,
            request.deleteAfterCountRecordings,
            request.deleteAfterDaysOfFirstRecording);

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

    std::string createdId =
        extractReturnedId(response.body);

    if (createdId.empty())
    {
        createdId =
            readBackCreatedIdByExactQuery(
                httpClient_,
                request.query);
    }

    if (createdId.empty())
    {
        return SearchTimerCreateResult::failed(
            "RESTfulAPI searchtimer create did not return an id",
            {
                response.body,
                "readback did not find a unique created searchtimer id for query: "
                    + request.query
            });
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
            request.contentDescriptors,
            request.useInFavorites,
            request.activeFrom,
            request.activeUntil,
            request.pauseOnRecordings,
            request.switchMinutesBefore,
            request.unmuteSoundOnSwitch,
            request.deleteRecordingsAfterDays,
            request.deleteAfterCountRecordings,
            request.deleteAfterDaysOfFirstRecording);

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

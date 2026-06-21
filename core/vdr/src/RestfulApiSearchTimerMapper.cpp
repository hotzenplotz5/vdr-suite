#include "RestfulApiSearchTimerMapper.h"

#include <cctype>
#include <string>
#include <vector>

namespace {

std::size_t skipWhitespace(const std::string& text, std::size_t pos)
{
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
        ++pos;
    }
    return pos;
}

std::size_t findMatching(const std::string& text, std::size_t start, char openChar, char closeChar)
{
    bool inString = false;
    bool escaped = false;
    int depth = 0;

    for (std::size_t i = start; i < text.size(); ++i) {
        char c = text[i];

        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }

        if (c == '"') {
            inString = true;
            continue;
        }

        if (c == openChar) {
            ++depth;
        } else if (c == closeChar) {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }

    return std::string::npos;
}

std::string getArrayField(
    const std::string& objectText,
    const std::string& fieldName)
{
    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return "";
    }

    std::size_t colon = objectText.find(':', keyPos + key.size());
    if (colon == std::string::npos) {
        return "";
    }

    std::size_t arrayStart = objectText.find('[', colon + 1);
    if (arrayStart == std::string::npos) {
        return "";
    }

    std::size_t arrayEnd = findMatching(objectText, arrayStart, '[', ']');
    if (arrayEnd == std::string::npos) {
        return "";
    }

    return objectText.substr(arrayStart, arrayEnd - arrayStart + 1);
}

std::vector<std::string> splitTopLevelObjects(const std::string& arrayText)
{
    std::vector<std::string> objects;
    std::size_t pos = 0;

    while (pos < arrayText.size()) {
        std::size_t start = arrayText.find('{', pos);
        if (start == std::string::npos) {
            break;
        }

        std::size_t end = findMatching(arrayText, start, '{', '}');
        if (end == std::string::npos) {
            break;
        }

        objects.push_back(arrayText.substr(start, end - start + 1));
        pos = end + 1;
    }

    return objects;
}

std::string unescapeJsonString(const std::string& value)
{
    std::string result;
    bool escaped = false;

    for (char c : value) {
        if (escaped) {
            switch (c) {
            case 'n':
                result.push_back('\n');
                break;
            case 'r':
                result.push_back('\r');
                break;
            case 't':
                result.push_back('\t');
                break;
            case '"':
            case '\\':
            case '/':
                result.push_back(c);
                break;
            default:
                result.push_back(c);
                break;
            }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else {
            result.push_back(c);
        }
    }

    return result;
}

std::string getStringField(const std::string& objectText, const std::string& fieldName)
{
    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return "";
    }

    std::size_t colon = objectText.find(':', keyPos + key.size());
    if (colon == std::string::npos) {
        return "";
    }

    std::size_t quoteStart = objectText.find('"', colon + 1);
    if (quoteStart == std::string::npos) {
        return "";
    }

    bool escaped = false;
    for (std::size_t i = quoteStart + 1; i < objectText.size(); ++i) {
        char c = objectText[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') {
            return unescapeJsonString(objectText.substr(quoteStart + 1, i - quoteStart - 1));
        }
    }

    return "";
}

int getIntField(const std::string& objectText, const std::string& fieldName, int fallback = 0)
{
    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return fallback;
    }

    std::size_t colon = objectText.find(':', keyPos + key.size());
    if (colon == std::string::npos) {
        return fallback;
    }

    std::size_t pos = skipWhitespace(objectText, colon + 1);
    if (pos >= objectText.size()) {
        return fallback;
    }

    bool negative = false;
    if (objectText[pos] == '-') {
        negative = true;
        ++pos;
    }

    if (pos >= objectText.size() || !std::isdigit(static_cast<unsigned char>(objectText[pos]))) {
        return fallback;
    }

    int value = 0;
    while (pos < objectText.size() && std::isdigit(static_cast<unsigned char>(objectText[pos]))) {
        value = value * 10 + (objectText[pos] - '0');
        ++pos;
    }

    return negative ? -value : value;
}

SearchTimerState mapState(const std::string& objectText)
{
    const int useAsSearchTimer = getIntField(objectText, "use_as_searchtimer", 1);
    return useAsSearchTimer == 0
        ? SearchTimerState::Inactive
        : SearchTimerState::Active;
}

} // namespace

std::vector<SearchTimer> RestfulApiSearchTimerMapper::parseSearchTimers(
    const std::string& backendId,
    const std::string& json)
{
    std::vector<SearchTimer> timers;
    const std::string arrayText = getArrayField(json, "searchtimers");

    for (const std::string& objectText : splitTopLevelObjects(arrayText)) {
        const int id = getIntField(objectText, "id", -1);
        const std::string search = getStringField(objectText, "search");

        if (backendId.empty() || id < 0 || search.empty()) {
            continue;
        }

        SearchTimer timer = SearchTimer::create(
            SearchTimerId::fromBackendNativeId(
                backendId,
                std::to_string(id)),
            search,
            search,
            mapState(objectText));

        timer.recordingOptions().setDirectory(
            getStringField(objectText, "directory"));
        timer.recordingOptions().setPriority(
            getIntField(objectText, "priority", 0));
        timer.recordingOptions().setLifetime(
            getIntField(objectText, "lifetime", 0));
        timer.scheduleOptions().setMarginStartMinutes(
            getIntField(objectText, "margin_start", 0));
        timer.scheduleOptions().setMarginStopMinutes(
            getIntField(objectText, "margin_stop", 0));
        timer.scheduleOptions().setUseVps(
            getIntField(objectText, "use_vps", 0) != 0);
        timer.filterOptions().setUseChannel(
            getIntField(objectText, "use_channel", 0) != 0);
        timer.filterOptions().setUseDayOfWeek(
            getIntField(objectText, "use_dayofweek", 0) != 0);
        timer.filterOptions().setUseDuration(
            getIntField(objectText, "use_duration", 0) != 0);
        timer.filterOptions().setDurationMinMinutes(
            getIntField(objectText, "duration_min", 0));
        timer.filterOptions().setDurationMaxMinutes(
            getIntField(objectText, "duration_max", 0));

        timer.comparisonOptions().setCompareTitle(
            getIntField(objectText, "compare_title", 0) != 0);
        timer.comparisonOptions().setCompareSubtitle(
            getIntField(objectText, "compare_subtitle", 0) != 0);
        timer.comparisonOptions().setCompareSummary(
            getIntField(objectText, "compare_summary", 0) != 0);
        timer.comparisonOptions().setCompareCategories(
            getIntField(objectText, "compare_categories", 0) != 0);
        timer.comparisonOptions().setCompareTime(
            getIntField(objectText, "compare_time", 0) != 0);

        timer.repeatOptions().setAvoidRepeats(
            getIntField(objectText, "avoid_repeats", 0) != 0);
        timer.repeatOptions().setAllowedRepeats(
            getIntField(objectText, "allowed_repeats", 0));
        timer.repeatOptions().setRepeatsWithinDays(
            getIntField(objectText, "repeats_within_days", 0));

        timer.channelOptions().setChannels(
            getStringField(objectText, "channels"));
        timer.channelOptions().setChannelMin(
            getIntField(objectText, "channel_min", 0));
        timer.channelOptions().setChannelMax(
            getIntField(objectText, "channel_max", 0));

        timer.seriesOptions().setUseSeriesRecording(
            getIntField(objectText, "use_series_recording", 0) != 0);
        timer.seriesOptions().setKeepRecordings(
            getIntField(objectText, "keep_recs", 0));
        timer.seriesOptions().setDeleteMode(
            getIntField(objectText, "del_mode", 0));
        timer.seriesOptions().setSearchTimerAction(
            getIntField(objectText, "search_timer_action", 0));

        timers.push_back(timer);
    }

    return timers;
}
#pragma once

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace vdrsuite::agent
{
// Standard epgsearch LSTS/EDIS format. Preserve every native field,
// including newer trailing fields invisible to the RESTfulAPI projection.
class EpgsearchStockRule
{
public:
    static bool nativeIdValid(const std::string& id)
    {
        return !id.empty() && id.size() <= 10U &&
            (id.size() == 1U || id.front() != '0') &&
            (id.size() < 10U || id <= "2147483647") &&
            std::all_of(id.begin(), id.end(), [](char c) {
                return c >= '0' && c <= '9';
            });
    }

    static bool singleLine(const std::string& text)
    {
        return !text.empty() && std::none_of(text.begin(), text.end(),
            [](char c) { return c == '\r' || c == '\n' || c == '\0'; });
    }

    static std::optional<EpgsearchStockRule> parse(
        const std::string& nativeId, const std::string& raw)
    {
        if (!nativeIdValid(nativeId) || !singleLine(raw)) return std::nullopt;
        EpgsearchStockRule out;
        std::size_t begin = 0U;
        for (;;)
        {
            const auto end = raw.find(':', begin);
            out.fields_.push_back(raw.substr(begin,
                end == std::string::npos ? end : end - begin));
            if (end == std::string::npos) break;
            begin = end + 1U;
        }
        // The stock 54-field form and its subsequent extensions share
        // these positions; don't truncate or manufacture default fields.
        if (out.fields_.size() < 54U || out.fields_[0] != nativeId ||
            out.fields_[1].empty() ||
            (out.fields_[15] != "0" && out.fields_[15] != "1" &&
             out.fields_[15] != "2")) return std::nullopt;
        return out;
    }

    std::string text() const
    {
        std::string out;
        for (std::size_t i = 0; i < fields_.size(); ++i)
        {
            if (i) out += ':';
            out += fields_[i];
        }
        return out;
    }

    std::string query() const
    {
        std::string decoded;
        const auto& encoded = fields_[1];
        const std::string marker = "!^pipe^!";
        for (std::size_t i = 0; i < encoded.size();)
        {
            if (encoded.compare(i, marker.size(), marker) == 0)
            {
                decoded += '|';
                i += marker.size();
            }
            else
            {
                decoded += encoded[i] == '|' ? ':' : encoded[i];
                ++i;
            }
        }
        return decoded;
    }

    bool active() const { return fields_[15] != "0"; }

    std::optional<EpgsearchStockRule> withQuery(
        const std::string& queryText) const
    {
        if (!singleLine(queryText)) return std::nullopt;
        EpgsearchStockRule out = *this;
        std::string encoded;
        for (char c : queryText)
        {
            if (c == '|') encoded += "!^pipe^!";
            else encoded += c == ':' ? '|' : c;
        }
        out.fields_[1] = std::move(encoded);
        if (out.query() != queryText) return std::nullopt;
        return out;
    }

    // Apply only explicitly supplied Public-v1 settings to the original
    // native record. All other fields (including future native fields) survive.
    std::optional<EpgsearchStockRule> withSettings(
        const std::map<std::string, std::string>& settings) const
    {
        static const std::map<std::string, std::size_t> positions = {
            {"useTime", 2U},
            {"useChannel", 5U},
            {"useDuration", 12U},
            {"activeMode", 15U},
            {"blacklistMode", 39U},
            {"matchCase", 7U},
            {"compareTitle", 9U},
            {"compareSubtitle", 10U},
            {"compareSummary", 11U},
            {"useDayOfWeek", 16U},
            {"useSeriesRecording", 18U},
            {"useVps", 24U},
            {"useExtendedEpgInfo", 26U},
            {"avoidRepeats", 28U},
            {"repeatCompareTitle", 30U},
            {"repeatCompareSummary", 32U},
            {"useInFavorites", 42U},
            {"unmuteSoundOnSwitch", 50U},
            {"mode", 8U},
            {"dayOfWeek", 17U},
            {"priority", 20U},
            {"lifetime", 21U},
            {"marginStartMinutes", 22U},
            {"marginStopMinutes", 23U},
            {"searchTimerAction", 25U},
            {"allowedRepeats", 29U},
            {"repeatsWithinDays", 34U},
            {"deleteRecordingsAfterDays", 35U},
            {"keepRecordings", 36U},
            {"switchMinutesBefore", 37U},
            {"pauseOnRecordings", 38U},
            {"tolerance", 41U},
            {"deleteMode", 44U},
            {"deleteAfterCountRecordings", 45U},
            {"deleteAfterDaysOfFirstRecording", 46U},
            {"summaryMatch", 51U},
            {"repeatCompareSubtitleMode", 31U},
            {"repeatCompareTimeMode", 53U},
        };
        EpgsearchStockRule out = *this;
        for (const auto& [key, value] : settings)
        {
            if (key == "directory")
            {
                if (value.find("!^pipe^!") != std::string::npos ||
                    value.find_first_of("\r\n") != std::string::npos ||
                    value.find('\0') != std::string::npos) return std::nullopt;
                std::string encoded;
                for (char c : value)
                {
                    if (c == '|') encoded += "!^pipe^!";
                    else encoded += c == ':' ? '|' : c;
                }
                out.fields_[19] = std::move(encoded);
            }
            else if (key == "startTime" || key == "stopTime" ||
                     key == "durationMinMinutes" || key == "durationMaxMinutes" ||
                     key == "channels" || key == "channelMin" || key == "channelMax" ||
                     key == "blacklistIds" || key == "extendedEpgInfo" ||
                     key == "contentDescriptors" || key == "activeFrom" ||
                     key == "activeUntil" || key == "repeatCompareCategoriesMask")
            {
                // Related fields are processed together below, so request key
                // ordering cannot change the native result.
                continue;
            }
            else if (key == "repeatCompareSubtitle" || key == "compareTime" ||
                     key == "compareCategories" || key == "ignoreMissingEpgCategories")
            {
                const auto index = key == "repeatCompareSubtitle" ? 31U :
                    key == "compareTime" ? 53U : key == "compareCategories" ? 33U : 49U;
                if (value == "0") out.fields_[index] = "0";
                else if (value == "1")
                {
                    if (out.fields_[index].empty() || out.fields_[index] == "0")
                        out.fields_[index] = "1";
                }
                else return std::nullopt;
            }
            else
            {
                const auto pos = positions.find(key);
                if (pos == positions.end() ||
                    value.empty() ||
                    value.find_first_not_of("0123456789-") != std::string::npos)
                    return std::nullopt;
                out.fields_[pos->second] = value;
            }
        }
        // Explicit mode overrides a boolean convenience field, independent
        // of JSON member order.
        for (const auto* key : {"repeatCompareSubtitleMode", "repeatCompareTimeMode"})
        {
            const auto it = settings.find(key);
            if (it != settings.end()) out.fields_[positions.at(key)] = it->second;
        }
        const auto supplied = [&settings](const char* key) {
            return settings.find(key) != settings.end();
        };
        const auto valueOr = [&settings](const char* key, const std::string& old) {
            const auto it = settings.find(key);
            return it == settings.end() ? old : it->second;
        };
        const auto nativeText = [](const std::string& value) {
            return value.find_first_of(":\r\n") == std::string::npos &&
                value.find('\0') == std::string::npos;
        };
        const auto clockValue = [](const std::string& value) {
            return !value.empty() && value.size() <= 4U &&
                value.find_first_not_of("0123456789") == std::string::npos;
        };
        // Native disabled time/duration filters serialize empty endpoints;
        // enabled values use the provider's HHMM representation.
        const auto applyRange = [&](const char* enabled, const char* start,
            const char* stop, std::size_t index) {
            if (!supplied(enabled) && !supplied(start) && !supplied(stop)) return true;
            if (out.fields_[index] == "0")
            {
                out.fields_[index + 1U].clear();
                out.fields_[index + 2U].clear();
                return true;
            }
            const auto applyEndpoint = [&](const char* key, std::size_t position,
                const char* fallback) {
                auto value = valueOr(key, out.fields_[position]);
                if (value.empty()) value = fallback;
                if (!clockValue(value)) return false;
                out.fields_[position] = std::string(4U - value.size(), '0') + value;
                return true;
            };
            return applyEndpoint(start, index + 1U, "0") &&
                applyEndpoint(stop, index + 2U, "2359");
        };
        if (!applyRange("useTime", "startTime", "stopTime", 2U) ||
            !applyRange("useDuration", "durationMinMinutes", "durationMaxMinutes", 12U))
            return std::nullopt;
        if (supplied("useChannel") || supplied("channels") ||
            supplied("channelMin") || supplied("channelMax"))
        {
            if (out.fields_[5] == "0" || out.fields_[5] == "3") out.fields_[6] = "0";
            else if (out.fields_[5] == "2")
            {
                const auto group = valueOr("channels", fields_[5] == "2" ? fields_[6] : "");
                if (group.empty() || !nativeText(group)) return std::nullopt;
                out.fields_[6] = group;
            }
            else if (out.fields_[5] == "1")
            {
                const auto current = fields_[5] == "1" ? fields_[6] : "";
                const auto separator = current.find('|');
                const auto oldMin = current.substr(0, separator);
                const auto oldMax = separator == std::string::npos ? oldMin : current.substr(separator + 1U);
                const auto min = valueOr("channelMin", oldMin);
                const auto max = valueOr("channelMax", oldMax.empty() ? min : oldMax);
                if (min.empty() || max.empty() || !nativeText(min) || !nativeText(max) ||
                    min.find('|') != std::string::npos || max.find('|') != std::string::npos)
                    return std::nullopt;
                out.fields_[6] = min == max ? min : min + "|" + max;
            }
            else return std::nullopt;
        }
        if (supplied("blacklistMode") || supplied("blacklistIds"))
        {
            if (out.fields_[39] != "1") out.fields_[40].clear();
            else if (supplied("blacklistIds"))
            {
                auto ids = valueOr("blacklistIds", "");
                std::replace(ids.begin(), ids.end(), ',', '|');
                std::replace(ids.begin(), ids.end(), ';', '|');
                ids.erase(std::remove(ids.begin(), ids.end(), ' '), ids.end());
                if (ids.find_first_not_of("0123456789|") != std::string::npos)
                    return std::nullopt;
                out.fields_[40] = std::move(ids);
            }
        }
        if (supplied("extendedEpgInfo"))
        {
            // Public v1 uses a CSV list of provider category entries.
            // epgsearch uses id#value separated by |, with escaped delimiters.
            const auto list = valueOr("extendedEpgInfo", "");
            std::string encoded;
            std::size_t begin = 0U;
            while (begin < list.size())
            {
                const auto end = list.find(',', begin);
                const auto entry = list.substr(begin, end == std::string::npos ? end : end - begin);
                const auto hash = entry.find('#');
                if (hash == std::string::npos || hash == 0U ||
                    entry.substr(0, hash).find_first_not_of("0123456789") != std::string::npos ||
                    entry.find_first_of("\r\n") != std::string::npos ||
                    entry.find('\0') != std::string::npos ||
                    entry.find("!^colon^!") != std::string::npos ||
                    entry.find("!^pipe^!") != std::string::npos)
                    return std::nullopt;
                if (!encoded.empty()) encoded += '|';
                for (char c : entry)
                {
                    if (c == ':') encoded += "!^colon^!";
                    else if (c == '|') encoded += "!^pipe^!";
                    else encoded += c;
                }
                if (end == std::string::npos) break;
                begin = end + 1U;
            }
            out.fields_[27] = std::move(encoded);
        }
        if ((supplied("useExtendedEpgInfo") || supplied("extendedEpgInfo")) &&
            out.fields_[26] == "0")
            out.fields_[27].clear();
        for (const auto& [key, index] : {std::make_pair("activeFrom", 47U),
                 std::make_pair("activeUntil", 48U), std::make_pair("repeatCompareCategoriesMask", 33U)})
        {
            if (!supplied(key)) continue;
            auto value = valueOr(key, "");
            if (value.empty()) value = "0";
            if (value.find_first_not_of("0123456789") != std::string::npos) return std::nullopt;
            const auto first = value.find_first_not_of('0');
            out.fields_[index] = first == std::string::npos ? "0" : value.substr(first);
        }
        if (supplied("contentDescriptors"))
        {
            const auto value = valueOr("contentDescriptors", "");
            if (value.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos ||
                value.size() % 2U != 0U) return std::nullopt;
            out.fields_[52] = value;
        }
        if ((supplied("compareTitle") || supplied("compareSubtitle") ||
             supplied("compareSummary")) &&
            out.fields_[9] != "1" && out.fields_[10] != "1" && out.fields_[11] != "1")
            return std::nullopt; // same effective requirement as CREATE
        // UPDATE uses the existing modify authorization, not execute.
        // Activation/deactivation remain the separately authorized actions.
        if (supplied("activeMode") && out.active() != active()) return std::nullopt;
        if (supplied("activeMode") && out.fields_[15] == "2" &&
            (out.fields_[47].empty() || out.fields_[47] == "0"))
            return std::nullopt; // time-limited activation needs its start
        return out;
    }

    int activeMode() const { return fields_[15][0] - '0'; }
    int repeatCompareSubtitleMode() const { return modeField(31U, 2); }
    int repeatCompareTimeMode() const { return modeField(53U, 3); }
    std::string repeatCompareCategoriesMask() const
    {
        const auto& value = fields_[33];
        return value.empty() || value.find_first_not_of("0123456789") != std::string::npos
            ? std::string{} : value;
    }

    EpgsearchStockRule withActive(bool enabled) const
    {
        EpgsearchStockRule out = *this;
        out.fields_[15] = enabled ? "1" : "0";
        return out;
    }

private:
    int modeField(std::size_t index, int max) const
    {
        const auto& value = fields_[index];
        if (value.size() != 1U || value[0] < '0' || value[0] > '0' + max)
            return -1;
        return value[0] - '0';
    }

    std::vector<std::string> fields_;
};
} // namespace vdrsuite::agent

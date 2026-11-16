#include "SearchTimerCreateOperationPayload.h"

#include <openssl/sha.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr const char* kPrefix = "searchtimer-create-payload/3|";
constexpr const char* kLegacyPrefix = "searchtimer-create-payload/2|";
constexpr std::size_t kMaxText = 240U;

// Reject control bytes, malformed Unicode, overlong sequences and surrogates;
// backend and Public-v1 must receive the same unambiguous UTF-8 bytes.
bool safeUtf8Text(const std::string& input)
{
    if (input.empty() || input.size() > kMaxText ||
        input.front() == ' ' || input.back() == ' ')
        return false;

    for (std::size_t i = 0U; i < input.size();)
    {
        const unsigned char first = static_cast<unsigned char>(input[i]);
        if (first >= 0x20U && first <= 0x7eU)
        {
            ++i;
            continue;
        }
        std::size_t count = 0U;
        if (first >= 0xc2U && first <= 0xdfU) count = 2U;
        else if (first >= 0xe0U && first <= 0xefU) count = 3U;
        else if (first >= 0xf0U && first <= 0xf4U) count = 4U;
        else return false;
        if (i + count > input.size()) return false;
        for (std::size_t j = 1U; j < count; ++j)
        {
            const unsigned char next =
                static_cast<unsigned char>(input[i + j]);
            if (next < 0x80U || next > 0xbfU) return false;
        }
        const unsigned char second =
            static_cast<unsigned char>(input[i + 1U]);
        if ((first == 0xe0U && second < 0xa0U) ||
            (first == 0xedU && second > 0x9fU) ||
            (first == 0xf0U && second < 0x90U) ||
            (first == 0xf4U && second > 0x8fU))
            return false;
        i += count;
    }
    return true;
}

void appendField(std::string& output, const std::string& field)
{
    output += std::to_string(field.size());
    output += ':';
    output += field;
    output += '|';
}

bool readField(const std::string& input,
    std::size_t& offset, std::string& field)
{
    std::size_t length = 0U;
    const std::size_t begin = offset;
    while (offset < input.size() &&
           input[offset] >= '0' && input[offset] <= '9')
    {
        const std::size_t digit = static_cast<std::size_t>(
            input[offset] - '0');
        if (length > (std::numeric_limits<std::size_t>::max() - digit) / 10U)
            return false;
        length = length * 10U + digit;
        ++offset;
    }
    if (offset == begin || offset >= input.size() ||
        input[offset] != ':' || length > input.size() - offset - 1U)
        return false;
    ++offset;
    field = input.substr(offset, length);
    offset += length;
    if (offset >= input.size() || input[offset] != '|') return false;
    ++offset;
    return true;
}

bool parseMargin(const std::string& value, int& minutes)
{
    if (value.empty() || value.size() > 4U) return false;
    int count = 0;
    for (char c : value) {
        if (c < '0' || c > '9') return false;
        count = count * 10 + (c - '0');
        if (count > 1440) return false;
    }
    if (std::to_string(count) != value) return false;
    minutes = count;
    return true;
}

bool parseFlag(const std::string& field, bool& value)
{
    if (field != "0" && field != "1") return false;
    value = field == "1";
    return true;
}

bool optionalProviderText(const std::string& value)
{
    if (value.size() > 4096U) return false;
    std::size_t i = 0U;
    while (i < value.size())
    {
        const unsigned char first = static_cast<unsigned char>(value[i]);
        if (first >= 0x20U && first <= 0x7eU) { ++i; continue; }
        std::size_t count = 0U;
        if (first >= 0xc2U && first <= 0xdfU) count = 2U;
        else if (first >= 0xe0U && first <= 0xefU) count = 3U;
        else if (first >= 0xf0U && first <= 0xf4U) count = 4U;
        else return false;
        if (i + count > value.size()) return false;
        for (std::size_t j = 1U; j < count; ++j)
        {
            const auto next = static_cast<unsigned char>(value[i + j]);
            if (next < 0x80U || next > 0xbfU) return false;
        }
        const auto second = static_cast<unsigned char>(value[i + 1U]);
        if ((first == 0xe0U && second < 0xa0U) ||
            (first == 0xedU && second > 0x9fU) ||
            (first == 0xf0U && second < 0x90U) ||
            (first == 0xf4U && second > 0x8fU)) return false;
        i += count;
    }
    return true;
}

bool parseIntField(const std::string& value, int& out)
{
    if (value.empty() || value.size() > 11U) return false;
    const bool negative = value[0] == '-';
    if (negative && value.size() < 2U) return false;
    std::int64_t number = 0;
    for (std::size_t i = negative ? 1U : 0U; i < value.size(); ++i)
    {
        const char c = value[i];
        if (c < '0' || c > '9') return false;
        number = number * 10 + (c - '0');
        if (number > 2147483648LL) return false;
    }
    if (negative) number = -number;
    if (number < std::numeric_limits<int>::min() ||
        number > std::numeric_limits<int>::max() ||
        std::to_string(number) != value)
        return false;
    out = static_cast<int>(number);
    return true;
}

bool validClock(int clock)
{
    return clock >= 0 && clock <= 2359 && (clock % 100) < 60;
}

std::string serializeLegacyV2(const SearchTimerCreateOperationPayload& payload)
{
    std::string result(kLegacyPrefix);
    appendField(result, payload.name);
    appendField(result, payload.query);
    appendField(result, payload.active ? "1" : "0");
    appendField(result, std::to_string(payload.matchMode));
    appendField(result, std::to_string(payload.matchTolerance));
    appendField(result, payload.compareTitle ? "1" : "0");
    appendField(result, payload.compareSubtitle ? "1" : "0");
    appendField(result, payload.compareSummary ? "1" : "0");
    appendField(result, std::to_string(payload.marginStartMinutes));
    appendField(result, std::to_string(payload.marginStopMinutes));
    return result;
}

}

bool searchTimerCreateOperationPayloadValid(
    const SearchTimerCreateOperationPayload& payload)
{
    return safeUtf8Text(payload.name) &&
        safeUtf8Text(payload.query) && payload.query.size() >= 2U &&
        payload.matchMode >= 0 && payload.matchMode <= 5 &&
        payload.matchTolerance >= 0 && payload.matchTolerance <= 3 &&
        (payload.matchMode == 5 || payload.matchTolerance == 0) &&
        (payload.compareTitle || payload.compareSubtitle ||
            payload.compareSummary) &&
        payload.marginStartMinutes >= 0 && payload.marginStartMinutes <= 1440 &&
        payload.marginStopMinutes >= 0 && payload.marginStopMinutes <= 1440 &&
        payload.useChannel >= 0 && payload.useChannel <= 3 &&
        (!payload.useTime || (validClock(payload.startTime) &&
            validClock(payload.stopTime))) &&
        (!payload.useDuration || payload.durationMaxMinutes == 0 ||
            payload.durationMinMinutes <= payload.durationMaxMinutes) &&
        payload.blacklistIdList.empty() &&
        payload.extendedEpgInfoList.empty() &&
        optionalProviderText(payload.directory) &&
        optionalProviderText(payload.channels) &&
        optionalProviderText(payload.channelMin) &&
        optionalProviderText(payload.channelMax) &&
        optionalProviderText(payload.blacklistIds) &&
        optionalProviderText(payload.extendedEpgInfo) &&
        optionalProviderText(payload.contentDescriptors) &&
        optionalProviderText(payload.activeFrom) &&
        optionalProviderText(payload.activeUntil) &&
        true;
}

std::string serializeSearchTimerCreateOperationPayload(
    const SearchTimerCreateOperationPayload& payload)
{
    if (!searchTimerCreateOperationPayloadValid(payload)) return {};
    std::string output(kPrefix);
    appendField(output, payload.name);
    appendField(output, payload.query);
    appendField(output, payload.active ? "1" : "0");
    appendField(output, std::to_string(payload.matchMode));
    appendField(output, std::to_string(payload.matchTolerance));
    appendField(output, payload.compareTitle ? "1" : "0");
    appendField(output, payload.compareSubtitle ? "1" : "0");
    appendField(output, payload.compareSummary ? "1" : "0");
    appendField(output, std::to_string(payload.marginStartMinutes));
    appendField(output, std::to_string(payload.marginStopMinutes));
    appendField(output, payload.directory);
    appendField(output, std::to_string(payload.priority));
    appendField(output, std::to_string(payload.lifetime));
    appendField(output, payload.useVps ? "1" : "0");
    appendField(output, std::to_string(payload.useChannel));
    appendField(output, payload.channels);
    appendField(output, payload.channelMin);
    appendField(output, payload.channelMax);
    appendField(output, payload.useTime ? "1" : "0");
    appendField(output, std::to_string(payload.startTime));
    appendField(output, std::to_string(payload.stopTime));
    appendField(output, payload.useDuration ? "1" : "0");
    appendField(output, std::to_string(payload.durationMinMinutes));
    appendField(output, std::to_string(payload.durationMaxMinutes));
    appendField(output, payload.useDayOfWeek ? "1" : "0");
    appendField(output, std::to_string(payload.dayOfWeek));
    appendField(output, payload.avoidRepeats ? "1" : "0");
    appendField(output, std::to_string(payload.allowedRepeats));
    appendField(output, std::to_string(payload.repeatsWithinDays));
    appendField(output, payload.compareCategories ? "1" : "0");
    appendField(output, payload.compareTime ? "1" : "0");
    appendField(output, payload.useSeriesRecording ? "1" : "0");
    appendField(output, std::to_string(payload.keepRecordings));
    appendField(output, std::to_string(payload.deleteMode));
    appendField(output, std::to_string(payload.searchTimerAction));
    appendField(output, std::to_string(payload.blacklistMode));
    appendField(output, payload.blacklistIds);
    appendField(output, payload.matchCase ? "1" : "0");
    appendField(output, std::to_string(payload.summaryMatch));
    appendField(output, payload.useExtendedEpgInfo ? "1" : "0");
    appendField(output, payload.extendedEpgInfo);
    appendField(output, payload.ignoreMissingEpgCategories ? "1" : "0");
    appendField(output, payload.contentDescriptors);
    appendField(output, payload.useInFavorites ? "1" : "0");
    appendField(output, payload.activeFrom);
    appendField(output, payload.activeUntil);
    appendField(output, payload.pauseOnRecordings ? "1" : "0");
    appendField(output, std::to_string(payload.switchMinutesBefore));
    appendField(output, payload.unmuteSoundOnSwitch ? "1" : "0");
    appendField(output, std::to_string(payload.deleteRecordingsAfterDays));
    appendField(output, std::to_string(payload.deleteAfterCountRecordings));
    appendField(output, std::to_string(payload.deleteAfterDaysOfFirstRecording));
    return output;
}

bool parseSearchTimerCreateOperationPayload(
    const std::string& serialized,
    SearchTimerCreateOperationPayload& payload)
{
    const bool v3 = serialized.compare(0U, std::string(kPrefix).size(),
        kPrefix) == 0;
    const bool v2 = !v3 &&
        serialized.compare(0U, std::string(kLegacyPrefix).size(),
            kLegacyPrefix) == 0;
    if (!v3 && !v2) return false;
    std::size_t offset = v3 ? std::string(kPrefix).size() :
        std::string(kLegacyPrefix).size();
    std::vector<std::string> fields;
    const std::size_t count = v3 ? 52U : 10U;
    fields.reserve(count);
    for (std::size_t i = 0U; i < count; ++i)
    {
        std::string field;
        if (!readField(serialized, offset, field)) return false;
        fields.push_back(std::move(field));
    }
    if (offset != serialized.size()) return false;
    SearchTimerCreateOperationPayload decoded;
    decoded.name = fields[0];
    decoded.query = fields[1];
    if (!parseFlag(fields[2], decoded.active) ||
        !parseIntField(fields[3], decoded.matchMode) ||
        !parseIntField(fields[4], decoded.matchTolerance) ||
        !parseFlag(fields[5], decoded.compareTitle) ||
        !parseFlag(fields[6], decoded.compareSubtitle) ||
        !parseFlag(fields[7], decoded.compareSummary) ||
        !parseMargin(fields[8], decoded.marginStartMinutes) ||
        !parseMargin(fields[9], decoded.marginStopMinutes))
        return false;
    if (v3)
    {
        decoded.directory = fields[10];
        if (!parseIntField(fields[11], decoded.priority)) return false;
        if (!parseIntField(fields[12], decoded.lifetime)) return false;
        if (!parseFlag(fields[13], decoded.useVps)) return false;
        if (!parseIntField(fields[14], decoded.useChannel)) return false;
        decoded.channels = fields[15];
        decoded.channelMin = fields[16];
        decoded.channelMax = fields[17];
        if (!parseFlag(fields[18], decoded.useTime)) return false;
        if (!parseIntField(fields[19], decoded.startTime)) return false;
        if (!parseIntField(fields[20], decoded.stopTime)) return false;
        if (!parseFlag(fields[21], decoded.useDuration)) return false;
        if (!parseIntField(fields[22], decoded.durationMinMinutes)) return false;
        if (!parseIntField(fields[23], decoded.durationMaxMinutes)) return false;
        if (!parseFlag(fields[24], decoded.useDayOfWeek)) return false;
        if (!parseIntField(fields[25], decoded.dayOfWeek)) return false;
        if (!parseFlag(fields[26], decoded.avoidRepeats)) return false;
        if (!parseIntField(fields[27], decoded.allowedRepeats)) return false;
        if (!parseIntField(fields[28], decoded.repeatsWithinDays)) return false;
        if (!parseFlag(fields[29], decoded.compareCategories)) return false;
        if (!parseFlag(fields[30], decoded.compareTime)) return false;
        if (!parseFlag(fields[31], decoded.useSeriesRecording)) return false;
        if (!parseIntField(fields[32], decoded.keepRecordings)) return false;
        if (!parseIntField(fields[33], decoded.deleteMode)) return false;
        if (!parseIntField(fields[34], decoded.searchTimerAction)) return false;
        if (!parseIntField(fields[35], decoded.blacklistMode)) return false;
        decoded.blacklistIds = fields[36];
        if (!parseFlag(fields[37], decoded.matchCase)) return false;
        if (!parseIntField(fields[38], decoded.summaryMatch)) return false;
        if (!parseFlag(fields[39], decoded.useExtendedEpgInfo)) return false;
        decoded.extendedEpgInfo = fields[40];
        if (!parseFlag(fields[41], decoded.ignoreMissingEpgCategories)) return false;
        decoded.contentDescriptors = fields[42];
        if (!parseFlag(fields[43], decoded.useInFavorites)) return false;
        decoded.activeFrom = fields[44];
        decoded.activeUntil = fields[45];
        if (!parseFlag(fields[46], decoded.pauseOnRecordings)) return false;
        if (!parseIntField(fields[47], decoded.switchMinutesBefore)) return false;
        if (!parseFlag(fields[48], decoded.unmuteSoundOnSwitch)) return false;
        if (!parseIntField(fields[49], decoded.deleteRecordingsAfterDays)) return false;
        if (!parseIntField(fields[50], decoded.deleteAfterCountRecordings)) return false;
        if (!parseIntField(fields[51], decoded.deleteAfterDaysOfFirstRecording)) return false;
    }
    if (!searchTimerCreateOperationPayloadValid(decoded) ||
        (v3 ? serializeSearchTimerCreateOperationPayload(decoded) :
            serializeLegacyV2(decoded)) != serialized)
        return false;
    payload = std::move(decoded);
    return true;
}

std::string searchTimerCreateOperationPayloadFingerprint(
    const SearchTimerCreateOperationPayload& payload)
{
    const std::string serialized =
        serializeSearchTimerCreateOperationPayload(payload);
    if (serialized.empty()) return {};
    std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
    if (SHA256(reinterpret_cast<const unsigned char*>(serialized.data()),
            serialized.size(), digest.data()) == nullptr)
        return {};
    constexpr char hex[] = "0123456789abcdef";
    std::string fingerprint = "sha256:";
    for (unsigned char byte : digest)
    {
        fingerprint.push_back(hex[byte >> 4U]);
        fingerprint.push_back(hex[byte & 0x0fU]);
    }
    return fingerprint;
}

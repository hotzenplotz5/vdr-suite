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
constexpr const char* kPrefix = "searchtimer-create-payload/2|";
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

bool allowedMode(int mode)
{
    return mode == 0 || mode == 1 || mode == 2 ||
        mode == 3 || mode == 5;
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
}

bool searchTimerCreateOperationPayloadValid(
    const SearchTimerCreateOperationPayload& payload)
{
    return safeUtf8Text(payload.name) &&
        safeUtf8Text(payload.query) && payload.query.size() >= 2U &&
        allowedMode(payload.matchMode) &&
        payload.matchTolerance >= 0 && payload.matchTolerance <= 3 &&
        (payload.matchMode == 5 || payload.matchTolerance == 0) &&
        (payload.compareTitle || payload.compareSubtitle ||
            payload.compareSummary) &&
        payload.marginStartMinutes >= 0 && payload.marginStartMinutes <= 1440 &&
        payload.marginStopMinutes >= 0 && payload.marginStopMinutes <= 1440;
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
    return output;
}

bool parseSearchTimerCreateOperationPayload(
    const std::string& serialized,
    SearchTimerCreateOperationPayload& payload)
{
    const std::string prefix(kPrefix);
    if (serialized.size() <= prefix.size() ||
        serialized.compare(0U, prefix.size(), prefix) != 0)
        return false;

    std::size_t offset = prefix.size();
    std::vector<std::string> fields;
    fields.reserve(10U);
    for (std::size_t i = 0U; i < 10U; ++i)
    {
        std::string field;
        if (!readField(serialized, offset, field)) return false;
        fields.push_back(std::move(field));
    }
    if (offset != serialized.size()) return false;
    SearchTimerCreateOperationPayload decoded;
    decoded.name = fields[0];
    decoded.query = fields[1];
    if (!parseFlag(fields[2], decoded.active)) return false;
    if (fields[3].size() != 1U || fields[3][0] < '0' ||
        fields[3][0] > '9' ||
        fields[4].size() != 1U || fields[4][0] < '0' ||
        fields[4][0] > '9') return false;
    decoded.matchMode = fields[3][0] - '0';
    decoded.matchTolerance = fields[4][0] - '0';
    if (!parseFlag(fields[5], decoded.compareTitle) ||
        !parseFlag(fields[6], decoded.compareSubtitle) ||
        !parseFlag(fields[7], decoded.compareSummary) ||
        !parseMargin(fields[8], decoded.marginStartMinutes) ||
        !parseMargin(fields[9], decoded.marginStopMinutes))
        return false;
    if (serializeSearchTimerCreateOperationPayload(decoded) != serialized)
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

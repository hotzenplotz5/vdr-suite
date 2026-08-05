#include "BackendAgentChannelObservation.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace
{
constexpr std::size_t MaximumChannelsConfBytes = 1024U * 1024U;
constexpr std::size_t MaximumChannels = 4096U;
constexpr std::size_t MaximumLineBytes = 4096U;
constexpr std::size_t MaximumChannelIdBytes = 128U;
constexpr std::size_t MaximumNameBytes = 256U;
constexpr std::size_t MaximumProviderBytes = 256U;
constexpr std::size_t MaximumGroupNameBytes = 256U;

std::string trim(std::string value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
        value.erase(value.begin());
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
        value.pop_back();
    return value;
}

std::string decodeVdrText(std::string value)
{
    std::replace(value.begin(), value.end(), '|', ':');
    return value;
}

bool validUtf8(const std::string& value)
{
    std::size_t index = 0;
    while (index < value.size())
    {
        const unsigned char first = static_cast<unsigned char>(value[index]);
        if (first <= 0x7fU)
        {
            ++index;
            continue;
        }
        std::size_t count = 0;
        std::uint32_t codePoint = 0;
        if ((first & 0xe0U) == 0xc0U) { count = 2; codePoint = first & 0x1fU; }
        else if ((first & 0xf0U) == 0xe0U) { count = 3; codePoint = first & 0x0fU; }
        else if ((first & 0xf8U) == 0xf0U) { count = 4; codePoint = first & 0x07U; }
        else return false;
        if (index + count > value.size()) return false;
        for (std::size_t offset = 1; offset < count; ++offset)
        {
            const unsigned char next = static_cast<unsigned char>(value[index + offset]);
            if ((next & 0xc0U) != 0x80U) return false;
            codePoint = (codePoint << 6U) | (next & 0x3fU);
        }
        if ((count == 2 && codePoint < 0x80U) ||
            (count == 3 && codePoint < 0x800U) ||
            (count == 4 && codePoint < 0x10000U) ||
            codePoint > 0x10ffffU ||
            (codePoint >= 0xd800U && codePoint <= 0xdfffU))
        {
            return false;
        }
        index += count;
    }
    return true;
}

bool containsControl(const std::string& value)
{
    return std::any_of(value.begin(), value.end(), [](unsigned char character) {
        return character < 0x20U || character == 0x7fU;
    });
}

bool safeChannelId(const std::string& value)
{
    if (value.empty() || value.size() > MaximumChannelIdBytes) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) || character == '-' || character == '_' ||
            character == '.' || character == ':';
    });
}

bool parseUnsignedDecimal(const std::string& value, std::uint64_t& parsed)
{
    if (value.empty() || value.size() > 20 ||
        !std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isdigit(character) != 0;
        }))
    {
        return false;
    }
    errno = 0;
    char* end = nullptr;
    const unsigned long long candidate = std::strtoull(value.c_str(), &end, 10);
    if (errno != 0 || end == nullptr || *end != '\0') return false;
    parsed = static_cast<std::uint64_t>(candidate);
    return parsed <= static_cast<std::uint64_t>(
        std::numeric_limits<std::int64_t>::max());
}

bool parseHexListEncrypted(const std::string& value, bool& encrypted)
{
    encrypted = false;
    if (value.empty() || value.size() > 256) return false;
    std::size_t start = 0;
    while (start <= value.size())
    {
        const std::size_t comma = value.find(',', start);
        const std::string item = value.substr(
            start, comma == std::string::npos ? std::string::npos : comma - start);
        if (item.empty() || item.size() > 4 ||
            !std::all_of(item.begin(), item.end(), [](unsigned char character) {
                return std::isxdigit(character) != 0;
            }))
        {
            return false;
        }
        errno = 0;
        char* end = nullptr;
        const unsigned long candidate = std::strtoul(item.c_str(), &end, 16);
        if (errno != 0 || end == nullptr || *end != '\0' || candidate > 0xffffUL)
            return false;
        if (candidate != 0) encrypted = true;
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return true;
}

std::vector<std::string> splitFields(const std::string& line)
{
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (start <= line.size())
    {
        const std::size_t separator = line.find(':', start);
        fields.push_back(line.substr(
            start,
            separator == std::string::npos ? std::string::npos : separator - start));
        if (separator == std::string::npos) break;
        start = separator + 1;
    }
    return fields;
}

std::string jsonEscape(const std::string& value)
{
    std::ostringstream output;
    for (unsigned char character : value)
    {
        switch (character)
        {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\b': output << "\\b"; break;
            case '\f': output << "\\f"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (character >= 0x20U) output << static_cast<char>(character);
        }
    }
    return output.str();
}

std::string canonicalFact(const BackendAgentChannelFact& fact)
{
    std::ostringstream output;
    output << "{\"channelId\":\"" << jsonEscape(fact.channelId)
           << "\",\"channelNumber\":" << fact.channelNumber
           << ",\"name\":\"" << jsonEscape(fact.name)
           << "\",\"provider\":\"" << jsonEscape(fact.provider)
           << "\",\"groupName\":\"" << jsonEscape(fact.groupName)
           << "\",\"radio\":" << (fact.radio ? "true" : "false")
           << ",\"encrypted\":" << (fact.encrypted ? "true" : "false")
           << ",\"enabled\":" << (fact.enabled ? "true" : "false") << '}';
    return output.str();
}

void sortFacts(std::vector<BackendAgentChannelFact>& facts)
{
    std::sort(facts.begin(), facts.end(), [](const auto& left, const auto& right) {
        return left.channelId < right.channelId;
    });
}

std::string channelIdFor(
    const std::string& source,
    const std::string& parameters,
    std::uint64_t frequency,
    std::uint64_t sid,
    std::uint64_t nid,
    std::uint64_t tid,
    std::uint64_t rid,
    std::string& reasonCode)
{
    std::uint64_t effectiveTid = tid;
    if (nid == 0 && tid == 0)
    {
        effectiveTid = frequency;
        if (!source.empty() && source.front() == 'S')
        {
            std::uint64_t offset = 0;
            if (parameters.find('H') != std::string::npos) offset = 100000;
            else if (parameters.find('V') != std::string::npos) offset = 200000;
            else if (parameters.find('L') != std::string::npos) offset = 300000;
            else if (parameters.find('R') != std::string::npos) offset = 400000;
            else
            {
                reasonCode = "channels_conf_satellite_polarization_missing";
                return {};
            }
            if (effectiveTid > static_cast<std::uint64_t>(
                    std::numeric_limits<std::int64_t>::max()) - offset)
            {
                reasonCode = "channels_conf_channel_id_overflow";
                return {};
            }
            effectiveTid += offset;
        }
    }
    std::ostringstream id;
    id << source << '-' << nid << '-' << effectiveTid << '-' << sid;
    if (rid != 0) id << '-' << rid;
    if (!safeChannelId(id.str()))
    {
        reasonCode = "channels_conf_invalid_channel_id";
        return {};
    }
    return id.str();
}

bool parseNameField(
    const std::string& encoded,
    std::string& name,
    std::string& provider)
{
    std::string withoutProvider = encoded;
    const std::size_t providerSeparator = withoutProvider.rfind(';');
    if (providerSeparator != std::string::npos)
    {
        provider = decodeVdrText(withoutProvider.substr(providerSeparator + 1));
        withoutProvider.erase(providerSeparator);
    }
    else provider.clear();
    const std::size_t shortNameSeparator = withoutProvider.rfind(',');
    if (shortNameSeparator != std::string::npos)
        withoutProvider.erase(shortNameSeparator);
    name = decodeVdrText(withoutProvider);
    return !name.empty() && name.size() <= MaximumNameBytes &&
        provider.size() <= MaximumProviderBytes &&
        !containsControl(name) && !containsControl(provider) &&
        validUtf8(name) && validUtf8(provider);
}

bool parseVideoPidRadio(const std::string& value, bool& radio)
{
    const std::size_t delimiter = value.find_first_of("+=");
    const std::string primary = value.substr(0, delimiter);
    std::uint64_t pid = 0;
    if (!parseUnsignedDecimal(primary, pid) || pid > 0x1fffU) return false;
    radio = pid == 0;
    return true;
}
}

bool backendAgentValidChannelFact(
    const BackendAgentChannelFact& fact,
    std::string& reasonCode)
{
    if (!safeChannelId(fact.channelId))
        reasonCode = "invalid_channel_id";
    else if (fact.channelNumber == 0 ||
             fact.channelNumber > static_cast<std::uint64_t>(
                 std::numeric_limits<std::int64_t>::max()))
        reasonCode = "invalid_channel_number";
    else if (fact.name.empty() || fact.name.size() > MaximumNameBytes ||
             containsControl(fact.name))
        reasonCode = "invalid_channel_name";
    else if (fact.provider.size() > MaximumProviderBytes || containsControl(fact.provider))
        reasonCode = "invalid_channel_provider";
    else if (fact.groupName.size() > MaximumGroupNameBytes || containsControl(fact.groupName))
        reasonCode = "invalid_channel_group";
    else
    {
        reasonCode = "channel_fact_valid";
        return true;
    }
    return false;
}

std::string backendAgentCanonicalChannelPayload(
    const std::string& kind,
    const std::vector<BackendAgentChannelFact>& channels,
    const std::vector<BackendAgentChannelFact>& upserts,
    const std::vector<std::string>& removedChannelIds)
{
    std::ostringstream output;
    if (kind == "completeSnapshot")
    {
        std::vector<BackendAgentChannelFact> sorted = channels;
        sortFacts(sorted);
        output << "{\"channels\":[";
        for (std::size_t index = 0; index < sorted.size(); ++index)
        {
            if (index != 0) output << ',';
            output << canonicalFact(sorted[index]);
        }
        output << "]}";
        return output.str();
    }
    if (kind == "changeBatch")
    {
        std::vector<BackendAgentChannelFact> sortedUpserts = upserts;
        std::vector<std::string> sortedRemoved = removedChannelIds;
        sortFacts(sortedUpserts);
        std::sort(sortedRemoved.begin(), sortedRemoved.end());
        output << "{\"removedChannelIds\":[";
        for (std::size_t index = 0; index < sortedRemoved.size(); ++index)
        {
            if (index != 0) output << ',';
            output << '"' << jsonEscape(sortedRemoved[index]) << '"';
        }
        output << "],\"upserts\":[";
        for (std::size_t index = 0; index < sortedUpserts.size(); ++index)
        {
            if (index != 0) output << ',';
            output << canonicalFact(sortedUpserts[index]);
        }
        output << "]}";
        return output.str();
    }
    return {};
}

std::string backendAgentChannelPayloadIdentity(const std::string& canonicalPayload)
{
    std::uint64_t hash = 1469598103934665603ULL;
    for (unsigned char character : canonicalPayload)
    {
        hash ^= character;
        hash *= 1099511628211ULL;
    }
    std::ostringstream output;
    output << "fnv1a64-" << std::hex << std::setfill('0') << std::setw(16) << hash;
    return output.str();
}

bool readBackendAgentChannelsConfSnapshot(
    const std::string& path,
    BackendAgentChannelSnapshot& snapshot,
    std::string& reasonCode)
{
    snapshot = BackendAgentChannelSnapshot{};
    if (path.empty() || path.front() != '/' || path.size() > 4096)
    {
        reasonCode = "invalid_channels_conf_path";
        return false;
    }
    struct stat status{};
    if (lstat(path.c_str(), &status) != 0)
    {
        reasonCode = "channels_conf_unavailable";
        return false;
    }
    if (!S_ISREG(status.st_mode) || status.st_size < 0 ||
        static_cast<std::size_t>(status.st_size) > MaximumChannelsConfBytes)
    {
        reasonCode = "invalid_channels_conf_file";
        return false;
    }
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        reasonCode = "channels_conf_unavailable";
        return false;
    }
    std::string content(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());
    if (input.bad() || content.size() > MaximumChannelsConfBytes ||
        content.find('\0') != std::string::npos)
    {
        reasonCode = "invalid_channels_conf_file";
        return false;
    }

    std::string groupName;
    std::uint64_t nextChannelNumber = 1;
    std::set<std::string> identities;
    std::istringstream lines(content);
    std::string line;
    while (std::getline(lines, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() > MaximumLineBytes)
        {
            reasonCode = "channels_conf_line_too_large";
            return false;
        }
        if (line.empty() || line.front() == '#') continue;
        if (line.front() == ':')
        {
            std::size_t position = 1;
            if (position < line.size() && line[position] == '@')
            {
                ++position;
                const std::size_t start = position;
                while (position < line.size() &&
                       std::isdigit(static_cast<unsigned char>(line[position]))) ++position;
                std::uint64_t requested = 0;
                if (start == position ||
                    !parseUnsignedDecimal(line.substr(start, position - start), requested) ||
                    requested == 0)
                {
                    reasonCode = "channels_conf_invalid_group_number";
                    return false;
                }
                if (requested > nextChannelNumber) nextChannelNumber = requested;
            }
            groupName = decodeVdrText(trim(line.substr(position)));
            if (groupName.size() > MaximumGroupNameBytes || containsControl(groupName) ||
                !validUtf8(groupName))
            {
                reasonCode = "channels_conf_invalid_group_name";
                return false;
            }
            continue;
        }

        const std::vector<std::string> fields = splitFields(line);
        if (fields.size() != 13)
        {
            reasonCode = "channels_conf_invalid_field_count";
            return false;
        }
        std::uint64_t frequency = 0;
        std::uint64_t sid = 0;
        std::uint64_t nid = 0;
        std::uint64_t tid = 0;
        std::uint64_t rid = 0;
        if (!parseUnsignedDecimal(fields[1], frequency) ||
            !parseUnsignedDecimal(fields[9], sid) ||
            !parseUnsignedDecimal(fields[10], nid) ||
            !parseUnsignedDecimal(fields[11], tid) ||
            !parseUnsignedDecimal(fields[12], rid) ||
            fields[3].empty() || fields[3].size() > 64 ||
            containsControl(fields[2]) || containsControl(fields[3]))
        {
            reasonCode = "channels_conf_invalid_native_fields";
            return false;
        }

        BackendAgentChannelFact fact;
        if (!parseNameField(fields[0], fact.name, fact.provider) ||
            !parseVideoPidRadio(fields[5], fact.radio) ||
            !parseHexListEncrypted(fields[8], fact.encrypted))
        {
            reasonCode = "channels_conf_invalid_channel_fact";
            return false;
        }
        fact.channelId = channelIdFor(
            fields[3], fields[2], frequency, sid, nid, tid, rid, reasonCode);
        if (fact.channelId.empty()) return false;
        fact.channelNumber = nextChannelNumber++;
        fact.groupName = groupName;
        fact.enabled = true;
        std::string validationReason;
        if (!backendAgentValidChannelFact(fact, validationReason))
        {
            reasonCode = "channels_conf_" + validationReason;
            return false;
        }
        if (!identities.insert(fact.channelId).second)
        {
            reasonCode = "channels_conf_duplicate_channel_id";
            return false;
        }
        snapshot.channels.push_back(std::move(fact));
        if (snapshot.channels.size() > MaximumChannels)
        {
            reasonCode = "channels_conf_too_many_channels";
            return false;
        }
    }
    snapshot.canonicalPayload = backendAgentCanonicalChannelPayload(
        "completeSnapshot", snapshot.channels, {}, {});
    snapshot.resourceRevision = backendAgentChannelPayloadIdentity(
        snapshot.canonicalPayload);
    reasonCode = "channels_conf_snapshot_loaded";
    return true;
}

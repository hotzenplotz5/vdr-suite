#include "SearchTimerDiscoveryJsonSerializer.h"

#include <sstream>

namespace
{
void appendQuoted(
    std::ostringstream& json,
    const std::string& value)
{
    json << '"';

    for (const char character : value)
    {
        switch (character)
        {
        case '"':
            json << "\\\"";
            break;
        case '\\':
            json << "\\\\";
            break;
        case '\n':
            json << "\\n";
            break;
        case '\r':
            json << "\\r";
            break;
        case '\t':
            json << "\\t";
            break;
        default:
            if (static_cast<unsigned char>(character) < 0x20)
            {
                json << ' ';
            }
            else
            {
                json << character;
            }
            break;
        }
    }

    json << '"';
}

void appendStringArray(
    std::ostringstream& json,
    const std::vector<std::string>& values)
{
    json << '[';

    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        appendQuoted(json, values[index]);
    }

    json << ']';
}

void appendExtendedEpgInfo(
    std::ostringstream& json,
    const SearchTimerDiscoveryExtendedEpgInfo& info)
{
    json << '{';
    json << "\"id\":" << info.id();
    json << ",\"name\":";
    appendQuoted(json, info.name());
    json << ",\"values\":";
    appendStringArray(json, info.values());
    json << ",\"config\":";
    appendQuoted(json, info.config());
    json << '}';
}

void appendChannelGroup(
    std::ostringstream& json,
    const SearchTimerDiscoveryChannelGroup& group)
{
    json << '{';
    json << "\"name\":";
    appendQuoted(json, group.name());
    json << '}';
}

void appendBlacklist(
    std::ostringstream& json,
    const SearchTimerDiscoveryBlacklist& blacklist)
{
    json << '{';
    json << "\"id\":" << blacklist.id();
    json << ",\"search\":";
    appendQuoted(json, blacklist.search());
    json << '}';
}

void appendRecordingDirectory(
    std::ostringstream& json,
    const SearchTimerDiscoveryRecordingDirectory& directory)
{
    json << '{';
    json << "\"path\":";
    appendQuoted(json, directory.path());
    json << '}';
}

template <typename Item, typename Append>
void appendArray(
    std::ostringstream& json,
    const std::vector<Item>& items,
    Append append)
{
    json << '[';

    for (std::size_t index = 0; index < items.size(); ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        append(json, items[index]);
    }

    json << ']';
}
}

std::string SearchTimerDiscoveryJsonSerializer::serialize(
    const SearchTimerDiscoveryCatalog& catalog) const
{
    std::ostringstream json;

    json << '{';

    json << "\"backendId\":";
    appendQuoted(json, catalog.backendId());

    json << ",\"counts\":{";
    json << "\"extendedEpgInfo\":" << catalog.extendedEpgInfoCount();
    json << ",\"channelGroups\":" << catalog.channelGroupCount();
    json << ",\"blacklists\":" << catalog.blacklistCount();
    json << ",\"recordingDirectories\":" << catalog.recordingDirectoryCount();
    json << '}';

    json << ",\"extendedEpgInfo\":";
    appendArray(
        json,
        catalog.extendedEpgInfos(),
        appendExtendedEpgInfo);

    json << ",\"channelGroups\":";
    appendArray(
        json,
        catalog.channelGroups(),
        appendChannelGroup);

    json << ",\"blacklists\":";
    appendArray(
        json,
        catalog.blacklists(),
        appendBlacklist);

    json << ",\"recordingDirectories\":";
    appendArray(
        json,
        catalog.recordingDirectories(),
        appendRecordingDirectory);

    json << '}';

    return json.str();
}

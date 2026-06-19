#include "EpgSearchResultJsonSerializer.h"

#include <sstream>
#include <vector>

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

void appendKey(
    std::ostringstream& json,
    const std::string& key)
{
    appendQuoted(
        json,
        key);
    json << ':';
}

void appendStringArray(
    std::ostringstream& json,
    const std::vector<std::string>& values)
{
    json << '[';

    for (std::size_t index = 0;
         index < values.size();
         ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        appendQuoted(
            json,
            values.at(index));
    }

    json << ']';
}
}

std::string EpgSearchResultJsonSerializer::serialize(
    const EpgSearchResult& result) const
{
    std::ostringstream json;

    json << '{';

    appendKey(json, "totalCount");
    json << result.totalCount();
    json << ',';

    appendKey(json, "returnedCount");
    json << result.returnedCount();
    json << ',';

    appendKey(json, "limit");
    json << result.limit();
    json << ',';

    appendKey(json, "offset");
    json << result.offset();
    json << ',';

    appendKey(json, "results");
    json << '[';

    for (std::size_t index = 0;
         index < result.matches().size();
         ++index)
    {
        const auto& match =
            result.matches().at(index);
        const auto& event =
            match.event();

        if (index > 0)
        {
            json << ',';
        }

        json << '{';

        appendKey(json, "eventId");
        appendQuoted(json, event.id);
        json << ',';

        appendKey(json, "backendId");
        appendQuoted(json, match.backendId());
        json << ',';

        appendKey(json, "channelId");
        appendQuoted(json, event.channelId);
        json << ',';

        appendKey(json, "title");
        appendQuoted(json, event.title);
        json << ',';

        appendKey(json, "subtitle");
        appendQuoted(json, event.subtitle);
        json << ',';

        appendKey(json, "description");
        appendQuoted(json, event.description);
        json << ',';

        appendKey(json, "startTime");
        appendQuoted(json, event.startTime);
        json << ',';

        appendKey(json, "endTime");
        appendQuoted(json, event.endTime);
        json << ',';

        appendKey(json, "durationSeconds");
        json << event.durationSeconds;
        json << ',';

        appendKey(json, "matchedFields");
        appendStringArray(
            json,
            match.matchedFields());

        json << '}';
    }

    json << ']';
    json << '}';

    return json.str();
}

#include "SnapshotChangeFeedJsonSerializer.h"

#include <sstream>

std::string SnapshotChangeFeedJsonSerializer::serializeEntry(
    const SnapshotChangeFeedEntry& entry) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"sequenceNumber\":" << entry.sequenceNumber() << ","
        << "\"snapshotGeneration\":" << entry.snapshotGeneration() << ","
        << "\"changedDomains\":[";

    for (std::size_t index = 0;
         index < entry.changedDomains().size();
         ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        json
            << "\""
            << entry.changedDomains()[index]
            << "\"";
    }

    json << "]}";

    return json.str();
}

std::string SnapshotChangeFeedJsonSerializer::serializeFeed(
    const SnapshotChangeFeed& feed) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"latestSequenceNumber\":" << feed.latestSequenceNumber() << ","
        << "\"latestSnapshotGeneration\":" << feed.latestSnapshotGeneration() << ","
        << "\"entries\":[";

    for (std::size_t index = 0;
         index < feed.entries().size();
         ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        json << serializeEntry(feed.entries()[index]);
    }

    json << "]}";

    return json.str();
}

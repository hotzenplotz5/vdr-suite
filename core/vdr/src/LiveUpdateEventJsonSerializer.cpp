#include "LiveUpdateEventJsonSerializer.h"

#include <sstream>

std::string LiveUpdateEventJsonSerializer::serializeEvent(
    const LiveUpdateEvent& event) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"sequenceNumber\":" << event.sequenceNumber() << ","
        << "\"snapshotGeneration\":" << event.snapshotGeneration() << ","
        << "\"backendId\":\"" << event.backendId() << "\","
        << "\"changedDomains\":[";

    for (std::size_t index = 0;
         index < event.changedDomains().size();
         ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        json
            << "\""
            << event.changedDomains()[index]
            << "\"";
    }

    json << "]}";

    return json.str();
}

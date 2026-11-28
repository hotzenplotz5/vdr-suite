#include "LiveUpdateEventJsonSerializer.h"

#include <sstream>

std::string LiveUpdateEventJsonSerializer::serializeEvent(
    const LiveUpdateEvent& event,
    const std::vector<std::string>& recordingPublicIds) const
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

    json << "]";
    if (!event.recordingNativeIds().empty())
    {
        // Native Recording identities are arbitrary strings and must be
        // escaped before serializing them in an authenticated SSE frame.
        static const char hex[] = "0123456789abcdef";
        json << ",\"recordingNativeIds\":[";
        for (std::size_t index = 0; index < event.recordingNativeIds().size(); ++index)
        {
            if (index != 0) json << ",";
            json << '"';
            for (unsigned char character : event.recordingNativeIds()[index])
            {
                if (character == '"' || character == '\\')
                    json << '\\' << static_cast<char>(character);
                else if (character < 0x20)
                    json << "\\u00" << hex[character >> 4] << hex[character & 15];
                else
                    json << static_cast<char>(character);
            }
            json << '"';
        }
        json << "]";
    }
    // Only validated opaque rec_ IDs from the browser transport; never
    // expose a partial mapping as a complete targeted change.
    if (!recordingPublicIds.empty())
    {
        json << ",\"recordingPublicIds\":[";
        for (std::size_t index = 0; index < recordingPublicIds.size(); ++index)
        {
            if (index != 0) json << ",";
            json << '"' << recordingPublicIds[index] << '"';
        }
        json << "]";
    }
    json << "}";

    return json.str();
}

#include "LiveOverlayController.h"

#include <sstream>

namespace
{
std::string escapeJsonOverlay(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);

    for (const unsigned char character : value)
    {
        switch (character)
        {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (character < 0x20)
                {
                    static const char* digits = "0123456789abcdef";
                    escaped += "\\u00";
                    escaped += digits[(character >> 4) & 0x0f];
                    escaped += digits[character & 0x0f];
                }
                else
                {
                    escaped += static_cast<char>(character);
                }
        }
    }

    return escaped;
}

void appendEvent(
    std::ostringstream& json,
    const LiveOverlayEvent& event)
{
    json << "{\"available\":" << (event.available ? "true" : "false")
         << ",\"eventId\":";

    if (event.available)
    {
        json << "\"" << escapeJsonOverlay(event.eventId) << "\"";
    }
    else
    {
        json << "null";
    }

    json << ",\"title\":";

    if (event.available)
    {
        json << "\"" << escapeJsonOverlay(event.title) << "\"";
    }
    else
    {
        json << "null";
    }

    json << ",\"subtitle\":";

    if (event.available)
    {
        json << "\"" << escapeJsonOverlay(event.subtitle) << "\"";
    }
    else
    {
        json << "null";
    }

    json << ",\"startTime\":";

    if (event.available && event.startTime > 0)
    {
        json << event.startTime;
    }
    else
    {
        json << "null";
    }

    json << ",\"endTime\":";

    if (event.available && event.endTime > 0)
    {
        json << event.endTime;
    }
    else
    {
        json << "null";
    }

    json << '}';
}
}

std::string LiveOverlaySnapshotJsonSerializer::serialize(
    const LiveOverlaySnapshot& snapshot) const
{
    std::ostringstream json;
    json << "{\"success\":" << (snapshot.success ? "true" : "false")
         << ",\"backendId\":\"" << escapeJsonOverlay(snapshot.backendId) << "\""
         << ",\"revision\":" << snapshot.revision
         << ",\"generatedAt\":" << snapshot.generatedAt
         << ",\"message\":\"" << escapeJsonOverlay(snapshot.message) << "\""
         << ",\"channel\":{\"available\":" << (snapshot.channel.available ? "true" : "false")
         << ",\"id\":";

    if (snapshot.channel.available)
    {
        json << "\"" << escapeJsonOverlay(snapshot.channel.id) << "\"";
    }
    else
    {
        json << "null";
    }

    json << ",\"number\":";

    if (snapshot.channel.available)
    {
        json << snapshot.channel.number;
    }
    else
    {
        json << "null";
    }

    json << ",\"name\":";

    if (snapshot.channel.available)
    {
        json << "\"" << escapeJsonOverlay(snapshot.channel.name) << "\"";
    }
    else
    {
        json << "null";
    }

    json << "},\"present\":";
    appendEvent(json, snapshot.present);
    json << ",\"following\":";
    appendEvent(json, snapshot.following);
    json << ",\"timer\":{\"active\":" << (snapshot.timer.active ? "true" : "false")
         << ",\"recording\":" << (snapshot.timer.recording ? "true" : "false")
         << "},\"audio\":{\"available\":" << (snapshot.audio.available ? "true" : "false")
         << ",\"muted\":null,\"volume\":null}}";
    return json.str();
}

LiveOverlayController::LiveOverlayController(
    const LiveOverlayService& service,
    const LiveOverlaySnapshotJsonSerializer& serializer)
    : service_(service),
      serializer_(serializer)
{
}

ApiResponse LiveOverlayController::getSnapshot(
    const std::string& backendId) const
{
    const LiveOverlaySnapshot snapshot = service_.getSnapshot(backendId);
    ApiResponse response;
    response.statusCode = snapshot.statusCode;
    response.contentType = "application/json";
    response.body = serializer_.serialize(snapshot);
    return response;
}

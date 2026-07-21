#include "PersonContextController.h"

#include "PersonContextJsonSerializer.h"
#include "PersonContextService.h"

#include <iomanip>
#include <sstream>

namespace
{

std::string escapeJson(const std::string& value)
{
    std::ostringstream escaped;
    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '"': escaped << "\\\""; break;
        case '\\': escaped << "\\\\"; break;
        case '\n': escaped << "\\n"; break;
        case '\r': escaped << "\\r"; break;
        case '\t': escaped << "\\t"; break;
        default:
            if (character < 0x20)
            {
                escaped << "\\u"
                        << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(character)
                        << std::dec << std::setfill(' ');
            }
            else
            {
                escaped << static_cast<char>(character);
            }
            break;
        }
    }
    return escaped.str();
}

ApiResponse jsonError(int statusCode, const std::string& message)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.body = "{\"error\":\"" + escapeJson(message) + "\"}";
    return response;
}

}

PersonContextController::PersonContextController(
    PersonContextService& service,
    PersonContextJsonSerializer& serializer)
    : service_(service),
      serializer_(serializer)
{
}

ApiResponse PersonContextController::getContext(
    const std::string& name,
    const std::string& providerPersonId,
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    const std::string& fromTime,
    int limit,
    int offset) const
{
    if (name.empty())
    {
        return jsonError(400, "name is required");
    }

    if (limit > 100 || offset < 0)
    {
        return jsonError(400, "limit must not exceed 100 and offset must be non-negative");
    }

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = serializer_.serialize(service_.getContext(
        name,
        providerPersonId,
        backendId,
        channelId,
        eventId,
        fromTime,
        limit,
        offset));
    return response;
}

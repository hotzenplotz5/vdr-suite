#include "HbbtvApiRuntime.h"

#include "HbbtvControlPlaneReadService.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>

namespace
{

constexpr const char* ApplicationsRoute =
    "/api/vdr/broadcast/hbbtv/applications";
constexpr std::size_t MaximumBackendIdBytes = 128U;
constexpr std::size_t MaximumChannelIdBytes = 63U;

std::string requestPath(const std::string& target)
{
    const std::size_t query = target.find('?');
    return query == std::string::npos ? target : target.substr(0, query);
}

int hexValue(char character)
{
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

bool urlDecode(
    const std::string& input,
    std::string& output,
    std::size_t maximumBytes)
{
    output.clear();
    if (input.size() > maximumBytes * 3U) return false;

    for (std::size_t index = 0; index < input.size(); ++index)
    {
        const unsigned char character =
            static_cast<unsigned char>(input[index]);
        if (character == '+')
        {
            output.push_back(' ');
        }
        else if (character == '%')
        {
            if (index + 2U >= input.size()) return false;
            const int high = hexValue(input[index + 1U]);
            const int low = hexValue(input[index + 2U]);
            if (high < 0 || low < 0) return false;
            const unsigned char decoded =
                static_cast<unsigned char>((high << 4) | low);
            if (decoded < 0x20U || decoded == 0x7fU) return false;
            output.push_back(static_cast<char>(decoded));
            index += 2U;
        }
        else
        {
            if (character < 0x20U || character == 0x7fU) return false;
            output.push_back(static_cast<char>(character));
        }

        if (output.size() > maximumBytes) return false;
    }

    return true;
}

bool validBackendId(const std::string& value)
{
    return !value.empty() && value.size() <= MaximumBackendIdBytes &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '-' ||
                character == '_' || character == '.';
        });
}

bool validChannelId(const std::string& value)
{
    return !value.empty() && value.size() <= MaximumChannelIdBytes &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}

struct HbbtvRequest
{
    std::string backendId;
    std::string channelId;
};

bool parseRequest(const std::string& target, HbbtvRequest& request)
{
    request = {};
    if (requestPath(target) != ApplicationsRoute) return false;

    const std::size_t queryStart = target.find('?');
    if (queryStart == std::string::npos || queryStart + 1U >= target.size())
        return false;

    std::map<std::string, std::string> parameters;
    std::size_t start = queryStart + 1U;
    while (start <= target.size())
    {
        const std::size_t end = target.find('&', start);
        const std::string item = target.substr(
            start,
            end == std::string::npos ? std::string::npos : end - start);
        if (item.empty()) return false;

        const std::size_t equals = item.find('=');
        if (equals == std::string::npos || equals == 0U) return false;
        const std::string key = item.substr(0, equals);
        if ((key != "backend" && key != "channel") ||
            parameters.count(key) != 0U)
        {
            return false;
        }

        std::string decoded;
        const std::size_t maximum =
            key == "backend" ? MaximumBackendIdBytes : MaximumChannelIdBytes;
        if (!urlDecode(item.substr(equals + 1U), decoded, maximum))
            return false;
        parameters.emplace(key, std::move(decoded));

        if (end == std::string::npos) break;
        start = end + 1U;
    }

    if (parameters.size() != 2U) return false;
    const auto backend = parameters.find("backend");
    const auto channel = parameters.find("channel");
    if (backend == parameters.end() || channel == parameters.end())
        return false;

    request.backendId = backend->second;
    request.channelId = channel->second;
    return validBackendId(request.backendId) &&
        validChannelId(request.channelId);
}

std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '"': escaped += "\""; break;
        case '\': escaped += "\\"; break;
        case '': escaped += "\b"; break;
        case '': escaped += "\f"; break;
        case '
': escaped += "\n"; break;
        case '': escaped += "\r"; break;
        case '	': escaped += "\t"; break;
        default:
            if (character >= 0x20U)
                escaped.push_back(static_cast<char>(character));
            break;
        }
    }
    return escaped;
}

const char* controlSemantic(std::uint8_t code)
{
    switch (code)
    {
    case 1: return "autostart";
    case 2: return "present";
    case 3: return "destroy";
    case 4: return "kill";
    case 5: return "prefetch";
    case 6: return "remote";
    case 7: return "disabled";
    case 8: return "playback-autostart";
    default: return "reserved";
    }
}

ApiResponse errorResponse(int statusCode, const std::string& code)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.body =
        "{"error":{"code":"" + jsonEscape(code) + ""}}";
    return response;
}

int statusForDomainError(const std::string& error)
{
    if (error == "hbbtv_backend_id_required" ||
        error == "hbbtv_channel_id_required" ||
        error == "hbbtv_discovery_request_invalid")
    {
        return 400;
    }
    if (error == "hbbtv_backend_not_found" ||
        error == "hbbtv_channel_not_in_backend_snapshot")
    {
        return 404;
    }
    if (error == "hbbtv_backend_generation_mismatch" ||
        error == "hbbtv_backend_disabled")
    {
        return 409;
    }
    if (error == "hbbtv_discovery_payload_invalid" ||
        error == "hbbtv_application_descriptor_invalid")
    {
        return 502;
    }
    return 503;
}

ApiResponse serializeDiscovery(
    const BroadcastApplicationDiscoverySnapshot& snapshot)
{
    if (!snapshot.error.empty())
    {
        return errorResponse(statusForDomainError(snapshot.error), snapshot.error);
    }
    if (!snapshot.payloadValid)
    {
        return errorResponse(502, "hbbtv_discovery_payload_invalid");
    }

    std::ostringstream json;
    json << "{"schemaVersion":1"
         << ","result":"" << jsonEscape(snapshot.result) << """
         << ","available":"
         << (!snapshot.applications.empty() ? "true" : "false")
         << ","receiverActive":"
         << (snapshot.receiverActive ? "true" : "false")
         << ","backendGeneration":" << backendGeneration
         << ","channelId":"" << jsonEscape(snapshot.channelId) << """
         << ","discoveryRevision":" << snapshot.revision
         << ","observedAt":" << snapshot.observedAt
         << ","applications":[";

    for (std::size_t index = 0; index < snapshot.applications.size(); ++index)
    {
        if (index != 0U) json << ',';
        const BroadcastApplicationDescriptor& application =
            snapshot.applications[index];
        json << "{"ref":{"applicationId":"
             << application.ref.applicationId
             << ","descriptorRevision":"
             << application.ref.descriptorRevision
             << "},"control":{"code":"
             << static_cast<unsigned>(application.controlCode)
             << ","semantic":""
             << controlSemantic(application.controlCode)
             << ""},"priority":"
             << static_cast<unsigned>(application.priority)
             << ","name":"" << jsonEscape(application.name) << ""}";
    }
    json << "]}";

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.body = json.str();
    return response;
}

}

HbbtvApiRuntime& HbbtvApiRuntime::instance()
{
    static HbbtvApiRuntime runtime;
    return runtime;
}

bool HbbtvApiRuntime::configure(HbbtvControlPlaneReadService& readService)
{
    readService_ = &readService;
    return true;
}

void HbbtvApiRuntime::reset()
{
    readService_ = nullptr;
}

bool HbbtvApiRuntime::configured() const
{
    return readService_ != nullptr;
}

bool HbbtvApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    ApiResponse& response) const
{
    if (requestPath(requestTarget) != ApplicationsRoute)
    {
        return false;
    }

    if (readService_ == nullptr)
    {
        response = errorResponse(503, "hbbtv_runtime_unavailable");
        return true;
    }

    HbbtvRequest request;
    if (!parseRequest(requestTarget, request))
    {
        response = errorResponse(400, "hbbtv_request_invalid");
        return true;
    }

    response = serializeDiscovery(
        readService_->discoverApplications(
            request.backendId,
            request.channelId));
    return true;
}

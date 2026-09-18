#include "TeletextApiRuntime.h"

#include "TeletextControlPlaneReadService.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <map>
#include <sstream>
#include <string>

namespace
{

constexpr const char* ServiceRoute =
    "/api/vdr/broadcast/teletext/service";
constexpr const char* PageRoute =
    "/api/vdr/broadcast/teletext/page";
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

bool parseUnsigned(
    const std::string& text,
    std::uint64_t maximum,
    std::uint64_t& value)
{
    if (text.empty()) return false;
    value = 0;
    for (const unsigned char character : text)
    {
        if (character < '0' || character > '9') return false;
        const unsigned digit = character - '0';
        if (value > (maximum - digit) / 10U) return false;
        value = value * 10U + digit;
    }
    return value <= maximum;
}

struct TeletextRequest
{
    std::string backendId;
    std::string channelId;
    std::uint16_t pageNumber = 0;
    bool automaticSubpage = true;
    std::uint16_t subpageCode = 0;
};

bool parseRequest(
    const std::string& target,
    bool pageRequest,
    TeletextRequest& request)
{
    request = {};
    const std::string expectedPath = pageRequest ? PageRoute : ServiceRoute;
    if (requestPath(target) != expectedPath) return false;

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
        const bool allowed = key == "backend" || key == "channel" ||
            (pageRequest && (key == "page" || key == "subpage"));
        if (!allowed || parameters.count(key) != 0U) return false;

        std::string decoded;
        const std::size_t maximum = key == "backend"
            ? MaximumBackendIdBytes
            : (key == "channel" ? MaximumChannelIdBytes : 16U);
        if (!urlDecode(item.substr(equals + 1U), decoded, maximum))
            return false;
        parameters.emplace(key, std::move(decoded));

        if (end == std::string::npos) break;
        start = end + 1U;
    }

    const auto backend = parameters.find("backend");
    const auto channel = parameters.find("channel");
    if (backend == parameters.end() || channel == parameters.end())
        return false;

    request.backendId = backend->second;
    request.channelId = channel->second;
    if (!validBackendId(request.backendId) ||
        !validChannelId(request.channelId))
    {
        return false;
    }

    if (!pageRequest)
    {
        return parameters.size() == 2U;
    }

    const auto page = parameters.find("page");
    if (page == parameters.end()) return false;

    std::uint64_t pageValue = 0;
    if (!parseUnsigned(page->second, 899U, pageValue) || pageValue < 100U)
        return false;
    request.pageNumber = static_cast<std::uint16_t>(pageValue);

    const auto subpage = parameters.find("subpage");
    if (subpage == parameters.end() || subpage->second == "auto")
    {
        request.automaticSubpage = true;
        request.subpageCode = 0;
    }
    else
    {
        std::uint64_t subpageValue = 0;
        if (!parseUnsigned(subpage->second, 0xfffeU, subpageValue))
            return false;
        request.automaticSubpage = false;
        request.subpageCode = static_cast<std::uint16_t>(subpageValue);
    }

    return parameters.size() == (subpage == parameters.end() ? 3U : 4U);
}

std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
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
            if (character >= 0x20U)
                escaped.push_back(static_cast<char>(character));
            break;
        }
    }
    return escaped;
}

std::string sourceName(TeletextSourceState source)
{
    if (source == TeletextSourceState::Live) return "live";
    if (source == TeletextSourceState::Cached) return "cached";
    return "unknown";
}

ApiResponse errorResponse(int statusCode, const std::string& code)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.body =
        "{\"error\":{\"code\":\"" + jsonEscape(code) + "\"}}";
    return response;
}

int statusForDomainError(const std::string& error)
{
    if (error == "teletext_backend_id_required" ||
        error == "teletext_channel_id_required" ||
        error == "invalid_teletext_service_context" ||
        error == "invalid_teletext_page_context")
    {
        return 400;
    }
    if (error == "teletext_backend_not_found" ||
        error == "teletext_channel_not_in_backend_snapshot")
    {
        return 404;
    }
    if (error == "teletext_backend_generation_mismatch" ||
        error == "teletext_backend_disabled")
    {
        return 409;
    }
    if (error == "teletext_capability_payload_invalid" ||
        error == "teletext_page_payload_invalid")
    {
        return 502;
    }
    return 503;
}

void appendServiceRef(std::ostringstream& json, const TeletextServiceRef& service)
{
    json << "{\"backendId\":\"" << jsonEscape(service.backendId)
         << "\",\"backendGeneration\":" << service.backendGeneration
         << ",\"channelId\":\"" << jsonEscape(service.channelId)
         << "\",\"provider\":{\"id\":\""
         << jsonEscape(service.provider.providerId)
         << "\",\"capabilityRevision\":"
         << service.provider.capabilityRevision
         << ",\"generation\":" << service.provider.providerGeneration
         << ",\"observedAt\":" << service.provider.observedAt
         << "},\"available\":" << (service.available ? "true" : "false")
         << ",\"receiverActive\":"
         << (service.receiverActive ? "true" : "false")
         << ",\"source\":\"" << sourceName(service.source) << "\"}";
}

ApiResponse serializeService(const TeletextServiceSnapshot& snapshot)
{
    if (!snapshot.error.empty())
    {
        return errorResponse(statusForDomainError(snapshot.error), snapshot.error);
    }
    if (!snapshot.payloadValid)
    {
        return errorResponse(502, "teletext_service_payload_invalid");
    }

    std::ostringstream json;
    json << "{\"schemaVersion\":1,\"service\":";
    appendServiceRef(json, snapshot.service);
    json << ",\"rows\":" << snapshot.rows
         << ",\"columns\":" << snapshot.columns
         << ",\"capabilities\":{"
         << "\"subpages\":" << (snapshot.subpages ? "true" : "false")
         << ",\"level1\":" << (snapshot.level1 ? "true" : "false")
         << ",\"x26Partial\":" << (snapshot.x26Partial ? "true" : "false")
         << ",\"conceal\":" << (snapshot.conceal ? "true" : "false")
         << ",\"blink\":" << (snapshot.blink ? "true" : "false")
         << ",\"doubleSize\":" << (snapshot.doubleSize ? "true" : "false")
         << "}}";

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.body = json.str();
    return response;
}

ApiResponse serializePage(const TeletextPageSnapshot& snapshot)
{
    if (!snapshot.error.empty())
    {
        return errorResponse(statusForDomainError(snapshot.error), snapshot.error);
    }
    if (!snapshot.payloadValid)
    {
        return errorResponse(502, "teletext_page_payload_invalid");
    }

    std::ostringstream json;
    json << "{\"schemaVersion\":1,\"result\":\""
         << jsonEscape(snapshot.result)
         << "\",\"pageAvailable\":"
         << (snapshot.pageAvailable ? "true" : "false")
         << ",\"service\":";
    appendServiceRef(json, snapshot.page.service);
    json << ",\"page\":{\"number\":" << snapshot.page.pageNumber
         << ",\"subpage\":" << snapshot.page.subpageCode << "}"
         << ",\"revision\":" << snapshot.revision
         << ",\"complete\":" << (snapshot.complete ? "true" : "false")
         << ",\"rows\":" << snapshot.rows
         << ",\"columns\":" << snapshot.columns
         << ",\"text\":[";

    for (std::size_t index = 0; index < snapshot.textRows.size(); ++index)
    {
        if (index != 0U) json << ',';
        json << '"' << jsonEscape(snapshot.textRows[index]) << '"';
    }

    json << "],\"cells\":[";
    for (std::size_t index = 0; index < snapshot.cells.size(); ++index)
    {
        if (index != 0U) json << ',';
        const TeletextCell& cell = snapshot.cells[index];
        json << '['
             << cell.codepoint << ','
             << static_cast<unsigned>(cell.rawChar) << ','
             << static_cast<unsigned>(cell.charset) << ','
             << static_cast<unsigned>(cell.foreground) << ','
             << static_cast<unsigned>(cell.background) << ','
             << static_cast<unsigned>(cell.kind) << ','
             << static_cast<unsigned>(cell.flags)
             << ']';
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

TeletextApiRuntime& TeletextApiRuntime::instance()
{
    static TeletextApiRuntime runtime;
    return runtime;
}

bool TeletextApiRuntime::configure(TeletextControlPlaneReadService& readService)
{
    readService_ = &readService;
    return true;
}

void TeletextApiRuntime::reset()
{
    readService_ = nullptr;
}

bool TeletextApiRuntime::configured() const
{
    return readService_ != nullptr;
}

bool TeletextApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    ApiResponse& response) const
{
    const std::string path = requestPath(requestTarget);
    const bool serviceRequest = path == ServiceRoute;
    const bool pageRequest = path == PageRoute;
    if (!serviceRequest && !pageRequest)
    {
        return false;
    }

    if (readService_ == nullptr)
    {
        response = errorResponse(503, "teletext_runtime_unavailable");
        return true;
    }

    TeletextRequest request;
    if (!parseRequest(requestTarget, pageRequest, request))
    {
        response = errorResponse(400, "teletext_request_invalid");
        return true;
    }

    const TeletextServiceSnapshot service =
        readService_->discoverService(request.backendId, request.channelId);
    if (!serviceRequest)
    {
        if (!service.error.empty())
        {
            response = serializeService(service);
            return true;
        }
        if (!service.payloadValid)
        {
            response = errorResponse(502, "teletext_service_payload_invalid");
            return true;
        }
        if (!service.service.available)
        {
            response = errorResponse(409, "teletext_service_unavailable");
            return true;
        }

        response = serializePage(readService_->readPage(
            service.service,
            request.pageNumber,
            request.automaticSubpage,
            request.subpageCode));
        return true;
    }

    response = serializeService(service);
    return true;
}

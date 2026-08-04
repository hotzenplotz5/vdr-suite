#include "ManualRecordingMetadataApiRuntime.h"

#include "MetadataController.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <map>
#include <string>

namespace
{
constexpr const char* RoutePrefix = "/api/backends/";
constexpr const char* MetadataSegment = "/recordings/metadata/";
constexpr std::size_t MaximumBodyBytes = 32U * 1024U;

struct Route
{
    std::string backendId;
    std::string operation;
};

std::string requestPath(const std::string& target)
{
    const std::size_t query = target.find('?');
    return query == std::string::npos ? target : target.substr(0, query);
}

bool validBackendId(const std::string& value)
{
    return !value.empty() && value.size() <= 128U &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return (character >= 'A' && character <= 'Z') ||
                (character >= 'a' && character <= 'z') ||
                (character >= '0' && character <= '9') ||
                character == '.' || character == '_' || character == '-';
        });
}

bool parseRoute(const std::string& target, Route& route)
{
    route = {};
    const std::string path = requestPath(target);
    const std::string prefix(RoutePrefix);
    const std::string segment(MetadataSegment);
    if (path.compare(0, prefix.size(), prefix) != 0) return false;
    const std::size_t operationStart = path.find(segment, prefix.size());
    if (operationStart == std::string::npos) return false;
    route.backendId = path.substr(
        prefix.size(), operationStart - prefix.size());
    route.operation = path.substr(operationStart + segment.size());
    if (!validBackendId(route.backendId) || route.operation.empty() ||
        route.operation.find('/') != std::string::npos)
        return false;
    return route.operation == "manual" ||
        route.operation == "search" ||
        route.operation == "seasons" ||
        route.operation == "episodes" ||
        route.operation == "assign" ||
        route.operation == "withdraw";
}

int hexValue(char character)
{
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

bool urlDecode(const std::string& input, std::string& output)
{
    output.clear();
    if (input.size() > 12288U) return false;
    for (std::size_t index = 0; index < input.size(); ++index)
    {
        const unsigned char character =
            static_cast<unsigned char>(input[index]);
        if (character == '+') output.push_back(' ');
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
        if (output.size() > 4096U) return false;
    }
    return true;
}

std::string queryValue(
    const std::string& target,
    const std::string& key)
{
    const std::size_t queryStart = target.find('?');
    if (queryStart == std::string::npos) return {};
    std::size_t start = queryStart + 1U;
    while (start <= target.size())
    {
        const std::size_t end = target.find('&', start);
        const std::string item = target.substr(
            start,
            end == std::string::npos ? std::string::npos : end - start);
        const std::size_t equals = item.find('=');
        if (equals != std::string::npos && item.substr(0, equals) == key)
        {
            std::string decoded;
            return urlDecode(item.substr(equals + 1U), decoded)
                ? decoded : std::string{};
        }
        if (end == std::string::npos) break;
        start = end + 1U;
    }
    return {};
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
            if (character >= 0x20U) escaped.push_back(static_cast<char>(character));
        }
    }
    return escaped;
}

ApiResponse errorResponse(
    int statusCode,
    const std::string& code,
    const std::string& message)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.body =
        "{\"error\":{\"code\":\"" + jsonEscape(code) +
        "\",\"message\":\"" + jsonEscape(message) + "\"}}";
    return response;
}

class FlatJsonParser
{
public:
    explicit FlatJsonParser(const std::string& text) : text_(text) {}

    bool parse(
        std::map<std::string, std::string>& strings,
        std::map<std::string, int>& integers)
    {
        strings.clear();
        integers.clear();
        if (text_.empty() || text_.size() > MaximumBodyBytes) return false;
        whitespace();
        if (!consume('{')) return false;
        whitespace();
        if (consume('}'))
        {
            whitespace();
            return position_ == text_.size();
        }
        while (position_ < text_.size())
        {
            std::string key;
            if (!string(key)) return false;
            whitespace();
            if (!consume(':')) return false;
            whitespace();
            if (position_ < text_.size() && text_[position_] == '"')
            {
                std::string value;
                if (!string(value) ||
                    strings.count(key) > 0 || integers.count(key) > 0)
                    return false;
                strings.emplace(std::move(key), std::move(value));
            }
            else
            {
                int value = 0;
                if (!integer(value) ||
                    strings.count(key) > 0 || integers.count(key) > 0)
                    return false;
                integers.emplace(std::move(key), value);
            }
            whitespace();
            if (consume('}'))
            {
                whitespace();
                return position_ == text_.size();
            }
            if (!consume(',')) return false;
            whitespace();
        }
        return false;
    }

private:
    void whitespace()
    {
        while (position_ < text_.size() &&
               std::isspace(static_cast<unsigned char>(text_[position_])))
            ++position_;
    }

    bool consume(char character)
    {
        if (position_ >= text_.size() || text_[position_] != character)
            return false;
        ++position_;
        return true;
    }

    bool string(std::string& output)
    {
        output.clear();
        if (!consume('"')) return false;
        while (position_ < text_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(text_[position_++]);
            if (character == '"') return true;
            if (character < 0x20U) return false;
            if (character != '\\') output.push_back(static_cast<char>(character));
            else
            {
                if (position_ >= text_.size()) return false;
                const char escaped = text_[position_++];
                if (escaped == '"' || escaped == '\\' || escaped == '/')
                    output.push_back(escaped);
                else if (escaped == 'b') output.push_back('\b');
                else if (escaped == 'f') output.push_back('\f');
                else if (escaped == 'n') output.push_back('\n');
                else if (escaped == 'r') output.push_back('\r');
                else if (escaped == 't') output.push_back('\t');
                else return false;
            }
            if (output.size() > 16384U) return false;
        }
        return false;
    }

    bool integer(int& output)
    {
        const std::size_t start = position_;
        if (position_ < text_.size() && text_[position_] == '-') ++position_;
        const std::size_t digits = position_;
        while (position_ < text_.size() &&
               text_[position_] >= '0' && text_[position_] <= '9')
            ++position_;
        if (digits == position_) return false;
        const std::string token = text_.substr(start, position_ - start);
        errno = 0;
        char* end = nullptr;
        const long value = std::strtol(token.c_str(), &end, 10);
        if (errno != 0 || end == token.c_str() || end == nullptr || *end != '\0' ||
            value < -2147483647L - 1L || value > 2147483647L)
            return false;
        output = static_cast<int>(value);
        return true;
    }

    const std::string& text_;
    std::size_t position_ = 0;
};

std::string stringValue(
    const std::map<std::string, std::string>& values,
    const std::string& key)
{
    const auto iterator = values.find(key);
    return iterator == values.end() ? std::string{} : iterator->second;
}

int intValue(
    const std::map<std::string, int>& values,
    const std::string& key,
    int fallback)
{
    const auto iterator = values.find(key);
    return iterator == values.end() ? fallback : iterator->second;
}

bool parseKind(
    const std::string& value,
    RecordingMetadataCandidateKind& kind)
{
    if (value == "movie")
    {
        kind = RecordingMetadataCandidateKind::Movie;
        return true;
    }
    if (value == "series")
    {
        kind = RecordingMetadataCandidateKind::Series;
        return true;
    }
    return false;
}
}

ManualRecordingMetadataApiRuntime&
ManualRecordingMetadataApiRuntime::instance()
{
    static ManualRecordingMetadataApiRuntime runtime;
    return runtime;
}

void ManualRecordingMetadataApiRuntime::registerController(
    MetadataController& controller)
{
    std::lock_guard<std::mutex> lock(mutex_);
    controller_ = &controller;
}

void ManualRecordingMetadataApiRuntime::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    controller_ = nullptr;
}

MetadataController* ManualRecordingMetadataApiRuntime::controller() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return controller_;
}

bool ManualRecordingMetadataApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    ApiResponse& response) const
{
    Route route;
    if (!parseRoute(requestTarget, route) || route.operation != "manual")
        return false;

    MetadataController* metadata = controller();
    if (metadata == nullptr)
    {
        response = errorResponse(
            503,
            "manual_metadata_runtime_unavailable",
            "Manual recording metadata is unavailable");
        return true;
    }

    const std::string resourceKey = queryValue(requestTarget, "resourceKey");
    if (resourceKey.empty())
    {
        response = errorResponse(
            400,
            "invalid_recording_resource_key",
            "A recording resource key is required");
        return true;
    }

    response = metadata->getManualRecordingMetadata(
        route.backendId,
        resourceKey);
    return true;
}

bool ManualRecordingMetadataApiRuntime::tryHandlePost(
    const std::string& requestTarget,
    const std::string& body,
    const std::string& actorRef,
    ApiResponse& response) const
{
    Route route;
    if (!parseRoute(requestTarget, route) || route.operation == "manual")
        return false;

    MetadataController* metadata = controller();
    if (metadata == nullptr)
    {
        response = errorResponse(
            503,
            "manual_metadata_runtime_unavailable",
            "Manual recording metadata is unavailable");
        return true;
    }
    if (actorRef.empty())
    {
        response = errorResponse(
            401,
            "authenticated_actor_required",
            "An authenticated actor is required");
        return true;
    }

    std::map<std::string, std::string> strings;
    std::map<std::string, int> integers;
    FlatJsonParser parser(body);
    if (!parser.parse(strings, integers))
    {
        response = errorResponse(
            400,
            "invalid_metadata_request",
            "The recording metadata request payload is invalid");
        return true;
    }

    const int limit = intValue(integers, "limit", 10);
    if (route.operation == "search")
    {
        RecordingMetadataCandidateKind kind;
        if (!parseKind(stringValue(strings, "kind"), kind))
        {
            response = errorResponse(
                400,
                "invalid_metadata_candidate_kind",
                "Candidate kind must be movie or series");
            return true;
        }
        response = metadata->searchRecordingMetadataCandidates(
            route.backendId,
            stringValue(strings, "query"),
            kind,
            limit);
        return true;
    }

    if (route.operation == "seasons")
    {
        response = metadata->getRecordingMetadataSeasons(
            route.backendId,
            stringValue(strings, "seriesExternalId"),
            limit);
        return true;
    }

    if (route.operation == "episodes")
    {
        response = metadata->getRecordingMetadataEpisodes(
            route.backendId,
            stringValue(strings, "seriesExternalId"),
            intValue(integers, "seasonNumber", 0),
            limit);
        return true;
    }

    if (route.operation == "assign")
    {
        ManualRecordingMetadataSelection selection;
        selection.backendId = route.backendId;
        selection.resourceKey = stringValue(strings, "resourceKey");
        selection.providerId = stringValue(strings, "providerId");
        selection.externalNamespace = stringValue(strings, "externalNamespace");
        selection.externalId = stringValue(strings, "externalId");
        selection.mediaType = stringValue(strings, "mediaType");
        selection.title = stringValue(strings, "title");
        selection.originalTitle = stringValue(strings, "originalTitle");
        selection.overview = stringValue(strings, "overview");
        selection.releaseDate = stringValue(strings, "releaseDate");
        selection.posterReference = stringValue(strings, "posterReference");
        selection.seasonNumber = intValue(integers, "seasonNumber", 0);
        selection.episodeNumber = intValue(integers, "episodeNumber", 0);
        selection.expectedRevision = intValue(integers, "expectedRevision", 0);
        response = metadata->assignManualRecordingMetadata(
            std::move(selection),
            actorRef);
        return true;
    }

    if (route.operation == "withdraw")
    {
        response = metadata->withdrawManualRecordingMetadata(
            route.backendId,
            stringValue(strings, "resourceKey"),
            intValue(integers, "expectedRevision", 0),
            actorRef);
        return true;
    }

    return false;
}

#include "SeriesArtworkSettingsApiRuntime.h"

#include "SeriesArtworkBackendSettingsService.h"

#include <cctype>
#include <map>
#include <string>

namespace
{
constexpr const char* RouteSuffix = "/settings/series-artwork";
constexpr const char* RoutePrefix = "/api/backends/";

std::string requestPath(const std::string& target)
{
    const std::size_t query = target.find('?');
    return query == std::string::npos
        ? target
        : target.substr(0, query);
}

bool routeBackendId(
    const std::string& requestTarget,
    std::string& backendId)
{
    const std::string path = requestPath(requestTarget);
    const std::string prefix(RoutePrefix);
    const std::string suffix(RouteSuffix);
    if (path.size() <= prefix.size() + suffix.size() ||
        path.compare(0, prefix.size(), prefix) != 0 ||
        path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0)
    {
        return false;
    }

    backendId = path.substr(
        prefix.size(),
        path.size() - prefix.size() - suffix.size());
    return SeriesArtworkBackendSettingsService::validBackendId(backendId);
}

bool routeBackendIdForSuffix(
    const std::string& requestTarget,
    const std::string& requestedSuffix,
    std::string& backendId)
{
    const std::string path =
        requestPath(requestTarget);

    const std::string prefix(
        RoutePrefix);

    if (path.size() <=
            prefix.size() +
            requestedSuffix.size() ||
        path.compare(
            0,
            prefix.size(),
            prefix) != 0 ||
        path.compare(
            path.size() -
                requestedSuffix.size(),
            requestedSuffix.size(),
            requestedSuffix) != 0)
    {
        return false;
    }

    backendId = path.substr(
        prefix.size(),
        path.size() -
            prefix.size() -
            requestedSuffix.size());

    return
        SeriesArtworkBackendSettingsService::
            validBackendId(
                backendId);
}

int hexValue(char character)
{
    if (character >= '0' &&
        character <= '9')
    {
        return character - '0';
    }

    if (character >= 'a' &&
        character <= 'f')
    {
        return character - 'a' + 10;
    }

    if (character >= 'A' &&
        character <= 'F')
    {
        return character - 'A' + 10;
    }

    return -1;
}

bool urlDecode(
    const std::string& input,
    std::string& output)
{
    output.clear();

    if (input.size() > 4096U)
    {
        return false;
    }

    for (std::size_t index = 0;
         index < input.size();
         ++index)
    {
        const unsigned char character =
            static_cast<unsigned char>(
                input[index]);

        if (character == '+')
        {
            output.push_back(' ');
        }
        else if (character == '%')
        {
            if (index + 2U >= input.size())
            {
                return false;
            }

            const int high =
                hexValue(input[index + 1U]);

            const int low =
                hexValue(input[index + 2U]);

            if (high < 0 || low < 0)
            {
                return false;
            }

            const unsigned char decoded =
                static_cast<unsigned char>(
                    (high << 4) | low);

            if (decoded < 0x20U ||
                decoded == 0x7fU)
            {
                return false;
            }

            output.push_back(
                static_cast<char>(decoded));

            index += 2U;
        }
        else
        {
            if (character < 0x20U ||
                character == 0x7fU)
            {
                return false;
            }

            output.push_back(
                static_cast<char>(character));
        }

        if (output.size() > 4096U)
        {
            return false;
        }
    }

    return true;
}

std::string queryValue(
    const std::string& target,
    const std::string& key)
{
    const std::size_t queryStart =
        target.find('?');

    if (queryStart ==
        std::string::npos)
    {
        return {};
    }

    std::size_t start =
        queryStart + 1U;

    while (start <= target.size())
    {
        const std::size_t end =
            target.find('&', start);

        const std::string item =
            target.substr(
                start,
                end == std::string::npos
                    ? std::string::npos
                    : end - start);

        const std::size_t equals =
            item.find('=');

        if (equals != std::string::npos &&
            item.substr(0, equals) == key)
        {
            std::string decoded;

            return urlDecode(
                item.substr(equals + 1U),
                decoded)
                ? decoded
                : std::string{};
        }

        if (end == std::string::npos)
        {
            break;
        }

        start = end + 1U;
    }

    return {};
}

std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value)
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
                if (static_cast<unsigned char>(character) >= 0x20U)
                {
                    escaped.push_back(character);
                }
                break;
        }
    }
    return escaped;
}

std::string serialize(
    const SeriesArtworkBackendSettingsSnapshot& settings)
{
    std::string json =
        "{\"backendId\":\"" + jsonEscape(settings.backendId) +
        "\",\"provider\":\"" + jsonEscape(settings.provider) +
        "\",\"configurationSource\":\"" +
        jsonEscape(settings.configurationSource) +
        "\",\"tmdbTokenConfigured\":" +
        (settings.tmdbTokenConfigured ? "true" : "false") +
        ",\"tmdbTokenSource\":\"" +
        jsonEscape(settings.tmdbTokenSource) +
        "\",\"restartRequired\":false,"
        "\"availableProviders\":[\"none\",\"tvmaze\",\"tmdb\"],"
        "\"coverOverrides\":[";

    for (std::size_t index = 0;
         index < settings.coverOverrides.size();
         ++index)
    {
        if (index > 0U)
        {
            json += ',';
        }

        const SeriesArtworkCoverOverride& value =
            settings.coverOverrides[index];

        json +=
            "{\"seriesKey\":\"" + jsonEscape(value.seriesKey) +
            "\",\"posterUrl\":\"" + jsonEscape(value.posterUrl) +
            "\",\"revision\":" + std::to_string(value.revision) +
            "}";
    }

    json += "]}";
    return json;
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
    explicit FlatJsonParser(const std::string& text)
        : text_(text)
    {
    }

    bool parse(std::map<std::string, std::string>& strings,
               std::map<std::string, bool>& booleans)
    {
        strings.clear();
        booleans.clear();
        skipWhitespace();
        if (!consume('{'))
        {
            return false;
        }
        skipWhitespace();
        if (consume('}'))
        {
            skipWhitespace();
            return position_ == text_.size();
        }

        while (position_ < text_.size())
        {
            std::string key;
            if (!parseString(key))
            {
                return false;
            }
            skipWhitespace();
            if (!consume(':'))
            {
                return false;
            }
            skipWhitespace();

            if (position_ < text_.size() && text_[position_] == '"')
            {
                std::string value;
                if (!parseString(value) || !strings.emplace(key, value).second)
                {
                    return false;
                }
            }
            else if (consumeLiteral("true"))
            {
                if (!booleans.emplace(key, true).second)
                {
                    return false;
                }
            }
            else if (consumeLiteral("false"))
            {
                if (!booleans.emplace(key, false).second)
                {
                    return false;
                }
            }
            else
            {
                return false;
            }

            skipWhitespace();
            if (consume('}'))
            {
                skipWhitespace();
                return position_ == text_.size();
            }
            if (!consume(','))
            {
                return false;
            }
            skipWhitespace();
        }
        return false;
    }

private:
    void skipWhitespace()
    {
        while (position_ < text_.size() &&
               std::isspace(static_cast<unsigned char>(text_[position_])))
        {
            ++position_;
        }
    }

    bool consume(char character)
    {
        if (position_ >= text_.size() || text_[position_] != character)
        {
            return false;
        }
        ++position_;
        return true;
    }

    bool consumeLiteral(const char* literal)
    {
        const std::string value(literal);
        if (text_.compare(position_, value.size(), value) != 0)
        {
            return false;
        }
        position_ += value.size();
        return true;
    }

    bool parseString(std::string& value)
    {
        if (!consume('"'))
        {
            return false;
        }
        value.clear();
        while (position_ < text_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(text_[position_++]);
            if (character == '"')
            {
                return true;
            }
            if (character < 0x20U)
            {
                return false;
            }
            if (character != '\\')
            {
                value.push_back(static_cast<char>(character));
                continue;
            }
            if (position_ >= text_.size())
            {
                return false;
            }
            const char escaped = text_[position_++];
            switch (escaped)
            {
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case '/': value.push_back('/'); break;
                case 'b': value.push_back('\b'); break;
                case 'f': value.push_back('\f'); break;
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                default: return false;
            }
        }
        return false;
    }

    const std::string& text_;
    std::size_t position_ = 0;
};

std::string stringValue(
    const std::map<std::string, std::string>& values,
    const std::string& key)
{
    const auto iterator = values.find(key);
    return iterator == values.end() ? std::string() : iterator->second;
}

bool boolValue(
    const std::map<std::string, bool>& values,
    const std::string& key)
{
    const auto iterator = values.find(key);
    return iterator != values.end() && iterator->second;
}
}

SeriesArtworkSettingsApiRuntime&
SeriesArtworkSettingsApiRuntime::instance()
{
    static SeriesArtworkSettingsApiRuntime runtime;
    return runtime;
}

void SeriesArtworkSettingsApiRuntime::registerBackend(
    const std::string& backendId,
    SeriesArtworkBackendSettingsService& service)
{
    std::lock_guard<std::mutex> lock(mutex_);
    services_[backendId] = &service;
}

void SeriesArtworkSettingsApiRuntime::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    services_.clear();
}

SeriesArtworkBackendSettingsService*
SeriesArtworkSettingsApiRuntime::findService(
    const std::string& backendId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = services_.find(backendId);
    return iterator == services_.end() ? nullptr : iterator->second;
}

bool SeriesArtworkSettingsApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    ApiResponse& response) const
{
    std::string backendId;

    if (routeBackendIdForSuffix(
            requestTarget,
            "/settings/series-artwork/candidate-image",
            backendId))
    {
        SeriesArtworkBackendSettingsService* service =
            findService(backendId);

        if (service == nullptr)
        {
            response = errorResponse(
                404,
                "backend_artwork_settings_not_found",
                "Series artwork settings are unavailable for this backend");
            return true;
        }

        const std::string externalId =
            queryValue(
                requestTarget,
                "externalId");

        const std::string posterReference =
            queryValue(
                requestTarget,
                "posterReference");

        const SeriesArtworkImageResult image =
            service->tmdbCandidateImage(
                backendId,
                externalId,
                posterReference);

        if (!image.success)
        {
            response = errorResponse(
                image.statusCode,
                image.errorCode,
                image.message);
            return true;
        }

        response.statusCode = 200;
        response.contentType =
            image.contentType;
        response.body = image.body;

        return true;
    }

    if (routeBackendIdForSuffix(
            requestTarget,
            "/settings/series-artwork/image",
            backendId))
    {
        SeriesArtworkBackendSettingsService* service =
            findService(backendId);

        if (service == nullptr)
        {
            response = errorResponse(
                404,
                "backend_artwork_settings_not_found",
                "Series artwork settings are unavailable for this backend");
            return true;
        }

        const std::string seriesKey =
            queryValue(
                requestTarget,
                "seriesKey");

        const SeriesArtworkImageResult image =
            service->coverImage(
                backendId,
                seriesKey);

        if (!image.success)
        {
            response = errorResponse(
                image.statusCode,
                image.errorCode,
                image.message);
            return true;
        }

        response.statusCode = 200;
        response.contentType =
            image.contentType;
        response.body = image.body;

        return true;
    }

    if (!routeBackendId(
            requestTarget,
            backendId))
    {
        return false;
    }

    SeriesArtworkBackendSettingsService* service =
        findService(backendId);

    if (service == nullptr)
    {
        response = errorResponse(
            404,
            "backend_artwork_settings_not_found",
            "Series artwork settings are unavailable for this backend");

        return true;
    }

    const SeriesArtworkBackendSettingsSnapshot settings =
        service->get(backendId);

    if (settings.backendId.empty())
    {
        response = errorResponse(
            503,
            "backend_artwork_settings_unavailable",
            "Series artwork settings could not be read");

        return true;
    }

    response.statusCode = 200;
    response.contentType =
        "application/json";
    response.body = serialize(settings);

    return true;
}

bool SeriesArtworkSettingsApiRuntime::tryHandlePost(
    const std::string& requestTarget,
    const std::string& body,
    ApiResponse& response) const
{
    std::string backendId;
    if (!routeBackendId(requestTarget, backendId))
    {
        return false;
    }

    std::map<std::string, std::string> strings;
    std::map<std::string, bool> booleans;
    FlatJsonParser parser(body);
    if (!parser.parse(strings, booleans))
    {
        response = errorResponse(
            400,
            "invalid_settings_payload",
            "The series artwork settings payload is invalid");
        return true;
    }

    SeriesArtworkBackendSettingsUpdate request;
    request.backendId = stringValue(strings, "backendId");
    request.provider = stringValue(strings, "provider");
    request.operation = stringValue(strings, "operation");
    request.seriesKey = stringValue(strings, "seriesKey");
    request.posterUrl = stringValue(strings, "posterUrl");
    request.providerId = stringValue(strings, "providerId");
    request.externalNamespace =
        stringValue(strings, "externalNamespace");
    request.externalId =
        stringValue(strings, "externalId");
    request.posterReference =
        stringValue(strings, "posterReference");
    request.tmdbReadAccessToken =
        stringValue(strings, "tmdbReadAccessToken");
    request.clearTmdbReadAccessToken =
        boolValue(booleans, "clearTmdbReadAccessToken");

    if (request.backendId != backendId)
    {
        response = errorResponse(
            400,
            "backend_id_mismatch",
            "The backend ID in the route and payload must match");
        return true;
    }

    SeriesArtworkBackendSettingsService* service = findService(backendId);
    if (service == nullptr)
    {
        response = errorResponse(
            404,
            "backend_artwork_settings_not_found",
            "Series artwork settings are unavailable for this backend");
        return true;
    }

    const SeriesArtworkBackendSettingsUpdateResult result =
        service->update(request);
    if (!result.success)
    {
        response = errorResponse(
            result.statusCode,
            result.errorCode,
            result.message);
        return true;
    }

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = serialize(result.settings);
    return true;
}

#include "RecordingSeriesHierarchyApiRuntime.h"

#include "RecordingSeriesHierarchyOverrideRepository.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <map>
#include <string>

namespace
{
constexpr const char* RoutePrefix =
    "/api/backends/";

constexpr const char* RouteSuffix =
    "/recordings/series-hierarchy";

constexpr std::size_t MaximumBodyBytes =
    16U * 1024U;

bool validBackendId(const std::string& value)
{
    return !value.empty() &&
        value.size() <= 128U &&
        std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character) {
                return
                    (character >= 'A' &&
                     character <= 'Z') ||
                    (character >= 'a' &&
                     character <= 'z') ||
                    (character >= '0' &&
                     character <= '9') ||
                    character == '.' ||
                    character == '_' ||
                    character == '-';
            });
}

std::string requestPath(
    const std::string& target)
{
    const std::size_t query =
        target.find('?');

    return query == std::string::npos
        ? target
        : target.substr(0, query);
}

bool parseBackendId(
    const std::string& target,
    std::string& backendId)
{
    backendId.clear();

    const std::string path =
        requestPath(target);

    const std::string prefix(RoutePrefix);
    const std::string suffix(RouteSuffix);

    if (path.compare(
            0,
            prefix.size(),
            prefix) != 0)
    {
        return false;
    }

    if (path.size() <=
        prefix.size() + suffix.size())
    {
        return false;
    }

    if (path.compare(
            path.size() - suffix.size(),
            suffix.size(),
            suffix) != 0)
    {
        return false;
    }

    backendId = path.substr(
        prefix.size(),
        path.size() -
            prefix.size() -
            suffix.size());

    return validBackendId(backendId);
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

    if (input.size() > 12288U)
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

    if (queryStart == std::string::npos)
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

std::string jsonEscape(
    const std::string& value)
{
    std::string escaped;

    for (const unsigned char character :
         value)
    {
        switch (character)
        {
        case '"':
            escaped += "\\\"";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            if (character >= 0x20U)
            {
                escaped.push_back(
                    static_cast<char>(
                        character));
            }
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
    response.contentType =
        "application/json";

    response.headers["Cache-Control"] =
        "no-store";

    response.body =
        "{\"error\":{\"code\":\"" +
        jsonEscape(code) +
        "\",\"message\":\"" +
        jsonEscape(message) +
        "\"}}";

    return response;
}

ApiResponse valueResponse(
    const RecordingSeriesHierarchyOverride& value)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType =
        "application/json";

    response.headers["Cache-Control"] =
        "no-store";

    if (!value.available)
    {
        response.body =
            "{\"available\":false}";

        return response;
    }

    response.body =
        "{\"available\":true"
        ",\"backendId\":\"" +
        jsonEscape(value.backendId) +
        "\""
        ",\"recordingKey\":\"" +
        jsonEscape(value.recordingKey) +
        "\""
        ",\"groupType\":\"" +
        jsonEscape(value.groupType) +
        "\""
        ",\"seasonNumber\":" +
        std::to_string(
            value.seasonNumber) +
        ",\"groupLabel\":\"" +
        jsonEscape(value.groupLabel) +
        "\""
        ",\"sortOrder\":" +
        std::to_string(
            value.sortOrder) +
        ",\"episodeStart\":" +
        std::to_string(
            value.episodeStart) +
        ",\"episodeEnd\":" +
        std::to_string(
            value.episodeEnd) +
        ",\"revision\":" +
        std::to_string(
            value.revision) +
        "}";

    return response;
}

class FlatJsonParser
{
public:
    explicit FlatJsonParser(
        const std::string& text)
        : text_(text)
    {
    }

    bool parse(
        std::map<
            std::string,
            std::string>& strings,
        std::map<
            std::string,
            int>& integers)
    {
        strings.clear();
        integers.clear();

        if (text_.empty() ||
            text_.size() >
                MaximumBodyBytes)
        {
            return false;
        }

        whitespace();

        if (!consume('{'))
        {
            return false;
        }

        whitespace();

        if (consume('}'))
        {
            whitespace();
            return position_ ==
                text_.size();
        }

        while (position_ <
               text_.size())
        {
            std::string key;

            if (!string(key))
            {
                return false;
            }

            whitespace();

            if (!consume(':'))
            {
                return false;
            }

            whitespace();

            if (position_ <
                    text_.size() &&
                text_[position_] == '"')
            {
                std::string value;

                if (!string(value) ||
                    strings.count(key) > 0 ||
                    integers.count(key) > 0)
                {
                    return false;
                }

                strings.emplace(
                    std::move(key),
                    std::move(value));
            }
            else
            {
                int value = 0;

                if (!integer(value) ||
                    strings.count(key) > 0 ||
                    integers.count(key) > 0)
                {
                    return false;
                }

                integers.emplace(
                    std::move(key),
                    value);
            }

            whitespace();

            if (consume('}'))
            {
                whitespace();

                return position_ ==
                    text_.size();
            }

            if (!consume(','))
            {
                return false;
            }

            whitespace();
        }

        return false;
    }

private:
    void whitespace()
    {
        while (
            position_ < text_.size() &&
            std::isspace(
                static_cast<unsigned char>(
                    text_[position_])))
        {
            ++position_;
        }
    }

    bool consume(char character)
    {
        if (position_ >= text_.size() ||
            text_[position_] != character)
        {
            return false;
        }

        ++position_;
        return true;
    }

    bool string(std::string& output)
    {
        output.clear();

        if (!consume('"'))
        {
            return false;
        }

        while (position_ <
               text_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(
                    text_[position_++]);

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
                output.push_back(
                    static_cast<char>(
                        character));
            }
            else
            {
                if (position_ >=
                    text_.size())
                {
                    return false;
                }

                const char escaped =
                    text_[position_++];

                if (escaped == '"' ||
                    escaped == '\\' ||
                    escaped == '/')
                {
                    output.push_back(
                        escaped);
                }
                else if (escaped == 'n')
                {
                    output.push_back('\n');
                }
                else if (escaped == 'r')
                {
                    output.push_back('\r');
                }
                else if (escaped == 't')
                {
                    output.push_back('\t');
                }
                else
                {
                    return false;
                }
            }

            if (output.size() >
                16384U)
            {
                return false;
            }
        }

        return false;
    }

    bool integer(int& output)
    {
        const std::size_t start =
            position_;

        if (position_ < text_.size() &&
            text_[position_] == '-')
        {
            ++position_;
        }

        const std::size_t digits =
            position_;

        while (
            position_ < text_.size() &&
            text_[position_] >= '0' &&
            text_[position_] <= '9')
        {
            ++position_;
        }

        if (digits == position_)
        {
            return false;
        }

        const std::string token =
            text_.substr(
                start,
                position_ - start);

        errno = 0;
        char* end = nullptr;

        const long value =
            std::strtol(
                token.c_str(),
                &end,
                10);

        if (errno != 0 ||
            end == token.c_str() ||
            end == nullptr ||
            *end != '\0' ||
            value <
                -2147483647L - 1L ||
            value >
                2147483647L)
        {
            return false;
        }

        output =
            static_cast<int>(value);

        return true;
    }

    const std::string& text_;
    std::size_t position_ = 0;
};

std::string stringValue(
    const std::map<
        std::string,
        std::string>& values,
    const std::string& key)
{
    const auto iterator =
        values.find(key);

    return iterator == values.end()
        ? std::string{}
        : iterator->second;
}

int intValue(
    const std::map<
        std::string,
        int>& values,
    const std::string& key,
    int fallback)
{
    const auto iterator =
        values.find(key);

    return iterator == values.end()
        ? fallback
        : iterator->second;
}
}

RecordingSeriesHierarchyApiRuntime&
RecordingSeriesHierarchyApiRuntime::instance()
{
    static RecordingSeriesHierarchyApiRuntime
        runtime;

    return runtime;
}

bool RecordingSeriesHierarchyApiRuntime::configure(
    Database& database)
{
    RecordingSeriesHierarchyOverrideRepository
        repository(database);

    if (!repository.ensureSchema())
    {
        return false;
    }

    std::lock_guard<std::mutex>
        lock(mutex_);

    database_ = &database;
    return true;
}

bool RecordingSeriesHierarchyApiRuntime::configured()
    const
{
    std::lock_guard<std::mutex>
        lock(mutex_);

    return database_ != nullptr;
}

void RecordingSeriesHierarchyApiRuntime::reset()
{
    std::lock_guard<std::mutex>
        lock(mutex_);

    database_ = nullptr;
}

Database*
RecordingSeriesHierarchyApiRuntime::database()
    const
{
    std::lock_guard<std::mutex>
        lock(mutex_);

    return database_;
}

bool RecordingSeriesHierarchyApiRuntime::
tryHandleGet(
    const std::string& requestTarget,
    ApiResponse& response) const
{
    std::string backendId;

    if (!parseBackendId(
            requestTarget,
            backendId))
    {
        return false;
    }

    Database* db = database();

    if (db == nullptr)
    {
        response = errorResponse(
            503,
            "series_hierarchy_runtime_unavailable",
            "Series hierarchy overrides are unavailable");

        return true;
    }

    const std::string recordingKey =
        queryValue(
            requestTarget,
            "resourceKey");

    if (recordingKey.empty())
    {
        response = errorResponse(
            400,
            "invalid_recording_resource_key",
            "A recording resource key is required");

        return true;
    }

    RecordingSeriesHierarchyOverrideRepository
        repository(*db);

    response = valueResponse(
        repository.get(
            backendId,
            recordingKey));

    return true;
}

bool RecordingSeriesHierarchyApiRuntime::
tryHandlePost(
    const std::string& requestTarget,
    const std::string& body,
    const std::string& actorRef,
    ApiResponse& response) const
{
    std::string backendId;

    if (!parseBackendId(
            requestTarget,
            backendId))
    {
        return false;
    }

    Database* db = database();

    if (db == nullptr)
    {
        response = errorResponse(
            503,
            "series_hierarchy_runtime_unavailable",
            "Series hierarchy overrides are unavailable");

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

    std::map<
        std::string,
        std::string> strings;

    std::map<
        std::string,
        int> integers;

    FlatJsonParser parser(body);

    if (!parser.parse(
            strings,
            integers))
    {
        response = errorResponse(
            400,
            "invalid_series_hierarchy_request",
            "Series hierarchy request payload is invalid");

        return true;
    }

    const std::string operation =
        stringValue(
            strings,
            "operation");

    const std::string recordingKey =
        stringValue(
            strings,
            "resourceKey");

    RecordingSeriesHierarchyOverrideRepository
        repository(*db);

    if (operation == "clear")
    {
        const auto result =
            repository.clear(
                backendId,
                recordingKey,
                intValue(
                    integers,
                    "expectedRevision",
                    0));

        if (!result.success)
        {
            response = errorResponse(
                result.statusCode,
                result.errorCode,
                result.message);

            return true;
        }

        response =
            valueResponse({});

        return true;
    }

    if (operation != "set")
    {
        response = errorResponse(
            400,
            "invalid_series_hierarchy_operation",
            "Operation must be set or clear");

        return true;
    }

    RecordingSeriesHierarchyOverrideMutation
        mutation;

    mutation.backendId =
        backendId;

    mutation.recordingKey =
        recordingKey;

    mutation.groupType =
        stringValue(
            strings,
            "groupType");

    mutation.seasonNumber =
        intValue(
            integers,
            "seasonNumber",
            0);

    mutation.groupLabel =
        stringValue(
            strings,
            "groupLabel");

    mutation.sortOrder =
        intValue(
            integers,
            "sortOrder",
            0);

    mutation.episodeStart =
        intValue(
            integers,
            "episodeStart",
            0);

    mutation.episodeEnd =
        intValue(
            integers,
            "episodeEnd",
            0);

    mutation.expectedRevision =
        intValue(
            integers,
            "expectedRevision",
            0);

    const auto result =
        repository.set(mutation);

    if (!result.success)
    {
        response = errorResponse(
            result.statusCode,
            result.errorCode,
            result.message);

        return true;
    }

    response =
        valueResponse(result.value);

    return true;
}

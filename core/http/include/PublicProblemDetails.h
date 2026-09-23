#pragma once

#include <string>

struct PublicProblemDetails
{
    int statusCode = 500;
    std::string code;
    std::string title;
    std::string detail;
    std::string instance;
    std::string requestId;
    std::string correlationId;

    std::string serialize() const
    {
        std::string body =
            "{\"type\":\"urn:vdr-suite:error:" +
            jsonEscape(typeSuffix(code)) +
            "\",\"title\":\"" + jsonEscape(
                title.empty()
                    ? defaultTitle(statusCode)
                    : title) +
            "\",\"status\":" + std::to_string(statusCode);

        if (!detail.empty())
        {
            body +=
                ",\"detail\":\"" +
                jsonEscape(detail) + "\"";
        }

        if (!instance.empty())
        {
            body +=
                ",\"instance\":\"" +
                jsonEscape(instance) + "\"";
        }

        body +=
            ",\"code\":\"" +
            jsonEscape(code) +
            "\",\"requestId\":\"" +
            jsonEscape(requestId) + "\"";

        if (!correlationId.empty())
        {
            body +=
                ",\"correlationId\":\"" +
                jsonEscape(correlationId) + "\"";
        }

        body += "}";
        return body;
    }

    static constexpr const char* contentType()
    {
        return "application/problem+json";
    }

    static std::string defaultTitle(int statusCode)
    {
        switch (statusCode)
        {
            case 400: return "Invalid request";
            case 401: return "Authentication required";
            case 403: return "Request forbidden";
            case 404: return "Resource not found";
            case 405: return "Method not allowed";
            case 409: return "Conflict";
            case 410: return "Resource gone";
            case 412: return "Precondition failed";
            case 415: return "Unsupported media type";
            case 422: return "Validation failed";
            case 428: return "Precondition required";
            case 429: return "Rate limit exceeded";
            case 500: return "Internal server error";
            case 502: return "Upstream error";
            case 503: return "Service unavailable";
            case 504: return "Upstream timeout";
            default: return "Request failed";
        }
    }

private:
    static std::string typeSuffix(const std::string& code)
    {
        std::string value = code;
        for (char& character : value)
        {
            if (character == '_')
            {
                character = '-';
            }
        }
        return value;
    }

    static std::string jsonEscape(const std::string& value)
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
                    if (static_cast<unsigned char>(character) >= 0x20)
                    {
                        escaped.push_back(character);
                    }
                    break;
            }
        }

        return escaped;
    }
};

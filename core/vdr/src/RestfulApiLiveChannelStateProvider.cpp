#include "RestfulApiLiveChannelStateProvider.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "JsonStringDecoder.h"

#include <cctype>
#include <exception>

RestfulApiLiveChannelStateProvider::RestfulApiLiveChannelStateProvider(
    IHttpClient& httpClient)
    : httpClient_(httpClient)
{
}

std::string RestfulApiLiveChannelStateProvider::parseChannelId(
    const std::string& json)
{
    const std::string key = "\"channel\"";
    const std::size_t keyPosition = json.find(key);

    if (keyPosition == std::string::npos)
    {
        return "";
    }

    const std::size_t colon = json.find(':', keyPosition + key.size());

    if (colon == std::string::npos)
    {
        return "";
    }

    std::size_t quote = colon + 1;

    while (quote < json.size() && std::isspace(static_cast<unsigned char>(json[quote])))
    {
        ++quote;
    }

    if (quote >= json.size() || json[quote] != '"')
    {
        return "";
    }

    bool escaped = false;

    for (std::size_t end = quote + 1; end < json.size(); ++end)
    {
        const char character = json[end];

        if (escaped)
        {
            escaped = false;
            continue;
        }

        if (character == '\\')
        {
            escaped = true;
            continue;
        }

        if (character == '"')
        {
            return vdrsuite::decodeJsonStringEscapes(
                json.substr(quote + 1, end - quote - 1));
        }
    }

    return "";
}

LiveChannelState RestfulApiLiveChannelStateProvider::getState() const
{
    LiveChannelState state;
    HttpRequest request;
    request.method = "GET";
    request.url = "/info.json";
    request.headers["Accept"] = "application/json";

    try
    {
        const HttpResponse response = httpClient_.execute(request);

        if (response.statusCode != 200)
        {
            state.message = "RESTfulAPI live channel source returned HTTP " +
                std::to_string(response.statusCode);
            return state;
        }

        state.channelId = parseChannelId(response.body);
        state.available = !state.channelId.empty();
        state.message = state.available
            ? "RESTfulAPI live channel available"
            : "RESTfulAPI info response contains no live channel";
        return state;
    }
    catch (const std::exception& error)
    {
        state.message = error.what();
        return state;
    }
    catch (...)
    {
        state.message = "RESTfulAPI live channel transport failed";
        return state;
    }
}

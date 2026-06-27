#include "RestfulApiSearchTimerDiscoveryProvider.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "IHttpClient.h"

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string buildUrl(
    const std::string& basePath,
    const std::string& endpoint)
{
    if (basePath.empty())
    {
        return endpoint;
    }

    if (basePath.back() == '/' && endpoint.front() == '/')
    {
        return basePath.substr(0, basePath.size() - 1) + endpoint;
    }

    if (basePath.back() != '/' && endpoint.front() != '/')
    {
        return basePath + "/" + endpoint;
    }

    return basePath + endpoint;
}

HttpResponse getJson(
    const IHttpClient& httpClient,
    const std::string& basePath,
    const std::string& endpoint)
{
    HttpRequest request;
    request.method = "GET";
    request.url = buildUrl(basePath, endpoint);
    request.headers["Accept"] = "application/json";
    return httpClient.execute(request);
}

std::size_t findArrayStart(
    const std::string& body,
    const std::string& key)
{
    const std::string marker = "\"" + key + "\"";
    const std::size_t keyPosition = body.find(marker);

    if (keyPosition == std::string::npos)
    {
        return std::string::npos;
    }

    const std::size_t colonPosition = body.find(':', keyPosition + marker.size());
    if (colonPosition == std::string::npos)
    {
        return std::string::npos;
    }

    return body.find('[', colonPosition + 1);
}

std::size_t findMatchingArrayEnd(
    const std::string& body,
    const std::size_t arrayStart)
{
    int depth = 0;
    bool insideString = false;
    bool escaped = false;

    for (std::size_t index = arrayStart; index < body.size(); ++index)
    {
        const char character = body[index];

        if (escaped)
        {
            escaped = false;
            continue;
        }

        if (insideString && character == '\\')
        {
            escaped = true;
            continue;
        }

        if (character == '"')
        {
            insideString = !insideString;
            continue;
        }

        if (insideString)
        {
            continue;
        }

        if (character == '[')
        {
            ++depth;
        }
        else if (character == ']')
        {
            --depth;
            if (depth == 0)
            {
                return index;
            }
        }
    }

    return std::string::npos;
}

std::vector<std::string> extractStringArray(
    const std::string& body,
    const std::string& key)
{
    std::vector<std::string> values;

    const std::size_t arrayStart = findArrayStart(body, key);
    if (arrayStart == std::string::npos)
    {
        return values;
    }

    const std::size_t arrayEnd = findMatchingArrayEnd(body, arrayStart);
    if (arrayEnd == std::string::npos)
    {
        return values;
    }

    bool insideString = false;
    bool escaped = false;
    std::string current;

    for (std::size_t index = arrayStart + 1; index < arrayEnd; ++index)
    {
        const char character = body[index];

        if (escaped)
        {
            current.push_back(character);
            escaped = false;
            continue;
        }

        if (insideString && character == '\\')
        {
            escaped = true;
            continue;
        }

        if (character == '"')
        {
            if (insideString)
            {
                values.push_back(current);
                current.clear();
            }

            insideString = !insideString;
            continue;
        }

        if (insideString)
        {
            current.push_back(character);
        }
    }

    return values;
}

std::string extractStringField(
    const std::string& object,
    const std::string& key)
{
    const std::string marker = "\"" + key + "\"";
    const std::size_t keyPosition = object.find(marker);

    if (keyPosition == std::string::npos)
    {
        return "";
    }

    const std::size_t colonPosition = object.find(':', keyPosition + marker.size());
    if (colonPosition == std::string::npos)
    {
        return "";
    }

    const std::size_t quoteStart = object.find('"', colonPosition + 1);
    if (quoteStart == std::string::npos)
    {
        return "";
    }

    std::string value;
    bool escaped = false;

    for (std::size_t index = quoteStart + 1; index < object.size(); ++index)
    {
        const char character = object[index];

        if (escaped)
        {
            value.push_back(character);
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
            return value;
        }

        value.push_back(character);
    }

    return "";
}

int extractIntField(
    const std::string& object,
    const std::string& key,
    int defaultValue)
{
    const std::string marker = "\"" + key + "\"";
    const std::size_t keyPosition = object.find(marker);

    if (keyPosition == std::string::npos)
    {
        return defaultValue;
    }

    const std::size_t colonPosition = object.find(':', keyPosition + marker.size());
    if (colonPosition == std::string::npos)
    {
        return defaultValue;
    }

    std::size_t valueStart = object.find_first_not_of(" \t\n\r", colonPosition + 1);
    if (valueStart == std::string::npos)
    {
        return defaultValue;
    }

    if (object[valueStart] == '"')
    {
        ++valueStart;
    }

    return std::atoi(object.c_str() + valueStart);
}

std::vector<std::string> extractObjectArray(
    const std::string& body,
    const std::string& key)
{
    std::vector<std::string> objects;
    const std::size_t arrayStart = findArrayStart(body, key);
    if (arrayStart == std::string::npos)
    {
        return objects;
    }

    const std::size_t arrayEnd = findMatchingArrayEnd(body, arrayStart);
    if (arrayEnd == std::string::npos)
    {
        return objects;
    }

    bool insideString = false;
    bool escaped = false;
    int depth = 0;
    std::size_t objectStart = std::string::npos;

    for (std::size_t index = arrayStart + 1; index < arrayEnd; ++index)
    {
        const char character = body[index];

        if (escaped)
        {
            escaped = false;
            continue;
        }

        if (insideString && character == '\\')
        {
            escaped = true;
            continue;
        }

        if (character == '"')
        {
            insideString = !insideString;
            continue;
        }

        if (insideString)
        {
            continue;
        }

        if (character == '{')
        {
            if (depth == 0)
            {
                objectStart = index;
            }
            ++depth;
        }
        else if (character == '}')
        {
            --depth;
            if (depth == 0 && objectStart != std::string::npos)
            {
                objects.push_back(body.substr(objectStart, index - objectStart + 1));
                objectStart = std::string::npos;
            }
        }
    }

    return objects;
}

void addChannelGroups(
    SearchTimerDiscoveryCatalog& catalog,
    const std::string& body)
{
    std::vector<std::string> groups = extractStringArray(body, "groups");

    if (groups.empty())
    {
        groups = extractStringArray(body, "channelgroups");
    }

    for (const std::string& group : groups)
    {
        catalog.addChannelGroup(
            SearchTimerDiscoveryChannelGroup::create(group));
    }
}

void addRecordingDirectories(
    SearchTimerDiscoveryCatalog& catalog,
    const std::string& body)
{
    for (const std::string& path : extractStringArray(body, "dirs"))
    {
        catalog.addRecordingDirectory(
            SearchTimerDiscoveryRecordingDirectory::create(path));
    }
}

void addBlacklists(
    SearchTimerDiscoveryCatalog& catalog,
    const std::string& body)
{
    for (const std::string& object : extractObjectArray(body, "blacklists"))
    {
        catalog.addBlacklist(
            SearchTimerDiscoveryBlacklist::create(
                extractIntField(object, "id", -1),
                extractStringField(object, "search")));
    }
}

void addExtendedEpgInfo(
    SearchTimerDiscoveryCatalog& catalog,
    const std::string& body)
{
    for (const std::string& object : extractObjectArray(body, "ext_epg_info"))
    {
        catalog.addExtendedEpgInfo(
            SearchTimerDiscoveryExtendedEpgInfo::create(
                extractIntField(object, "id", 0),
                extractStringField(object, "name"),
                extractStringArray(object, "values"),
                extractStringField(object, "config")));
    }
}

void addIfOk(
    const HttpResponse& response,
    void (*add)(SearchTimerDiscoveryCatalog&, const std::string&),
    SearchTimerDiscoveryCatalog& catalog)
{
    if (response.statusCode == 200)
    {
        add(catalog, response.body);
    }
}
}

RestfulApiSearchTimerDiscoveryProvider::RestfulApiSearchTimerDiscoveryProvider(
    std::string configuredBackendId,
    std::string discoveryEndpoint)
    : configuredBackendId_(std::move(configuredBackendId)),
      discoveryEndpoint_(std::move(discoveryEndpoint)),
      basePath_(""),
      httpClient_(nullptr)
{
}

RestfulApiSearchTimerDiscoveryProvider::RestfulApiSearchTimerDiscoveryProvider(
    std::string configuredBackendId,
    IHttpClient& httpClient,
    std::string basePath,
    std::string discoveryEndpoint)
    : configuredBackendId_(std::move(configuredBackendId)),
      discoveryEndpoint_(std::move(discoveryEndpoint)),
      basePath_(std::move(basePath)),
      httpClient_(&httpClient)
{
}

const std::string& RestfulApiSearchTimerDiscoveryProvider::configuredBackendId() const
{
    return configuredBackendId_;
}

const std::string& RestfulApiSearchTimerDiscoveryProvider::discoveryEndpoint() const
{
    return discoveryEndpoint_;
}

const std::string& RestfulApiSearchTimerDiscoveryProvider::basePath() const
{
    return basePath_;
}

SearchTimerDiscoveryCatalog RestfulApiSearchTimerDiscoveryProvider::discover(
    const std::string& backendId) const
{
    SearchTimerDiscoveryCatalog catalog;

    catalog.setBackendId(
        backendId.empty()
            ? configuredBackendId_
            : backendId);

    if (httpClient_ == nullptr)
    {
        return catalog;
    }

    addIfOk(
        getJson(*httpClient_, basePath_, "/searchtimers/channelgroups.json"),
        addChannelGroups,
        catalog);
    addIfOk(
        getJson(*httpClient_, basePath_, "/searchtimers/recordingdirs.json"),
        addRecordingDirectories,
        catalog);
    addIfOk(
        getJson(*httpClient_, basePath_, "/searchtimers/blacklists.json"),
        addBlacklists,
        catalog);
    addIfOk(
        getJson(*httpClient_, basePath_, "/searchtimers/extepginfo.json"),
        addExtendedEpgInfo,
        catalog);

    return catalog;
}

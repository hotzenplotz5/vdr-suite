std::string extractReturnedId(const std::string& body)
{
    const std::string marker = "Id:";
    const std::size_t markerPosition = body.find(marker);

    if (markerPosition == std::string::npos)
    {
        return "";
    }

    std::size_t idStart = markerPosition + marker.size();
    while (idStart < body.size() &&
           std::isspace(static_cast<unsigned char>(body[idStart])))
    {
        ++idStart;
    }

    std::size_t idEnd = idStart;
    while (idEnd < body.size() &&
           std::isdigit(static_cast<unsigned char>(body[idEnd])))
    {
        ++idEnd;
    }

    return body.substr(idStart, idEnd - idStart);
}

std::string extractJsonStringField(
    const std::string& object,
    const std::string& fieldName)
{
    const std::string marker = "\"" + fieldName + "\"";
    const std::size_t field = object.find(marker);

    if (field == std::string::npos)
    {
        return "";
    }

    const std::size_t colon = object.find(':', field + marker.size());
    if (colon == std::string::npos)
    {
        return "";
    }

    std::size_t value = object.find_first_not_of(" \t\n\r", colon + 1);
    if (value == std::string::npos || object[value] != '"')
    {
        return "";
    }

    std::ostringstream out;
    bool escaped = false;

    for (++value; value < object.size(); ++value)
    {
        const char c = object[value];

        if (escaped)
        {
            out << (c == 'n' ? '\n' : c == 'r' ? '\r' : c == 't' ? '\t' : c);
            escaped = false;
            continue;
        }

        if (c == '\\')
        {
            escaped = true;
            continue;
        }

        if (c == '"')
        {
            return out.str();
        }

        out << c;
    }

    return "";
}

std::string extractJsonIntegerField(
    const std::string& object,
    const std::string& fieldName)
{
    const std::string marker = "\"" + fieldName + "\"";
    const std::size_t field = object.find(marker);

    if (field == std::string::npos)
    {
        return "";
    }

    const std::size_t colon = object.find(':', field + marker.size());
    if (colon == std::string::npos)
    {
        return "";
    }

    std::size_t value = object.find_first_not_of(" \t\n\r", colon + 1);
    if (value == std::string::npos)
    {
        return "";
    }

    if (object[value] == '"')
    {
        ++value;
    }

    const std::size_t start = value;
    while (value < object.size() &&
           std::isdigit(static_cast<unsigned char>(object[value])))
    {
        ++value;
    }

    return value == start ? "" : object.substr(start, value - start);
}

bool findNextJsonObject(
    const std::string& body,
    std::size_t& position,
    std::string& object)
{
    const std::size_t start = body.find('{', position);
    if (start == std::string::npos)
    {
        return false;
    }

    int depth = 0;
    bool insideString = false;
    bool escaped = false;

    for (std::size_t index = start; index < body.size(); ++index)
    {
        const char c = body[index];

        if (escaped)
        {
            escaped = false;
            continue;
        }

        if (c == '\\' && insideString)
        {
            escaped = true;
            continue;
        }

        if (c == '"')
        {
            insideString = !insideString;
            continue;
        }

        if (insideString)
        {
            continue;
        }

        if (c == '{')
        {
            ++depth;
        }
        else if (c == '}')
        {
            --depth;
            if (depth == 0)
            {
                object = body.substr(start, index - start + 1);
                position = start + 1;
                return true;
            }
        }
    }

    return false;
}

std::string extractUniqueSearchTimerIdByExactQuery(
    const std::string& body,
    const std::string& query)
{
    std::size_t position = 0;
    std::string object;
    std::string foundId;
    int matches = 0;

    while (findNextJsonObject(body, position, object))
    {
        if (object.find("\"searchtimers\"") != std::string::npos)
        {
            continue;
        }

        if (extractJsonStringField(object, "search") != query)
        {
            continue;
        }

        const std::string id = extractJsonIntegerField(object, "id");
        if (!id.empty())
        {
            foundId = id;
            ++matches;
        }
    }

    return matches == 1 ? foundId : "";
}

std::string readBackCreatedIdByExactQuery(
    IHttpClient& httpClient,
    const std::string& query)
{
    HttpRequest request;
    request.method = "GET";
    request.url = "/searchtimers.json";
    request.headers["Accept"] = "application/json";

    const HttpResponse response = httpClient.execute(request);

    return response.statusCode == 200
        ? extractUniqueSearchTimerIdByExactQuery(response.body, query)
        : "";
}

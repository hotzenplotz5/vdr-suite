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


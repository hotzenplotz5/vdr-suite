std::string trimListValue(const std::string& value)
{
    const std::size_t first =
        value.find_first_not_of(" \t\n\r");

    if (first == std::string::npos)
    {
        return "";
    }

    const std::size_t last =
        value.find_last_not_of(" \t\n\r");

    return value.substr(first, last - first + 1);
}

std::vector<std::string> listValues(
    const std::vector<std::string>& structured,
    const std::string& legacy)
{
    if (!structured.empty())
    {
        return structured;
    }

    std::vector<std::string> result;
    std::string current;

    for (const char character : legacy)
    {
        if (character == ',' || character == ';')
        {
            const std::string item = trimListValue(current);

            if (!item.empty())
            {
                result.push_back(item);
            }

            current.clear();
        }
        else
        {
            current.push_back(character);
        }
    }

    const std::string item = trimListValue(current);

    if (!item.empty())
    {
        result.push_back(item);
    }

    return result;
}

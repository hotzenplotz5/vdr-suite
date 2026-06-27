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
            if (!current.empty())
            {
                result.push_back(current);
            }
            current.clear();
        }
        else if (character != ' ' && character != '\t' && character != '\n' && character != '\r')
        {
            current.push_back(character);
        }
    }

    if (!current.empty())
    {
        result.push_back(current);
    }

    return result;
}

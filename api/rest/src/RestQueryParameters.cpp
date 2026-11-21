#include "RestQueryParameters.h"

#include <cstdlib>
#include <sstream>

namespace
{
int hexValue(char value)
{
    if (value >= '0' && value <= '9')
    {
        return value - '0';
    }

    if (value >= 'A' && value <= 'F')
    {
        return value - 'A' + 10;
    }

    if (value >= 'a' && value <= 'f')
    {
        return value - 'a' + 10;
    }

    return -1;
}

std::string urlDecode(const std::string& value)
{
    std::string decoded;

    for (std::size_t index = 0; index < value.size(); ++index)
    {
        const char current = value.at(index);

        if (current == '+')
        {
            decoded.push_back(' ');
            continue;
        }

        if (current == '%' && index + 2 < value.size())
        {
            const int high = hexValue(value.at(index + 1));
            const int low = hexValue(value.at(index + 2));

            if (high >= 0 && low >= 0)
            {
                decoded.push_back(static_cast<char>((high << 4) | low));
                index += 2;
                continue;
            }
        }

        decoded.push_back(current);
    }

    return decoded;
}
}

RestQueryParameters RestQueryParameters::parse(
    const std::string& queryString)
{
    RestQueryParameters parameters;

    std::istringstream stream(queryString);
    std::string item;

    while (std::getline(stream, item, '&'))
    {
        if (item.empty())
        {
            continue;
        }

        const std::size_t separator = item.find('=');

        if (separator == std::string::npos)
        {
            parameters.values_[urlDecode(item)] = "";
            continue;
        }

        const std::string key = urlDecode(item.substr(0, separator));
        const std::string value = urlDecode(item.substr(separator + 1));

        if (!key.empty())
        {
            parameters.values_[key] = value;
        }
    }

    return parameters;
}

bool RestQueryParameters::has(
    const std::string& key) const
{
    return values_.find(key) != values_.end();
}

std::string RestQueryParameters::get(
    const std::string& key,
    const std::string& defaultValue) const
{
    const auto it = values_.find(key);

    if (it == values_.end())
    {
        return defaultValue;
    }

    return it->second;
}

int RestQueryParameters::getInt(
    const std::string& key,
    int defaultValue) const
{
    const auto it = values_.find(key);

    if (it == values_.end())
    {
        return defaultValue;
    }

    char* end = nullptr;
    const long value = std::strtol(it->second.c_str(), &end, 10);

    if (end == it->second.c_str() || *end != '\0')
    {
        return defaultValue;
    }

    return static_cast<int>(value);
}

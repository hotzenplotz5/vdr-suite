#include "RestQueryParameters.h"

#include <cstdlib>
#include <sstream>

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
            parameters.values_[item] = "";
            continue;
        }

        const std::string key = item.substr(0, separator);
        const std::string value = item.substr(separator + 1);

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

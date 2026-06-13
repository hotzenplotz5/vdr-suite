#pragma once

#include <map>
#include <string>

class RestQueryParameters
{
public:
    static RestQueryParameters parse(
        const std::string& queryString);

    bool has(
        const std::string& key) const;

    std::string get(
        const std::string& key,
        const std::string& defaultValue = "") const;

    int getInt(
        const std::string& key,
        int defaultValue) const;

private:
    std::map<std::string, std::string> values_;
};

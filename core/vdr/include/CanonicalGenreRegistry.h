#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <vector>

class CanonicalGenreRegistry
{
public:
    CanonicalGenreRegistry()
    {
        add("crime", {"crime", "krimi", "kriminalfilm"});
        add("comedy", {"comedy", "komödie", "komoedie"});
        add("action", {"action"});
        add("drama", {"drama"});
        add("documentary", {"documentary", "dokumentation", "doku"});
        add("children", {"children", "kinder", "kinderfilm"});
        add("sports", {"sports", "sport"});
        add("news", {"news", "nachrichten"});
        add("movie", {"movie", "film", "spielfilm"});
        add("series", {"series", "serie"});
    }

    std::string canonicalize(
        const std::string& value) const
    {
        const std::string key = normalize(value);

        const auto found = aliases_.find(key);
        if (found != aliases_.end())
        {
            return found->second;
        }

        return key;
    }

    bool hasCanonicalGenre(
        const std::string& canonicalId) const
    {
        return canonicalIds_.find(normalize(canonicalId)) != canonicalIds_.end();
    }

private:
    void add(
        const std::string& canonicalId,
        const std::vector<std::string>& aliases)
    {
        const std::string normalizedCanonicalId = normalize(canonicalId);

        canonicalIds_[normalizedCanonicalId] = true;
        aliases_[normalizedCanonicalId] = normalizedCanonicalId;

        for (const std::string& alias : aliases)
        {
            aliases_[normalize(alias)] = normalizedCanonicalId;
        }
    }

    std::string normalize(
        const std::string& value) const
    {
        std::string result;

        for (unsigned char character : value)
        {
            if (std::isspace(character) || character == '_' || character == '-')
            {
                if (!result.empty() && result.back() != '-')
                {
                    result.push_back('-');
                }
                continue;
            }

            result.push_back(
                static_cast<char>(
                    std::tolower(character)));
        }

        while (!result.empty() && result.back() == '-')
        {
            result.pop_back();
        }

        return result;
    }

    std::map<std::string, std::string> aliases_;
    std::map<std::string, bool> canonicalIds_;
};

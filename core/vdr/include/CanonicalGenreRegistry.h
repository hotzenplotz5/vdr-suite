#pragma once

#include "GenreClassification.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <string>
#include <vector>

struct CanonicalGenre
{
    std::string id;
    std::string labelDe;
    std::string labelEn;
    bool known = false;
};

class CanonicalGenreRegistry
{
public:
    CanonicalGenreRegistry()
    {
        add("action", "Action", "Action", {"actionfilm"});
        add("adventure", "Abenteuer", "Adventure", {"abenteuerfilm"});
        add("animation", "Animation", "Animation", {"zeichentrick", "zeichentrickfilm", "anime"});
        add("comedy", "Komödie", "Comedy", {"komoedie", "komodie", "lustspiel"});
        add("crime", "Krimi", "Crime", {"kriminalfilm", "crime thriller"});
        add("documentary", "Dokumentation", "Documentary", {"doku", "dokumentarfilm", "documentary film"});
        add("drama", "Drama", "Drama", {});
        add("family", "Familie", "Family", {"familienfilm"});
        add("fantasy", "Fantasy", "Fantasy", {});
        add("history", "Historie", "History", {"historienfilm", "historical", "historical film", "geschichte"});
        add("horror", "Horror", "Horror", {"horrorfilm"});
        add("music", "Musik", "Music", {"musikfilm", "concert", "konzert"});
        add("musical", "Musical", "Musical", {});
        add("mystery", "Mystery", "Mystery", {"mysterie"});
        add("news", "Nachrichten", "News", {"nachricht", "current affairs"});
        add("romance", "Romantik", "Romance", {"romanze", "liebesfilm"});
        add("science-fiction", "Science-Fiction", "Science Fiction", {"science fiction", "sci fi", "sci-fi", "scifi", "sf"});
        add("series", "Serie", "Series", {"serien", "tv series", "television series"});
        add("sports", "Sport", "Sports", {"sporting event"});
        add("talk-show", "Talkshow", "Talk Show", {"talk show", "talkshow", "gesprächssendung", "gespraechssendung"});
        add("reality", "Reality", "Reality", {"reality tv", "reality-tv", "realityshow", "reality show"});
        add("thriller", "Thriller", "Thriller", {"suspense"});
        add("war", "Krieg", "War", {"kriegsfilm", "war film"});
        add("western", "Western", "Western", {});
        add("disaster", "Katastrophenfilm", "Disaster", {"katastrophe", "disaster film"});
        add("children", "Kinder", "Children", {"kinderfilm", "kids"});
        add("movie", "Spielfilm", "Movie", {"film", "feature film"});
        add("unclassified", "Nicht klassifiziert", "Unclassified", {"ohne genre"});
    }

    CanonicalGenre classify(const std::string& providerValue) const
    {
        const std::string normalized = normalizeAlias(providerValue);
        if (normalized.empty())
        {
            return find("unclassified");
        }

        const auto alias = aliases_.find(normalized);
        if (alias != aliases_.end())
        {
            return find(alias->second);
        }

        CanonicalGenre unknown;
        unknown.id = stableUnknownId(normalized);
        unknown.labelDe = trim(providerValue).empty() ? unknown.id : trim(providerValue);
        unknown.labelEn = unknown.labelDe;
        unknown.known = false;
        return unknown;
    }

    CanonicalGenre find(const std::string& id) const
    {
        const auto found = genres_.find(id);
        if (found != genres_.end())
        {
            return found->second;
        }

        CanonicalGenre unknown;
        unknown.id = stableUnknownId(normalizeAlias(id));
        unknown.labelDe = id;
        unknown.labelEn = id;
        unknown.known = false;
        return unknown;
    }

    bool isKnown(const std::string& id) const
    {
        const auto found = genres_.find(id);
        return found != genres_.end() && found->second.known;
    }

    std::vector<CanonicalGenre> allKnown() const
    {
        std::vector<CanonicalGenre> result;
        for (const auto& entry : genres_)
        {
            result.push_back(entry.second);
        }
        std::sort(result.begin(), result.end(), [](const CanonicalGenre& left, const CanonicalGenre& right) {
            if (left.id == "unclassified") return false;
            if (right.id == "unclassified") return true;
            return left.labelDe < right.labelDe;
        });
        return result;
    }

    static std::string normalizeAlias(const std::string& value)
    {
        std::string folded;
        folded.reserve(value.size() + 8);

        for (std::size_t index = 0; index < value.size();)
        {
            const unsigned char current = static_cast<unsigned char>(value[index]);
            if (current == 0xc3 && index + 1 < value.size())
            {
                const unsigned char next = static_cast<unsigned char>(value[index + 1]);
                switch (next)
                {
                case 0x84: case 0xa4: folded += "ae"; index += 2; continue;
                case 0x96: case 0xb6: folded += "oe"; index += 2; continue;
                case 0x9c: case 0xbc: folded += "ue"; index += 2; continue;
                case 0x9f: folded += "ss"; index += 2; continue;
                default: break;
                }
            }

            if (current < 0x80)
            {
                if (std::isalnum(current))
                {
                    folded.push_back(static_cast<char>(std::tolower(current)));
                }
                else
                {
                    folded.push_back(' ');
                }
                ++index;
                continue;
            }

            folded.push_back(' ');
            ++index;
        }

        std::string normalized;
        bool separator = true;
        for (const char character : folded)
        {
            if (character == ' ')
            {
                if (!separator && !normalized.empty())
                {
                    normalized.push_back(' ');
                }
                separator = true;
                continue;
            }
            normalized.push_back(character);
            separator = false;
        }
        if (!normalized.empty() && normalized.back() == ' ')
        {
            normalized.pop_back();
        }
        return normalized;
    }

    static std::string stableUnknownId(const std::string& normalizedValue)
    {
        std::string slug;
        bool dash = false;
        for (const char character : normalizedValue)
        {
            const unsigned char value = static_cast<unsigned char>(character);
            if (std::isalnum(value))
            {
                slug.push_back(static_cast<char>(std::tolower(value)));
                dash = false;
            }
            else if (!slug.empty() && !dash)
            {
                slug.push_back('-');
                dash = true;
            }
        }
        while (!slug.empty() && slug.back() == '-') slug.pop_back();
        if (slug.empty()) slug = "unknown";
        if (slug.size() > 48) slug.resize(48);
        return "unknown-" + slug;
    }

private:
    std::map<std::string, CanonicalGenre> genres_;
    std::map<std::string, std::string> aliases_;

    static std::string trim(const std::string& value)
    {
        const std::size_t first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return {};
        const std::size_t last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    void add(
        const std::string& id,
        const std::string& labelDe,
        const std::string& labelEn,
        const std::vector<std::string>& aliases)
    {
        CanonicalGenre genre{id, labelDe, labelEn, true};
        genres_[id] = genre;
        aliases_[normalizeAlias(id)] = id;
        aliases_[normalizeAlias(labelDe)] = id;
        aliases_[normalizeAlias(labelEn)] = id;
        for (const std::string& alias : aliases)
        {
            aliases_[normalizeAlias(alias)] = id;
        }
    }
};

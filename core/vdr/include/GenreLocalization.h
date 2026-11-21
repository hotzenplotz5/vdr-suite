#pragma once

#include <map>
#include <string>

class GenreLocalization
{
public:
    GenreLocalization()
    {
        add("crime", "de", "Krimi");
        add("crime", "en", "Crime");

        add("comedy", "de", "Komödie");
        add("comedy", "en", "Comedy");

        add("action", "de", "Action");
        add("action", "en", "Action");

        add("drama", "de", "Drama");
        add("drama", "en", "Drama");

        add("documentary", "de", "Dokumentation");
        add("documentary", "en", "Documentary");

        add("children", "de", "Kinder");
        add("children", "en", "Children");

        add("sports", "de", "Sport");
        add("sports", "en", "Sports");

        add("news", "de", "Nachrichten");
        add("news", "en", "News");

        add("movie", "de", "Spielfilm");
        add("movie", "en", "Movie");

        add("series", "de", "Serie");
        add("series", "en", "Series");
    }

    std::string label(
        const std::string& canonicalId,
        const std::string& locale) const
    {
        const std::string normalizedLocale = normalizeLocale(locale);

        const std::string localeKey =
            canonicalId + ":" + normalizedLocale;

        const auto localeLabel = labels_.find(localeKey);
        if (localeLabel != labels_.end())
        {
            return localeLabel->second;
        }

        const std::string englishKey =
            canonicalId + ":en";

        const auto englishLabel = labels_.find(englishKey);
        if (englishLabel != labels_.end())
        {
            return englishLabel->second;
        }

        return canonicalId;
    }

private:
    void add(
        const std::string& canonicalId,
        const std::string& locale,
        const std::string& label)
    {
        labels_[canonicalId + ":" + normalizeLocale(locale)] = label;
    }

    std::string normalizeLocale(
        const std::string& locale) const
    {
        if (locale.size() >= 2)
        {
            return locale.substr(0, 2);
        }

        return "en";
    }

    std::map<std::string, std::string> labels_;
};

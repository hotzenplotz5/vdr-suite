#include "EpgSearchMatcher.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::string lowerCopy(
    const std::string& value)
{
    std::string lowered = value;

    std::transform(
        lowered.begin(),
        lowered.end(),
        lowered.begin(),
        [](unsigned char character) {
            return static_cast<char>(
                std::tolower(character));
        });

    return lowered;
}

std::string comparableText(
    const std::string& value,
    bool matchCase)
{
    return matchCase ? value : lowerCopy(value);
}

bool containsText(
    const std::string& haystack,
    const std::string& needle,
    bool matchCase)
{
    if (needle.empty())
    {
        return true;
    }

    return comparableText(haystack, matchCase).find(
        comparableText(needle, matchCase))
        != std::string::npos;
}

bool equalsText(
    const std::string& haystack,
    const std::string& needle,
    bool matchCase)
{
    return comparableText(haystack, matchCase)
        == comparableText(needle, matchCase);
}

std::vector<std::string> splitWords(
    const std::string& value)
{
    std::vector<std::string> words;
    std::stringstream stream(value);
    std::string word;

    while (stream >> word)
    {
        words.push_back(word);
    }

    return words;
}

std::vector<std::string> searchableFields(
    const VdrEvent& event,
    const EpgSearchQuery& query)
{
    std::vector<std::string> fields;

    if (!query.hasFieldSelection())
    {
        fields.push_back(event.title);
        fields.push_back(event.subtitle);
        fields.push_back(event.description);
        return fields;
    }

    if (query.useTitle())
    {
        fields.push_back(event.title);
    }

    if (query.useSubtitle())
    {
        fields.push_back(event.subtitle);
    }

    if (query.useDescription())
    {
        fields.push_back(event.description);
    }

    return fields;
}

std::string joinedSearchableText(
    const VdrEvent& event,
    const EpgSearchQuery& query)
{
    const std::vector<std::string> fields =
        searchableFields(event, query);

    std::string joined;

    for (const std::string& field : fields)
    {
        if (!joined.empty())
        {
            joined += " ";
        }

        joined += field;
    }

    return joined;
}

bool matchesPhraseText(
    const VdrEvent& event,
    const EpgSearchQuery& query,
    bool matchCase)
{
    const std::vector<std::string> fields =
        searchableFields(event, query);

    for (const std::string& field : fields)
    {
        if (containsText(field, query.text(), matchCase))
        {
            return true;
        }
    }

    return false;
}

bool matchesExactText(
    const VdrEvent& event,
    const EpgSearchQuery& query,
    bool matchCase)
{
    const std::vector<std::string> fields =
        searchableFields(event, query);

    for (const std::string& field : fields)
    {
        if (equalsText(field, query.text(), matchCase))
        {
            return true;
        }
    }

    return false;
}

bool matchesAllWordsText(
    const VdrEvent& event,
    const EpgSearchQuery& query,
    bool matchCase)
{
    const std::vector<std::string> words =
        splitWords(query.text());

    if (words.empty())
    {
        return false;
    }

    const std::string joined =
        joinedSearchableText(event, query);

    for (const std::string& word : words)
    {
        if (!containsText(joined, word, matchCase))
        {
            return false;
        }
    }

    return true;
}

bool matchesAnyWordText(
    const VdrEvent& event,
    const EpgSearchQuery& query,
    bool matchCase)
{
    const std::vector<std::string> words =
        splitWords(query.text());

    if (words.empty())
    {
        return false;
    }

    const std::string joined =
        joinedSearchableText(event, query);

    for (const std::string& word : words)
    {
        if (containsText(joined, word, matchCase))
        {
            return true;
        }
    }

    return false;
}

bool matchesText(
    const VdrEvent& event,
    const EpgSearchQuery& query)
{
    if (!query.hasText())
    {
        return true;
    }

    const bool matchCase =
        query.hasMatchCase() && query.matchCase();

    if (!query.hasMode())
    {
        return matchesPhraseText(event, query, matchCase);
    }

    switch (query.mode())
    {
    case EpgSearchMode::Exact:
        return matchesExactText(event, query, matchCase);
    case EpgSearchMode::AllWords:
        return matchesAllWordsText(event, query, matchCase);
    case EpgSearchMode::AnyWord:
        return matchesAnyWordText(event, query, matchCase);
    case EpgSearchMode::Phrase:
    case EpgSearchMode::RegularExpression:
    case EpgSearchMode::Fuzzy:
        return matchesPhraseText(event, query, matchCase);
    }

    return matchesPhraseText(event, query, matchCase);
}

bool matchesChannel(
    const VdrEvent& event,
    const EpgSearchQuery& query)
{
    if (!query.hasChannelScope())
    {
        return true;
    }

    if (query.channelScope() == EpgSearchChannelScope::Interval)
    {
        return event.channelId >= query.channelMin()
            && event.channelId <= query.channelMax();
    }

    return true;
}

bool matchesDuration(
    const VdrEvent& event,
    const EpgSearchQuery& query)
{
    if (!query.hasDurationWindow())
    {
        return true;
    }

    const int durationMinutes =
        event.durationSeconds / 60;

    return durationMinutes >= query.durationMinMinutes()
        && durationMinutes <= query.durationMaxMinutes();
}

std::vector<std::string> splitCommaSeparated(
    const std::string& value)
{
    std::vector<std::string> result;
    std::stringstream stream(value);
    std::string item;

    while (std::getline(stream, item, ','))
    {
        if (!item.empty())
        {
            result.push_back(item);
        }
    }

    return result;
}

bool matchesContentDescriptors(
    const VdrEvent& event,
    const EpgSearchQuery& query)
{
    if (!query.hasContentDescriptors())
    {
        return true;
    }

    const std::vector<std::string> expected =
        splitCommaSeparated(query.contentDescriptors());

    for (const std::string& descriptor : expected)
    {
        const bool found =
            std::find(
                event.contentDescriptors.begin(),
                event.contentDescriptors.end(),
                descriptor)
            != event.contentDescriptors.end();

        if (!found)
        {
            return false;
        }
    }

    return true;
}
}

bool EpgSearchMatcher::matches(
    const VdrEvent& event,
    const EpgSearchQuery& query) const
{
    return matchesText(event, query)
        && matchesChannel(event, query)
        && matchesDuration(event, query)
        && matchesContentDescriptors(event, query);
}

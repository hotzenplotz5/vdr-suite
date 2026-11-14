#pragma once

// MT.META7.2: general-purpose, read-only title evidence for the existing
// Public-v1 MetadataController. Never assumes an identity from a query alone.
// No title, film, franchise or provider-ID specific exceptions.
#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <vector>

namespace recordingMetadataTitleMatch
{
struct Word
{
    std::string value;
    std::size_t start = 0;
    std::size_t end = 0;
};

// TMDb commonly uses Unicode en/em dashes and typographic quotation
// marks. Treat punctuation as separators, but preserve actual UTF-8 letters.
// Latin-1 uppercase letters are folded to the corresponding lowercase form.
inline bool separator(unsigned codepoint)
{
    if (codepoint < 128U)
        return std::isalnum(static_cast<unsigned char>(codepoint)) == 0;
    return (codepoint >= 0x2000U && codepoint <= 0x206fU) ||
        (codepoint >= 0x3000U && codepoint <= 0x303fU) ||
        (codepoint >= 0xff00U && codepoint <= 0xff65U) ||
        (codepoint >= 0x00a0U && codepoint <= 0x00bfU) ||
        codepoint == 0x00d7U || codepoint == 0x00f7U;
}

inline std::vector<Word> words(const std::string& title)
{
    std::vector<Word> result;
    std::string token;
    std::size_t start = 0U;
    for (std::size_t i = 0; i < title.size();)
    {
        const unsigned char first = static_cast<unsigned char>(title[i]);
        unsigned cp = first;
        std::size_t width = 1U;
        if (first >= 0xc2U && first <= 0xdfU) width = 2U;
        else if (first >= 0xe0U && first <= 0xefU) width = 3U;
        else if (first >= 0xf0U && first <= 0xf4U) width = 4U;
        if (width > 1U && i + width <= title.size())
        {
            bool valid = true;
            for (std::size_t j = 1U; j < width; ++j)
                valid = valid &&
                    (static_cast<unsigned char>(title[i + j]) & 0xc0U) == 0x80U;
            if (valid)
            {
                cp = first & (width == 2U ? 0x1fU :
                    width == 3U ? 0x0fU : 0x07U);
                for (std::size_t j = 1U; j < width; ++j)
                    cp = (cp << 6U) |
                        (static_cast<unsigned char>(title[i + j]) & 0x3fU);
            }
            else width = 1U;
        }
        else width = 1U;

        if (separator(cp))
        {
            if (!token.empty())
            {
                result.push_back({token, start, i});
                token.clear();
            }
        }
        else
        {
            if (token.empty()) start = i;
            if (cp >= 0x00c0U && cp <= 0x00d6U)
                cp += 0x20U;
            else if (cp >= 0x00d8U && cp <= 0x00deU)
                cp += 0x20U;
            if (cp < 128U)
                token.push_back(static_cast<char>(
                    std::tolower(static_cast<unsigned char>(cp))));
            else if (cp < 0x800U)
            {
                token.push_back(static_cast<char>(0xc0U | (cp >> 6U)));
                token.push_back(static_cast<char>(0x80U | (cp & 0x3fU)));
            }
            else if (cp < 0x10000U)
            {
                token.push_back(static_cast<char>(0xe0U | (cp >> 12U)));
                token.push_back(static_cast<char>(0x80U | ((cp >> 6U) & 0x3fU)));
                token.push_back(static_cast<char>(0x80U | (cp & 0x3fU)));
            }
            else token.append(title, i, width);
        }
        i += width;
    }
    if (!token.empty()) result.push_back({token, start, title.size()});
    return result;
}

inline bool partWord(const std::string& token)
{
    static const std::set<std::string> values{
        "part", "pt", "teil", "deel", "parte", "partie", "chapter",
        "kapitel", "volume", "vol", "tome", "band"
    };
    return values.count(token) != 0U;
}

inline bool articleWord(const std::string& token)
{
    static const std::set<std::string> values{
        "a", "an", "the", "der", "die", "das", "den", "dem", "des",
        "ein", "eine", "einer", "eines", "und", "and", "of", "von",
        "de", "del", "het", "een", "el", "la", "le", "les", "los",
        "las", "un", "une", "et", "y", "en"
    };
    return values.count(token) != 0U;
}

inline std::string number(const std::string& token)
{
    static const std::vector<std::vector<std::string>> names{
        {"1", "one", "eins", "ein", "uno", "un", "une", "een", "i"},
        {"2", "two", "zwei", "twee", "dos", "deux", "due", "ii"},
        {"3", "three", "drei", "drie", "tres", "trois", "iii"},
        {"4", "four", "vier", "quatre", "cuatro", "iv"},
        {"5", "five", "fuenf", "fünf", "cinco", "cinq", "v"},
        {"6", "six", "sechs", "seis", "vi"},
        {"7", "seven", "sieben", "siete", "vii"},
        {"8", "eight", "acht", "ocho", "viii"},
        {"9", "nine", "neun", "nueve", "ix"},
        {"10", "ten", "zehn", "diez", "x"}
    };
    for (std::size_t i = 0; i < names.size(); ++i)
        if (std::find(names[i].begin(), names[i].end(), token) !=
            names[i].end())
            return std::to_string(i + 1U);
    return token;
}

inline std::vector<std::string> canonicalWords(const std::string& title)
{
    const auto parts = words(title);
    std::vector<std::string> result;
    for (std::size_t i = 0; i < parts.size(); ++i)
    {
        const std::string& token = parts[i].value;
        // "Teil 2", "Deel 2", "Part Two" and "Chapter II" preserve
        // their numbered identity. Missing/different sequel numbers cannot
        // accidentally match a different film in the same franchise.
        if (partWord(token) && i + 1U < parts.size())
        {
            const std::string partNumber = number(parts[i + 1U].value);
            if (partNumber != parts[i + 1U].value ||
                (!partNumber.empty() &&
                 std::all_of(partNumber.begin(), partNumber.end(),
                    [](unsigned char c) { return std::isdigit(c) != 0; })))
            {
                result.push_back(partNumber);
                ++i;
                continue;
            }
        }
        if (partWord(token) || articleWord(token)) continue;
        result.push_back(number(token));
    }
    return result;
}

inline std::string trimmed(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n._:-");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n._:-");
    return value.substr(first, last - first + 1U);
}

inline int score(const std::string& supplied, const std::string& candidate)
{
    const auto expected = canonicalWords(supplied);
    const auto actual = canonicalWords(candidate);
    if (expected.empty() || actual.empty()) return 0;
    if (expected == actual) return 120;
    const std::set<std::string> left(expected.begin(), expected.end());
    const std::set<std::string> right(actual.begin(), actual.end());
    if (left == right) return 115;

    // Series frequently carry a franchise prefix separated by a colon
    // (e.g. "Franchise: Series"). A complete suffix may be an alias.
    // Single-token generic prefixes are NOT accepted as evidence.
    const auto colon = candidate.find_last_of(':');
    if (colon != std::string::npos &&
        canonicalWords(candidate.substr(colon + 1U)) == expected)
        return 105;
    return 0;
}

inline std::vector<std::string> searchQueries(const std::string& title)
{
    std::vector<std::string> queries;
    const auto append = [&](const std::string& text) {
        const std::string value = trimmed(text);
        if (value.size() < 2U || value.size() > 160U ||
            std::find(queries.begin(), queries.end(), value) != queries.end() ||
            queries.size() >= 4U) return;
        queries.push_back(value);
    };
    append(title);

    const auto tokens = words(title);
    for (std::size_t i = 1; i + 1U < tokens.size(); ++i)
    {
        if (!partWord(tokens[i].value)) continue;
        const std::string partNumber = number(tokens[i + 1U].value);
        if (!std::all_of(partNumber.begin(), partNumber.end(),
            [](unsigned char c) { return std::isdigit(c) != 0; })) continue;
        append(title.substr(0U, tokens[i].start));
        append(title.substr(tokens[i + 1U].end));
        break;
    }
    // Generic franchise/series suffix: "Star Wars-Andor" -> "Andor".
    // A single hyphen inside a word is left intact unless a meaningful
    // multiword prefix precedes it.
    for (std::size_t i = 0; i < title.size(); ++i)
    {
        if (title[i] != ':' && title[i] != '-') continue;
        if (words(title.substr(0U, i)).size() < 2U) continue;
        append(title.substr(i + 1U));
        break;
    }
    return queries;
}
} // namespace recordingMetadataTitleMatch

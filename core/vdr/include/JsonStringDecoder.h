#pragma once

#include <cstddef>
#include <string>

namespace vdrsuite
{

inline int jsonHexValue(
    const char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }

    if (character >= 'a' && character <= 'f')
    {
        return 10 + character - 'a';
    }

    if (character >= 'A' && character <= 'F')
    {
        return 10 + character - 'A';
    }

    return -1;
}

inline void appendUtf8CodePoint(
    std::string& value,
    const unsigned int codePoint)
{
    if (codePoint <= 0x7F)
    {
        value.push_back(static_cast<char>(codePoint));
    }
    else if (codePoint <= 0x7FF)
    {
        value.push_back(static_cast<char>(0xC0 | ((codePoint >> 6) & 0x1F)));
        value.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
    else if (codePoint <= 0xFFFF)
    {
        value.push_back(static_cast<char>(0xE0 | ((codePoint >> 12) & 0x0F)));
        value.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        value.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
    else
    {
        value.push_back(static_cast<char>(0xF0 | ((codePoint >> 18) & 0x07)));
        value.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
        value.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        value.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

inline bool appendUnicodeEscape(
    const std::string& source,
    std::size_t& index,
    std::string& value)
{
    if (index + 4 >= source.size())
    {
        return false;
    }

    unsigned int codePoint = 0;

    for (std::size_t offset = 1; offset <= 4; ++offset)
    {
        const int digit = jsonHexValue(source[index + offset]);

        if (digit < 0)
        {
            return false;
        }

        codePoint = (codePoint << 4) | static_cast<unsigned int>(digit);
    }

    appendUtf8CodePoint(value, codePoint);
    index += 4;
    return true;
}

inline void appendEscapedJsonCharacter(
    const std::string& source,
    std::size_t& index,
    std::string& value)
{
    const char character = source[index];

    switch (character)
    {
        case '"':
            value.push_back('"');
            break;
        case '\\':
            value.push_back('\\');
            break;
        case '/':
            value.push_back('/');
            break;
        case 'b':
            value.push_back('\b');
            break;
        case 'f':
            value.push_back('\f');
            break;
        case 'n':
            value.push_back('\n');
            break;
        case 'r':
            value.push_back('\r');
            break;
        case 't':
            value.push_back('\t');
            break;
        case 'u':
            if (!appendUnicodeEscape(source, index, value))
            {
                value.push_back(character);
            }
            break;
        default:
            value.push_back(character);
            break;
    }
}

inline std::string decodeJsonStringEscapes(
    const std::string& value)
{
    std::string decoded;
    bool escaped = false;

    for (std::size_t index = 0; index < value.size(); ++index)
    {
        const char character = value[index];

        if (escaped)
        {
            appendEscapedJsonCharacter(value, index, decoded);
            escaped = false;
            continue;
        }

        if (character == '\\')
        {
            escaped = true;
            continue;
        }

        decoded.push_back(character);
    }

    if (escaped)
    {
        decoded.push_back('\\');
    }

    return decoded;
}

}

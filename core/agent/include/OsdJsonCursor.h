#pragma once
#include <cstdint>
#include <limits>
#include <string>
namespace vdrsuite::agent::osd_json
{
inline bool validUtf8(const std::string& value)
{
    std::size_t index = 0;
    while (index < value.size())
    {
        const unsigned char first = static_cast<unsigned char>(value[index]);
        if (first <= 0x7fU)
        {
            ++index;
            continue;
        }
        std::size_t count = 0;
        std::uint32_t codePoint = 0;
        if ((first & 0xe0U) == 0xc0U) { count = 2; codePoint = first & 0x1fU; }
        else if ((first & 0xf0U) == 0xe0U) { count = 3; codePoint = first & 0x0fU; }
        else if ((first & 0xf8U) == 0xf0U) { count = 4; codePoint = first & 0x07U; }
        else return false;
        if (index + count > value.size()) return false;
        for (std::size_t offset = 1; offset < count; ++offset)
        {
            const unsigned char next = static_cast<unsigned char>(value[index + offset]);
            if ((next & 0xc0U) != 0x80U) return false;
            codePoint = (codePoint << 6U) | (next & 0x3fU);
        }
        if ((count == 2 && codePoint < 0x80U) ||
            (count == 3 && codePoint < 0x800U) ||
            (count == 4 && codePoint < 0x10000U) ||
            codePoint > 0x10ffffU ||
            (codePoint >= 0xd800U && codePoint <= 0xdfffU))
        {
            return false;
        }
        index += count;
    }
    return true;
}

class Cursor
{
public:
    explicit Cursor(const std::string& text)
        : text_(text)
    {
    }

    bool consume(char expected)
    {
        skipWhitespace();
        if (position_ >= text_.size() || text_[position_] != expected)
        {
            return fail(std::string("expected '") + expected + "'");
        }
        ++position_;
        return true;
    }

    bool consumeIf(char expected)
    {
        skipWhitespace();
        if (position_ < text_.size() && text_[position_] == expected)
        {
            ++position_;
            return true;
        }
        return false;
    }

    bool key(const char* expected)
    {
        std::string actual;
        return string(actual, 64) &&
            actual == expected &&
            consume(':');
    }

    bool string(std::string& output, std::size_t maximumBytes)
    {
        skipWhitespace();
        if (position_ >= text_.size() || text_[position_] != '"')
        {
            return fail("expected JSON string");
        }

        ++position_;
        output.clear();

        while (position_ < text_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(text_[position_++]);

            if (character == '"')
            {
                return (output.size() <= maximumBytes && validUtf8(output)) ||
                    fail("invalid or oversized JSON string");
            }

            if (character < 0x20)
            {
                return fail("control character in JSON string");
            }

            if (character != '\\')
            {
                output.push_back(static_cast<char>(character));
            }
            else
            {
                if (position_ >= text_.size())
                {
                    return fail("unterminated JSON escape");
                }

                const char escaped = text_[position_++];
                switch (escaped)
                {
                    case '"':
                    case '\\':
                    case '/':
                        output.push_back(escaped);
                        break;
                    case 'b':
                        output.push_back('\b');
                        break;
                    case 'f':
                        output.push_back('\f');
                        break;
                    case 'n':
                        output.push_back('\n');
                        break;
                    case 'r':
                        output.push_back('\r');
                        break;
                    case 't':
                        output.push_back('\t');
                        break;
                    case 'u':
                    {
                        std::uint32_t codePoint = 0;
                        for (int index = 0; index < 4; ++index)
                        {
                            if (position_ >= text_.size())
                            {
                                return fail("incomplete Unicode escape");
                            }
                            const int value = hexValue(text_[position_++]);
                            if (value < 0)
                            {
                                return fail("invalid Unicode escape");
                            }
                            codePoint =
                                (codePoint << 4) |
                                static_cast<std::uint32_t>(value);
                        }

                        if (codePoint >= 0xD800 && codePoint <= 0xDFFF)
                        {
                            return fail("surrogate Unicode escape unsupported");
                        }

                        appendUtf8(output, codePoint);
                        break;
                    }
                    default:
                        return fail("invalid JSON escape");
                }
            }

            if (output.size() > maximumBytes)
            {
                return fail("JSON string exceeds bounded size");
            }
        }

        return fail("unterminated JSON string");
    }

    bool boolean(bool& output)
    {
        skipWhitespace();
        if (literal("true"))
        {
            output = true;
            return true;
        }
        if (literal("false"))
        {
            output = false;
            return true;
        }
        return fail("expected boolean");
    }

    bool unsignedInteger(std::uint64_t& output)
    {
        skipWhitespace();
        if (position_ >= text_.size() ||
            text_[position_] < '0' ||
            text_[position_] > '9')
        {
            return fail("expected unsigned integer");
        }

        if (text_[position_] == '0' &&
            position_ + 1 < text_.size() &&
            text_[position_ + 1] >= '0' &&
            text_[position_ + 1] <= '9')
        {
            return fail("leading zero in integer");
        }

        std::uint64_t value = 0;
        while (position_ < text_.size() &&
               text_[position_] >= '0' &&
               text_[position_] <= '9')
        {
            const std::uint64_t digit =
                static_cast<std::uint64_t>(text_[position_] - '0');
            if (value >
                (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
            {
                return fail("unsigned integer overflow");
            }
            value = value * 10 + digit;
            ++position_;
        }

        output = value;
        return true;
    }

    bool signedInteger(std::int64_t& output)
    {
        skipWhitespace();
        bool negative = false;
        if (position_ < text_.size() && text_[position_] == '-')
        {
            negative = true;
            ++position_;
        }

        std::uint64_t magnitude = 0;
        if (!unsignedInteger(magnitude))
        {
            return false;
        }

        const std::uint64_t positiveMaximum =
            static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max());
        const std::uint64_t negativeMaximum = positiveMaximum + 1U;

        if ((!negative && magnitude > positiveMaximum) ||
            (negative && magnitude > negativeMaximum))
        {
            return fail("signed integer overflow");
        }

        if (!negative)
        {
            output = static_cast<std::int64_t>(magnitude);
        }
        else if (magnitude == negativeMaximum)
        {
            output = std::numeric_limits<std::int64_t>::min();
        }
        else
        {
            output = -static_cast<std::int64_t>(magnitude);
        }

        return true;
    }

    bool finished()
    {
        skipWhitespace();
        return position_ == text_.size();
    }

    const std::string& error() const
    {
        return error_;
    }

private:
    void skipWhitespace()
    {
        while (position_ < text_.size())
        {
            const char character = text_[position_];
            if (character != ' ' &&
                character != '\t' &&
                character != '\r' &&
                character != '\n')
            {
                break;
            }
            ++position_;
        }
    }

    bool literal(const char* value)
    {
        const std::size_t start = position_;
        for (; *value != '\0'; ++value, ++position_)
        {
            if (position_ >= text_.size() ||
                text_[position_] != *value)
            {
                position_ = start;
                return false;
            }
        }
        return true;
    }

    bool fail(const std::string& message)
    {
        if (error_.empty())
        {
            error_ = message;
        }
        return false;
    }

    static int hexValue(char character)
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

    static void appendUtf8(
        std::string& output,
        std::uint32_t codePoint)
    {
        if (codePoint <= 0x7F)
        {
            output.push_back(static_cast<char>(codePoint));
        }
        else if (codePoint <= 0x7FF)
        {
            output.push_back(
                static_cast<char>(0xC0 | (codePoint >> 6)));
            output.push_back(
                static_cast<char>(0x80 | (codePoint & 0x3F)));
        }
        else
        {
            output.push_back(
                static_cast<char>(0xE0 | (codePoint >> 12)));
            output.push_back(
                static_cast<char>(
                    0x80 | ((codePoint >> 6) & 0x3F)));
            output.push_back(
                static_cast<char>(0x80 | (codePoint & 0x3F)));
        }
    }

    const std::string& text_;
    std::size_t position_ = 0;
    std::string error_;
};

}

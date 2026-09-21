#include "TvmazeSeriesArtworkJson.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr int MaximumDepth = 20;

class Value
{
public:
    enum class Type { Null, Boolean, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolean = false;
    double number = 0.0;
    std::string string;
    std::vector<Value> array;
    std::map<std::string, Value> object;

    const Value* member(const std::string& name) const
    {
        if (type != Type::Object) return nullptr;
        const auto iterator = object.find(name);
        return iterator == object.end() ? nullptr : &iterator->second;
    }
};

class Parser
{
public:
    Parser(const std::string& text, std::size_t maximumBytes)
        : text_(text), maximumBytes_(maximumBytes) {}

    bool parse(Value& value)
    {
        if (text_.empty() || text_.size() > maximumBytes_) return false;
        whitespace();
        if (!valueAt(value, 0)) return false;
        whitespace();
        return position_ == text_.size();
    }

private:
    const std::string& text_;
    std::size_t maximumBytes_;
    std::size_t position_ = 0;

    void whitespace()
    {
        while (position_ < text_.size() &&
               (text_[position_] == ' ' || text_[position_] == '\t' ||
                text_[position_] == '\r' || text_[position_] == '\n'))
        {
            ++position_;
        }
    }

    bool valueAt(Value& value, int depth)
    {
        if (depth > MaximumDepth || position_ >= text_.size()) return false;
        if (text_[position_] == '{') return object(value, depth + 1);
        if (text_[position_] == '[') return array(value, depth + 1);
        if (text_[position_] == '"')
        {
            value.type = Value::Type::String;
            return string(value.string);
        }
        if (literal("true"))
        {
            value.type = Value::Type::Boolean;
            value.boolean = true;
            return true;
        }
        if (literal("false"))
        {
            value.type = Value::Type::Boolean;
            return true;
        }
        if (literal("null"))
        {
            value.type = Value::Type::Null;
            return true;
        }
        return number(value);
    }

    bool object(Value& value, int depth)
    {
        if (text_[position_++] != '{') return false;
        value.type = Value::Type::Object;
        whitespace();
        if (position_ < text_.size() && text_[position_] == '}')
        {
            ++position_;
            return true;
        }
        while (position_ < text_.size())
        {
            std::string name;
            if (!string(name)) return false;
            whitespace();
            if (position_ >= text_.size() || text_[position_++] != ':')
                return false;
            whitespace();
            Value member;
            if (!valueAt(member, depth)) return false;
            if (!value.object.emplace(std::move(name), std::move(member)).second)
                return false;
            whitespace();
            if (position_ >= text_.size()) return false;
            if (text_[position_] == '}')
            {
                ++position_;
                return true;
            }
            if (text_[position_++] != ',') return false;
            whitespace();
        }
        return false;
    }

    bool array(Value& value, int depth)
    {
        if (text_[position_++] != '[') return false;
        value.type = Value::Type::Array;
        whitespace();
        if (position_ < text_.size() && text_[position_] == ']')
        {
            ++position_;
            return true;
        }
        while (position_ < text_.size())
        {
            if (value.array.size() >= 2048U) return false;
            Value item;
            if (!valueAt(item, depth)) return false;
            value.array.push_back(std::move(item));
            whitespace();
            if (position_ >= text_.size()) return false;
            if (text_[position_] == ']')
            {
                ++position_;
                return true;
            }
            if (text_[position_++] != ',') return false;
            whitespace();
        }
        return false;
    }

    bool string(std::string& output)
    {
        output.clear();
        if (position_ >= text_.size() || text_[position_++] != '"')
            return false;
        while (position_ < text_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(text_[position_++]);
            if (character == '"') return true;
            if (character < 0x20U) return false;
            if (character != '\\')
            {
                output.push_back(static_cast<char>(character));
            }
            else
            {
                if (position_ >= text_.size()) return false;
                const char escaped = text_[position_++];
                if (escaped == '"' || escaped == '\\' || escaped == '/')
                    output.push_back(escaped);
                else if (escaped == 'b') output.push_back('\b');
                else if (escaped == 'f') output.push_back('\f');
                else if (escaped == 'n') output.push_back('\n');
                else if (escaped == 'r') output.push_back('\r');
                else if (escaped == 't') output.push_back('\t');
                else if (escaped == 'u')
                {
                    unsigned int codePoint = 0;
                    for (int index = 0; index < 4; ++index)
                    {
                        if (position_ >= text_.size()) return false;
                        const char digit = text_[position_++];
                        codePoint <<= 4U;
                        if (digit >= '0' && digit <= '9')
                            codePoint += digit - '0';
                        else if (digit >= 'a' && digit <= 'f')
                            codePoint += digit - 'a' + 10U;
                        else if (digit >= 'A' && digit <= 'F')
                            codePoint += digit - 'A' + 10U;
                        else
                            return false;
                    }
                    output.push_back(
                        codePoint <= 0x7fU ? static_cast<char>(codePoint) : '?');
                }
                else
                {
                    return false;
                }
            }
            if (output.size() > 8192U) return false;
        }
        return false;
    }

    bool number(Value& value)
    {
        const std::size_t start = position_;
        if (position_ < text_.size() && text_[position_] == '-') ++position_;
        if (position_ >= text_.size()) return false;
        if (text_[position_] == '0')
        {
            ++position_;
        }
        else
        {
            if (text_[position_] < '1' || text_[position_] > '9') return false;
            while (position_ < text_.size() &&
                   text_[position_] >= '0' && text_[position_] <= '9')
            {
                ++position_;
            }
        }
        if (position_ < text_.size() && text_[position_] == '.')
        {
            ++position_;
            const std::size_t digits = position_;
            while (position_ < text_.size() &&
                   text_[position_] >= '0' && text_[position_] <= '9')
            {
                ++position_;
            }
            if (digits == position_) return false;
        }
        if (position_ < text_.size() &&
            (text_[position_] == 'e' || text_[position_] == 'E'))
        {
            ++position_;
            if (position_ < text_.size() &&
                (text_[position_] == '+' || text_[position_] == '-'))
            {
                ++position_;
            }
            const std::size_t digits = position_;
            while (position_ < text_.size() &&
                   text_[position_] >= '0' && text_[position_] <= '9')
            {
                ++position_;
            }
            if (digits == position_) return false;
        }

        const std::string token = text_.substr(start, position_ - start);
        errno = 0;
        char* end = nullptr;
        const double parsed = std::strtod(token.c_str(), &end);
        if (errno != 0 || end == token.c_str() || end == nullptr ||
            *end != '\0' || !std::isfinite(parsed))
        {
            return false;
        }
        value.type = Value::Type::Number;
        value.number = parsed;
        return true;
    }

    bool literal(const char* value)
    {
        const std::size_t length = std::strlen(value);
        if (text_.compare(position_, length, value) != 0) return false;
        position_ += length;
        return true;
    }
};

bool positiveInteger(const Value* value, int& output)
{
    if (value == nullptr || value->type != Value::Type::Number ||
        std::floor(value->number) != value->number || value->number <= 0.0 ||
        value->number > static_cast<double>(std::numeric_limits<int>::max()))
    {
        return false;
    }
    output = static_cast<int>(value->number);
    return true;
}

bool digits(const std::string& value, bool allowZero)
{
    if (value.empty() || value.size() > 10U) return false;
    if (!std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return character >= '0' && character <= '9';
        }))
    {
        return false;
    }
    if (value.size() > 1U && value.front() == '0') return false;
    if (!allowZero && value == "0") return false;
    return true;
}

bool safeImageUrl(const std::string& value)
{
    static const std::string Prefix =
        "https://static.tvmaze.com/uploads/images/original_untouched/";
    if (value.size() <= Prefix.size() || value.size() > 512U ||
        value.compare(0, Prefix.size(), Prefix) != 0)
    {
        return false;
    }

    const std::string relative = value.substr(Prefix.size());
    const std::size_t slash = relative.find('/');
    if (slash == std::string::npos ||
        relative.find('/', slash + 1U) != std::string::npos)
    {
        return false;
    }

    const std::string directory = relative.substr(0, slash);
    const std::string file = relative.substr(slash + 1U);
    const std::size_t dot = file.find_last_of('.');
    if (dot == std::string::npos || file.find('.', dot + 1U) != std::string::npos)
        return false;

    const std::string stem = file.substr(0, dot);
    const std::string extension = file.substr(dot);
    return digits(directory, true) && digits(stem, false) &&
        (extension == ".jpg" || extension == ".jpeg" || extension == ".png");
}

int imageRank(const TvmazeSeriesImage& image)
{
    if (image.background) return 0;
    if (image.main) return 1;
    return 2;
}

double aspectDeviation(const TvmazeSeriesImage& image)
{
    const double target = image.background ? 16.0 / 9.0 : 2.0 / 3.0;
    return std::abs(
        static_cast<double>(image.width) / image.height - target);
}
}

bool parseTvmazeShowLocation(
    const std::string& location,
    int& showId)
{
    showId = 0;
    static const std::string RelativePrefix = "/shows/";
    static const std::string HttpsPrefix = "https://api.tvmaze.com/shows/";
    static const std::string HttpPrefix = "http://api.tvmaze.com/shows/";

    std::string value;
    if (location.compare(0, RelativePrefix.size(), RelativePrefix) == 0)
        value = location.substr(RelativePrefix.size());
    else if (location.compare(0, HttpsPrefix.size(), HttpsPrefix) == 0)
        value = location.substr(HttpsPrefix.size());
    else if (location.compare(0, HttpPrefix.size(), HttpPrefix) == 0)
        value = location.substr(HttpPrefix.size());
    else
        return false;

    if (!digits(value, false)) return false;
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || end == nullptr || *end != '\0' ||
        parsed <= 0 || parsed > std::numeric_limits<int>::max())
    {
        return false;
    }
    showId = static_cast<int>(parsed);
    return true;
}

bool parseTvmazeSeriesImage(
    const std::string& body,
    std::size_t maximumBytes,
    TvmazeSeriesImage& image)
{
    image = {};
    Value root;
    if (!Parser(body, maximumBytes).parse(root) ||
        root.type != Value::Type::Array)
    {
        return false;
    }

    std::vector<TvmazeSeriesImage> candidates;
    for (const Value& item : root.array)
    {
        if (item.type != Value::Type::Object) continue;

        TvmazeSeriesImage candidate;
        if (!positiveInteger(item.member("id"), candidate.imageId)) continue;

        const Value* type = item.member("type");
        if (type == nullptr || type->type != Value::Type::String) continue;
        if (type->string == "background")
            candidate.background = true;
        else if (type->string != "poster")
            continue;

        const Value* main = item.member("main");
        if (main != nullptr)
        {
            if (main->type != Value::Type::Boolean) continue;
            candidate.main = main->boolean;
        }

        const Value* resolutions = item.member("resolutions");
        const Value* original =
            resolutions == nullptr ? nullptr : resolutions->member("original");
        if (original == nullptr || original->type != Value::Type::Object)
            continue;

        const Value* url = original->member("url");
        if (url == nullptr || url->type != Value::Type::String ||
            !safeImageUrl(url->string))
        {
            continue;
        }
        candidate.url = url->string;
        if (!positiveInteger(original->member("width"), candidate.width) ||
            !positiveInteger(original->member("height"), candidate.height))
        {
            continue;
        }
        candidates.push_back(std::move(candidate));
    }

    if (candidates.empty()) return true;

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const TvmazeSeriesImage& left, const TvmazeSeriesImage& right) {
            const int leftRank = imageRank(left);
            const int rightRank = imageRank(right);
            if (leftRank != rightRank) return leftRank < rightRank;

            const double leftDeviation = aspectDeviation(left);
            const double rightDeviation = aspectDeviation(right);
            if (std::abs(leftDeviation - rightDeviation) > 0.000001)
                return leftDeviation < rightDeviation;

            const long long leftPixels =
                static_cast<long long>(left.width) * left.height;
            const long long rightPixels =
                static_cast<long long>(right.width) * right.height;
            if (leftPixels != rightPixels) return leftPixels > rightPixels;
            if (left.imageId != right.imageId)
                return left.imageId < right.imageId;
            return left.url < right.url;
        });

    image = candidates.front();
    return true;
}

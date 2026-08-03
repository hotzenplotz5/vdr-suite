#include "TmdbSeriesArtworkJson.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
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
            if (position_ >= text_.size() || text_[position_++] != ':') return false;
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
        if (position_ >= text_.size() || text_[position_++] != '"') return false;
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
                        if (digit >= '0' && digit <= '9') codePoint += digit - '0';
                        else if (digit >= 'a' && digit <= 'f') codePoint += digit - 'a' + 10U;
                        else if (digit >= 'A' && digit <= 'F') codePoint += digit - 'A' + 10U;
                        else return false;
                    }
                    output.push_back(codePoint <= 0x7fU
                        ? static_cast<char>(codePoint) : '?');
                }
                else return false;
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
        if (text_[position_] == '0') ++position_;
        else
        {
            if (text_[position_] < '1' || text_[position_] > '9') return false;
            while (position_ < text_.size() &&
                   text_[position_] >= '0' && text_[position_] <= '9') ++position_;
        }
        if (position_ < text_.size() && text_[position_] == '.')
        {
            ++position_;
            const std::size_t digits = position_;
            while (position_ < text_.size() &&
                   text_[position_] >= '0' && text_[position_] <= '9') ++position_;
            if (digits == position_) return false;
        }
        if (position_ < text_.size() &&
            (text_[position_] == 'e' || text_[position_] == 'E'))
        {
            ++position_;
            if (position_ < text_.size() &&
                (text_[position_] == '+' || text_[position_] == '-')) ++position_;
            const std::size_t digits = position_;
            while (position_ < text_.size() &&
                   text_[position_] >= '0' && text_[position_] <= '9') ++position_;
            if (digits == position_) return false;
        }
        const std::string token = text_.substr(start, position_ - start);
        errno = 0;
        char* end = nullptr;
        const double parsed = std::strtod(token.c_str(), &end);
        if (errno != 0 || end == token.c_str() || end == nullptr ||
            *end != '\0' || !std::isfinite(parsed)) return false;
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
        return false;
    output = static_cast<int>(value->number);
    return true;
}

bool safeFilePath(const std::string& value)
{
    if (value.size() < 6U || value.size() > 256U || value.front() != '/' ||
        value.find('/', 1) != std::string::npos || value.find("..") != std::string::npos)
        return false;
    if (!std::all_of(value.begin() + 1, value.end(), [](unsigned char character) {
            return (character >= 'A' && character <= 'Z') ||
                (character >= 'a' && character <= 'z') ||
                (character >= '0' && character <= '9') ||
                character == '_' || character == '-' || character == '.';
        })) return false;
    const std::size_t dot = value.find_last_of('.');
    if (dot == std::string::npos) return false;
    const std::string extension = value.substr(dot);
    return extension == ".jpg" || extension == ".jpeg" || extension == ".png";
}

int languageRank(const TmdbSeriesBackdrop& image, const std::string& preferred)
{
    const std::string base = preferred.substr(0, 2);
    if (!image.languageIsNull && image.language == base) return 0;
    if (image.languageIsNull) return 1;
    if (image.language == "en") return 2;
    return 3;
}
}

bool parseTmdbFindSeriesId(
    const std::string& body,
    std::size_t maximumBytes,
    int& seriesId)
{
    seriesId = 0;
    Value root;
    if (!Parser(body, maximumBytes).parse(root) || root.type != Value::Type::Object)
        return false;
    const Value* results = root.member("tv_results");
    if (results == nullptr || results->type != Value::Type::Array) return false;
    if (results->array.empty()) return true;
    if (results->array.size() != 1U ||
        results->array.front().type != Value::Type::Object) return true;
    positiveInteger(results->array.front().member("id"), seriesId);
    return true;
}

bool parseTmdbSeriesBackdrop(
    const std::string& body,
    std::size_t maximumBytes,
    const std::string& preferredLanguage,
    TmdbSeriesBackdrop& backdrop)
{
    backdrop = {};
    Value root;
    if (!Parser(body, maximumBytes).parse(root) || root.type != Value::Type::Object)
        return false;
    const Value* backdrops = root.member("backdrops");
    if (backdrops == nullptr || backdrops->type != Value::Type::Array) return false;

    std::vector<TmdbSeriesBackdrop> candidates;
    for (const Value& item : backdrops->array)
    {
        if (item.type != Value::Type::Object) continue;
        const Value* path = item.member("file_path");
        if (path == nullptr || path->type != Value::Type::String ||
            !safeFilePath(path->string)) continue;
        TmdbSeriesBackdrop image;
        image.filePath = path->string;
        if (!positiveInteger(item.member("width"), image.width) ||
            !positiveInteger(item.member("height"), image.height)) continue;
        const Value* vote = item.member("vote_average");
        if (vote != nullptr && vote->type == Value::Type::Number &&
            std::isfinite(vote->number)) image.voteAverage = vote->number;
        const Value* language = item.member("iso_639_1");
        if (language == nullptr || language->type == Value::Type::Null)
            image.languageIsNull = true;
        else if (language->type == Value::Type::String && language->string.size() == 2U)
            image.language = language->string;
        else continue;
        candidates.push_back(std::move(image));
    }
    if (candidates.empty()) return true;

    const double target = 16.0 / 9.0;
    std::sort(candidates.begin(), candidates.end(), [&](const auto& left, const auto& right) {
        const int leftLanguage = languageRank(left, preferredLanguage);
        const int rightLanguage = languageRank(right, preferredLanguage);
        if (leftLanguage != rightLanguage) return leftLanguage < rightLanguage;
        const double leftDeviation = std::abs(static_cast<double>(left.width) / left.height - target);
        const double rightDeviation = std::abs(static_cast<double>(right.width) / right.height - target);
        if (std::abs(leftDeviation - rightDeviation) > 0.000001)
            return leftDeviation < rightDeviation;
        const long long leftPixels = static_cast<long long>(left.width) * left.height;
        const long long rightPixels = static_cast<long long>(right.width) * right.height;
        if (leftPixels != rightPixels) return leftPixels > rightPixels;
        if (left.voteAverage != right.voteAverage) return left.voteAverage > right.voteAverage;
        return left.filePath < right.filePath;
    });
    backdrop = candidates.front();
    return true;
}

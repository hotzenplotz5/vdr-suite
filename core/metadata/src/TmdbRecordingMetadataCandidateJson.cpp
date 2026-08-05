#include "TmdbRecordingMetadataCandidateJson.h"

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
            ++position_;
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
            if (character != '\\') output.push_back(static_cast<char>(character));
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
            if (output.size() > 16384U) return false;
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

std::string text(const Value& object, const char* name)
{
    const Value* value = object.member(name);
    return value != nullptr && value->type == Value::Type::String
        ? value->string : std::string{};
}

int integer(const Value& object, const char* name)
{
    const Value* value = object.member(name);
    if (value == nullptr || value->type != Value::Type::Number ||
        std::floor(value->number) != value->number || value->number < 0.0 ||
        value->number > static_cast<double>(std::numeric_limits<int>::max()))
        return 0;
    return static_cast<int>(value->number);
}

double decimal(const Value& object, const char* name)
{
    const Value* value = object.member(name);
    if (value == nullptr || value->type != Value::Type::Number ||
        !std::isfinite(value->number)) return 0.0;
    return std::max(0.0, std::min(10.0, value->number));
}

bool validLimit(int limit)
{
    return limit >= 1 && limit <= 20;
}

bool validCastLimit(int limit)
{
    return limit >= 1 && limit <= 128;
}

bool appendCandidate(
    const Value& item,
    RecordingMetadataCandidateKind kind,
    const std::string& parentExternalId,
    int forcedSeasonNumber,
    std::vector<RecordingMetadataCandidate>& candidates)
{
    if (item.type != Value::Type::Object) return true;
    const int identifier = integer(item, "id");
    if (identifier <= 0) return true;

    RecordingMetadataCandidate candidate;
    candidate.kind = kind;
    candidate.providerId = "tmdb";
    candidate.externalId = std::to_string(identifier);
    candidate.parentExternalId = parentExternalId;
    candidate.rating = decimal(item, "vote_average");

    switch (kind)
    {
    case RecordingMetadataCandidateKind::Movie:
        candidate.externalNamespace = "movie";
        candidate.title = text(item, "title");
        candidate.originalTitle = text(item, "original_title");
        candidate.releaseDate = text(item, "release_date");
        candidate.posterReference = text(item, "poster_path");
        break;
    case RecordingMetadataCandidateKind::Series:
        candidate.externalNamespace = "tv";
        candidate.title = text(item, "name");
        candidate.originalTitle = text(item, "original_name");
        candidate.releaseDate = text(item, "first_air_date");
        candidate.posterReference = text(item, "poster_path");
        break;
    case RecordingMetadataCandidateKind::Season:
        candidate.externalNamespace = "tv-season";
        candidate.title = text(item, "name");
        candidate.overview = text(item, "overview");
        candidate.releaseDate = text(item, "air_date");
        candidate.posterReference = text(item, "poster_path");
        candidate.seasonNumber = integer(item, "season_number");
        break;
    case RecordingMetadataCandidateKind::Episode:
        candidate.externalNamespace = "tv-episode";
        candidate.title = text(item, "name");
        candidate.overview = text(item, "overview");
        candidate.releaseDate = text(item, "air_date");
        candidate.posterReference = text(item, "still_path");
        candidate.seasonNumber = integer(item, "season_number");
        if (candidate.seasonNumber == 0) candidate.seasonNumber = forcedSeasonNumber;
        candidate.episodeNumber = integer(item, "episode_number");
        break;
    }

    if (kind == RecordingMetadataCandidateKind::Movie ||
        kind == RecordingMetadataCandidateKind::Series)
        candidate.overview = text(item, "overview");

    if (candidate.valid()) candidates.push_back(std::move(candidate));
    return true;
}

bool parseArray(
    const std::string& body,
    std::size_t maximumBytes,
    const char* memberName,
    RecordingMetadataCandidateKind kind,
    const std::string& parentExternalId,
    int forcedSeasonNumber,
    int limit,
    std::vector<RecordingMetadataCandidate>& candidates,
    bool& truncated)
{
    candidates.clear();
    truncated = false;
    if (!validLimit(limit)) return false;

    Value root;
    if (!Parser(body, maximumBytes).parse(root) || root.type != Value::Type::Object)
        return false;
    const Value* results = root.member(memberName);
    if (results == nullptr || results->type != Value::Type::Array) return false;

    for (const Value& item : results->array)
    {
        if (static_cast<int>(candidates.size()) >= limit)
        {
            truncated = true;
            break;
        }
        if (!appendCandidate(
                item, kind, parentExternalId, forcedSeasonNumber, candidates))
            return false;
    }
    if (results->array.size() > candidates.size()) truncated = true;
    return true;
}
}

bool parseTmdbRecordingCandidateSearch(
    const std::string& body,
    std::size_t maximumBytes,
    RecordingMetadataCandidateKind kind,
    int limit,
    std::vector<RecordingMetadataCandidate>& candidates,
    bool& truncated)
{
    if (kind != RecordingMetadataCandidateKind::Movie &&
        kind != RecordingMetadataCandidateKind::Series)
        return false;
    return parseArray(
        body, maximumBytes, "results", kind, "", 0, limit,
        candidates, truncated);
}

bool parseTmdbRecordingCandidateSeasons(
    const std::string& body,
    std::size_t maximumBytes,
    const std::string& seriesExternalId,
    int limit,
    std::vector<RecordingMetadataCandidate>& candidates,
    bool& truncated)
{
    return parseArray(
        body, maximumBytes, "seasons",
        RecordingMetadataCandidateKind::Season,
        seriesExternalId, 0, limit, candidates, truncated);
}

bool parseTmdbRecordingCandidateEpisodes(
    const std::string& body,
    std::size_t maximumBytes,
    const std::string& seriesExternalId,
    int seasonNumber,
    int limit,
    std::vector<RecordingMetadataCandidate>& candidates,
    bool& truncated)
{
    if (seasonNumber <= 0 || seasonNumber > 10000) return false;
    return parseArray(
        body, maximumBytes, "episodes",
        RecordingMetadataCandidateKind::Episode,
        seriesExternalId, seasonNumber, limit, candidates, truncated);
}

bool parseTmdbRecordingMovieCredits(
    const std::string& body,
    std::size_t maximumBytes,
    int limit,
    std::vector<RecordingMetadataCastMember>& cast,
    bool& truncated)
{
    cast.clear();
    truncated = false;
    if (!validCastLimit(limit)) return false;

    Value root;
    if (!Parser(body, maximumBytes).parse(root) || root.type != Value::Type::Object)
        return false;
    const Value* items = root.member("cast");
    if (items == nullptr || items->type != Value::Type::Array) return false;

    for (const Value& item : items->array)
    {
        if (static_cast<int>(cast.size()) >= limit)
        {
            truncated = true;
            break;
        }
        if (item.type != Value::Type::Object) continue;
        const int identifier = integer(item, "id");
        if (identifier <= 0) continue;

        RecordingMetadataCastMember member;
        member.providerId = "tmdb";
        member.externalNamespace = "person";
        member.externalId = std::to_string(identifier);
        member.name = text(item, "name");
        member.characterName = text(item, "character");
        member.profileReference = text(item, "profile_path");
        member.order = integer(item, "order");
        if (member.valid()) cast.push_back(std::move(member));
    }

    if (items->array.size() > cast.size()) truncated = true;
    return true;
}
#include "RestfulApiNativeTimerInventoryReader.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "IHttpClient.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace vdrsuite::vdr
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxInventorySize = 4096;
constexpr std::size_t kMaxPayloadLength = 4 * 1024 * 1024;
constexpr std::size_t kMaxJsonDepth = 64;

bool validIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

void appendUtf8(std::string& output, unsigned int codePoint)
{
    if (codePoint <= 0x7F)
        output.push_back(static_cast<char>(codePoint));
    else if (codePoint <= 0x7FF)
    {
        output.push_back(static_cast<char>(0xC0 | ((codePoint >> 6) & 0x1F)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
    else
    {
        output.push_back(static_cast<char>(0xE0 | ((codePoint >> 12) & 0x0F)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

class JsonCursor
{
public:
    explicit JsonCursor(const std::string& text) : text_(text) {}

    void whitespace()
    {
        while (pos_ < text_.size()
            && std::isspace(static_cast<unsigned char>(text_[pos_])))
            ++pos_;
    }

    bool end()
    {
        whitespace();
        return pos_ == text_.size();
    }

    bool take(char expected)
    {
        whitespace();
        if (pos_ >= text_.size() || text_[pos_] != expected) return false;
        ++pos_;
        return true;
    }

    bool peek(char expected)
    {
        whitespace();
        return pos_ < text_.size() && text_[pos_] == expected;
    }

    bool string(std::string& output)
    {
        whitespace();
        if (pos_ >= text_.size() || text_[pos_] != '"') return false;
        ++pos_;
        output.clear();
        while (pos_ < text_.size())
        {
            unsigned char ch = static_cast<unsigned char>(text_[pos_++]);
            if (ch == '"') return true;
            if (ch < 0x20) return false;
            if (ch != '\\')
            {
                output.push_back(static_cast<char>(ch));
                continue;
            }
            if (pos_ >= text_.size()) return false;
            const char escaped = text_[pos_++];
            switch (escaped)
            {
                case '"': output.push_back('"'); break;
                case '\\': output.push_back('\\'); break;
                case '/': output.push_back('/'); break;
                case 'b': output.push_back('\b'); break;
                case 'f': output.push_back('\f'); break;
                case 'n': output.push_back('\n'); break;
                case 'r': output.push_back('\r'); break;
                case 't': output.push_back('\t'); break;
                case 'u':
                {
                    if (pos_ + 4 > text_.size()) return false;
                    unsigned int codePoint = 0;
                    for (int i = 0; i < 4; ++i)
                    {
                        const char hex = text_[pos_++];
                        unsigned int value = 0;
                        if (hex >= '0' && hex <= '9') value = hex - '0';
                        else if (hex >= 'a' && hex <= 'f') value = 10 + hex - 'a';
                        else if (hex >= 'A' && hex <= 'F') value = 10 + hex - 'A';
                        else return false;
                        codePoint = (codePoint << 4) | value;
                    }
                    // Reject surrogate halves. Conservative rejection is safer
                    // than inventing an identity during absence proof.
                    if (codePoint >= 0xD800 && codePoint <= 0xDFFF) return false;
                    appendUtf8(output, codePoint);
                    break;
                }
                default: return false;
            }
        }
        return false;
    }

    bool number(std::string& token)
    {
        whitespace();
        const std::size_t start = pos_;
        if (pos_ < text_.size() && text_[pos_] == '-') ++pos_;
        if (pos_ >= text_.size()) return false;
        if (text_[pos_] == '0') ++pos_;
        else
        {
            if (!std::isdigit(static_cast<unsigned char>(text_[pos_])) || text_[pos_] == '0')
                return false;
            while (pos_ < text_.size()
                && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
        }
        if (pos_ < text_.size() && text_[pos_] == '.')
        {
            ++pos_;
            const std::size_t fraction = pos_;
            while (pos_ < text_.size()
                && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
            if (pos_ == fraction) return false;
        }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E'))
        {
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
            const std::size_t exponent = pos_;
            while (pos_ < text_.size()
                && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
            if (pos_ == exponent) return false;
        }
        token = text_.substr(start, pos_ - start);
        return pos_ > start;
    }

    bool literal(const char* value)
    {
        whitespace();
        const std::string expected(value);
        if (text_.compare(pos_, expected.size(), expected) != 0) return false;
        pos_ += expected.size();
        return true;
    }

    bool skipValue(std::size_t depth = 0)
    {
        if (depth > kMaxJsonDepth) return false;
        whitespace();
        if (pos_ >= text_.size()) return false;
        if (text_[pos_] == '"')
        {
            std::string ignored;
            return string(ignored);
        }
        if (text_[pos_] == '{')
        {
            if (!take('{')) return false;
            if (take('}')) return true;
            std::set<std::string> keys;
            for (;;)
            {
                std::string key;
                if (!string(key) || !keys.insert(key).second || !take(':')
                    || !skipValue(depth + 1)) return false;
                if (take('}')) return true;
                if (!take(',')) return false;
            }
        }
        if (text_[pos_] == '[')
        {
            if (!take('[')) return false;
            if (take(']')) return true;
            for (;;)
            {
                if (!skipValue(depth + 1)) return false;
                if (take(']')) return true;
                if (!take(',')) return false;
            }
        }
        if (text_[pos_] == 't') return literal("true");
        if (text_[pos_] == 'f') return literal("false");
        if (text_[pos_] == 'n') return literal("null");
        std::string ignored;
        return number(ignored);
    }

private:
    const std::string& text_;
    std::size_t pos_ = 0;
};

bool nonNegativeIntegerToken(const std::string& token, std::string& identity)
{
    if (token.empty() || token[0] == '-') return false;
    for (char ch : token)
        if (ch < '0' || ch > '9') return false;
    std::size_t first = token.find_first_not_of('0');
    identity = first == std::string::npos ? "0" : token.substr(first);
    return validIdentity(identity);
}

bool parseTimerObject(JsonCursor& cursor, std::string& nativeId)
{
    if (!cursor.take('{')) return false;
    if (cursor.take('}')) return false;

    std::set<std::string> keys;
    bool idSeen = false;
    bool numberSeen = false;
    std::string id;
    std::string number;
    for (;;)
    {
        std::string key;
        if (!cursor.string(key) || !keys.insert(key).second || !cursor.take(':'))
            return false;

        if (key == "id")
        {
            idSeen = true;
            if (!cursor.string(id)) return false;
        }
        else if (key == "number")
        {
            numberSeen = true;
            if (cursor.peek('"'))
            {
                if (!cursor.string(number)) return false;
            }
            else if (!cursor.number(number)) return false;
        }
        else if (!cursor.skipValue(1)) return false;

        if (cursor.take('}')) break;
        if (!cursor.take(',')) return false;
    }

    if (idSeen && !id.empty())
    {
        if (!validIdentity(id)) return false;
        nativeId = id;
        return true;
    }
    if (!numberSeen) return false;
    return nonNegativeIntegerToken(number, nativeId);
}

bool parseTimerArray(JsonCursor& cursor, std::vector<std::string>& ids)
{
    if (!cursor.take('[')) return false;
    if (cursor.take(']')) return true;
    for (;;)
    {
        if (ids.size() >= kMaxInventorySize) return false;
        std::string id;
        if (!parseTimerObject(cursor, id)) return false;
        ids.push_back(id);
        if (cursor.take(']')) return true;
        if (!cursor.take(',')) return false;
    }
}

bool parseCompleteTimerIds(const std::string& json, std::vector<std::string>& ids)
{
    JsonCursor cursor(json);
    ids.clear();
    if (cursor.peek('['))
    {
        if (!parseTimerArray(cursor, ids) || !cursor.end()) return false;
    }
    else
    {
        if (!cursor.take('{')) return false;
        if (cursor.take('}')) return false;
        std::set<std::string> keys;
        bool timersSeen = false;
        for (;;)
        {
            std::string key;
            if (!cursor.string(key) || !keys.insert(key).second || !cursor.take(':'))
                return false;
            if (key == "timers")
            {
                if (timersSeen || !parseTimerArray(cursor, ids)) return false;
                timersSeen = true;
            }
            else if (!cursor.skipValue(1)) return false;
            if (cursor.take('}')) break;
            if (!cursor.take(',')) return false;
        }
        if (!timersSeen || !cursor.end()) return false;
    }

    std::sort(ids.begin(), ids.end());
    return std::adjacent_find(ids.begin(), ids.end()) == ids.end();
}

RestfulApiNativeTimerInventoryReadResult result(
    RestfulApiNativeTimerInventoryReadStatus status, int httpStatus = 0)
{
    RestfulApiNativeTimerInventoryReadResult value;
    value.status = status;
    value.httpStatusCode = httpStatus;
    return value;
}
}

RestfulApiNativeTimerInventoryReader::RestfulApiNativeTimerInventoryReader(
    IHttpClient& httpClient)
    : httpClient_(httpClient)
{
}

RestfulApiNativeTimerInventoryReadResult
RestfulApiNativeTimerInventoryReader::read(
    const RestfulApiNativeTimerInventoryReadRequest& request) const
{
    if (!validIdentity(request.backendId)
        || request.backendGeneration == 0
        || request.observedAt <= 0)
    {
        return result(RestfulApiNativeTimerInventoryReadStatus::invalidRequest);
    }

    HttpRequest httpRequest;
    httpRequest.method = "GET";
    httpRequest.url = "/timers.json";
    httpRequest.headers["Accept"] = "application/json";
    const HttpResponse response = httpClient_.execute(httpRequest);
    if (response.statusCode != 200)
        return result(RestfulApiNativeTimerInventoryReadStatus::httpError, response.statusCode);

    if (response.body.size() > kMaxPayloadLength)
        return result(RestfulApiNativeTimerInventoryReadStatus::parseError, response.statusCode);

    std::vector<std::string> ids;
    if (!parseCompleteTimerIds(response.body, ids))
        return result(RestfulApiNativeTimerInventoryReadStatus::parseError, response.statusCode);

    RestfulApiNativeTimerInventoryReadResult value;
    value.status = RestfulApiNativeTimerInventoryReadStatus::complete;
    value.httpStatusCode = response.statusCode;
    value.evidence.backendId = request.backendId;
    value.evidence.backendGeneration = request.backendGeneration;
    value.evidence.observedAt = request.observedAt;
    value.evidence.completeness =
        vdrsuite::timers::NativeTimerInventoryCompleteness::complete;
    value.evidence.backendNativeTimerIds = std::move(ids);
    if (!vdrsuite::timers::nativeTimerInventoryEvidenceValid(value.evidence))
        return result(RestfulApiNativeTimerInventoryReadStatus::parseError, response.statusCode);
    return value;
}

}

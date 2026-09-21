#include "SuiteBridgeRecordingMarksResolver.h"
#include "VdrRecordingNativeIdentity.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
class ParseError final : public std::runtime_error
{
public:
    using runtime_error::runtime_error;
};

class Parser final
{
public:
    explicit Parser(const std::string& value)
        : value_(value)
    {
    }

    bool done() const noexcept
    {
        return offset_ == value_.size();
    }

    bool take(char character) noexcept
    {
        if (offset_ < value_.size() && value_[offset_] == character)
        {
            ++offset_;
            return true;
        }
        return false;
    }

    void need(char character)
    {
        if (!take(character))
        {
            fail(std::string("expected '") + character + "'");
        }
    }

    void literal(const char* text)
    {
        const std::string expected(text);
        if (value_.compare(offset_, expected.size(), expected) != 0)
        {
            fail("expected " + expected);
        }
        offset_ += expected.size();
    }

    void field(const char* name, bool first = false)
    {
        if (!first)
        {
            need(',');
        }
        need('"');
        literal(name);
        need('"');
        need(':');
    }

    bool boolean()
    {
        if (value_.compare(offset_, 4, "true") == 0)
        {
            offset_ += 4;
            return true;
        }
        if (value_.compare(offset_, 5, "false") == 0)
        {
            offset_ += 5;
            return false;
        }
        fail("expected boolean");
        return false;
    }

    int integer()
    {
        const std::string token = numberToken(false);
        errno = 0;
        char* end = nullptr;
        const long value = std::strtol(token.c_str(), &end, 10);
        if (errno != 0 || end == nullptr || *end != '\0' ||
            value < std::numeric_limits<int>::min() ||
            value > std::numeric_limits<int>::max())
        {
            fail("integer out of range");
        }
        return static_cast<int>(value);
    }

    double number()
    {
        const std::string token = numberToken(true);
        errno = 0;
        char* end = nullptr;
        const double value = std::strtod(token.c_str(), &end);
        if (errno != 0 || end == nullptr || *end != '\0' ||
            !std::isfinite(value))
        {
            fail("number out of range");
        }
        return value;
    }

    std::string string()
    {
        need('"');
        std::string result;
        while (offset_ < value_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(value_[offset_++]);
            if (character == '"')
            {
                return result;
            }
            if (character < 0x20)
            {
                fail("control character in string");
            }
            if (character != '\\')
            {
                result.push_back(static_cast<char>(character));
                continue;
            }
            if (offset_ >= value_.size())
            {
                fail("truncated escape");
            }
            switch (value_[offset_++])
            {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            case 'u': appendUnicode(result); break;
            default: fail("invalid escape");
            }
        }
        fail("unterminated string");
        return {};
    }

private:
    [[noreturn]] void fail(const std::string& message) const
    {
        throw ParseError(
            message + " at byte " + std::to_string(offset_));
    }

    std::string numberToken(bool decimal)
    {
        const std::size_t start = offset_;
        if (offset_ < value_.size() && value_[offset_] == '-')
        {
            ++offset_;
        }
        if (offset_ >= value_.size() ||
            value_[offset_] < '0' || value_[offset_] > '9')
        {
            fail("expected number");
        }
        if (value_[offset_] == '0')
        {
            ++offset_;
        }
        else
        {
            while (offset_ < value_.size() &&
                   value_[offset_] >= '0' && value_[offset_] <= '9')
            {
                ++offset_;
            }
        }
        if (decimal && offset_ < value_.size() && value_[offset_] == '.')
        {
            ++offset_;
            const std::size_t digits = offset_;
            while (offset_ < value_.size() &&
                   value_[offset_] >= '0' && value_[offset_] <= '9')
            {
                ++offset_;
            }
            if (digits == offset_)
            {
                fail("invalid fraction");
            }
        }
        if (decimal && offset_ < value_.size() &&
            (value_[offset_] == 'e' || value_[offset_] == 'E'))
        {
            ++offset_;
            if (offset_ < value_.size() &&
                (value_[offset_] == '+' || value_[offset_] == '-'))
            {
                ++offset_;
            }
            const std::size_t digits = offset_;
            while (offset_ < value_.size() &&
                   value_[offset_] >= '0' && value_[offset_] <= '9')
            {
                ++offset_;
            }
            if (digits == offset_)
            {
                fail("invalid exponent");
            }
        }
        return value_.substr(start, offset_ - start);
    }

    unsigned hex4()
    {
        if (offset_ + 4 > value_.size())
        {
            fail("truncated unicode escape");
        }
        unsigned result = 0;
        for (int index = 0; index < 4; ++index)
        {
            const unsigned char character =
                static_cast<unsigned char>(value_[offset_++]);
            result <<= 4;
            if (character >= '0' && character <= '9')
            {
                result += character - '0';
            }
            else if (character >= 'a' && character <= 'f')
            {
                result += character - 'a' + 10;
            }
            else if (character >= 'A' && character <= 'F')
            {
                result += character - 'A' + 10;
            }
            else
            {
                fail("invalid unicode escape");
            }
        }
        return result;
    }

    void appendUnicode(std::string& result)
    {
        const unsigned value = hex4();
        if (value >= 0xD800 && value <= 0xDFFF)
        {
            fail("surrogate escape is unsupported");
        }
        if (value <= 0x7F)
        {
            result.push_back(static_cast<char>(value));
        }
        else if (value <= 0x7FF)
        {
            result.push_back(static_cast<char>(0xC0 | (value >> 6)));
            result.push_back(static_cast<char>(0x80 | (value & 0x3F)));
        }
        else
        {
            result.push_back(static_cast<char>(0xE0 | (value >> 12)));
            result.push_back(static_cast<char>(
                0x80 | ((value >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (value & 0x3F)));
        }
    }

    const std::string& value_;
    std::size_t offset_ = 0;
};

void bounded(
    const std::string& value,
    std::size_t maximum,
    const char* field)
{
    if (value.size() > maximum)
    {
        throw ParseError(std::string(field) + " exceeds maximum length");
    }
}

bool revisionValid(const std::string& revision)
{
    if (revision.size() != 32)
    {
        return false;
    }
    for (const unsigned char character : revision)
    {
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f')))
        {
            return false;
        }
    }
    return true;
}

std::vector<VdrRecordingNativeMark> parseMarks(
    Parser& parser,
    double framesPerSecond)
{
    std::vector<VdrRecordingNativeMark> marks;
    parser.need('[');
    bool first = true;
    int previousPosition = -1;
    while (!parser.take(']'))
    {
        if (!first)
        {
            parser.need(',');
        }
        if (marks.size() >= VdrRecordingNativeMarks::MaximumMarks)
        {
            throw ParseError("marks exceeds maximum count");
        }

        VdrRecordingNativeMark mark;
        parser.need('{');
        parser.field("positionFrame", true);
        mark.positionFrame = parser.integer();
        parser.field("timecode");
        mark.timecode = parser.string();
        parser.field("positionSeconds");
        mark.positionSeconds = parser.number();
        parser.field("comment");
        mark.comment = parser.string();
        parser.need('}');

        bounded(mark.timecode, 64, "mark.timecode");
        bounded(
            mark.comment,
            VdrRecordingNativeMarks::MaximumCommentBytes,
            "mark.comment");
        if (mark.positionFrame < 0 ||
            mark.positionFrame < previousPosition ||
            mark.timecode.empty() || mark.positionSeconds < 0.0)
        {
            throw ParseError("invalid native mark");
        }
        const double expectedSeconds =
            static_cast<double>(mark.positionFrame) / framesPerSecond;
        if (std::fabs(mark.positionSeconds - expectedSeconds) > 1.0e-6)
        {
            throw ParseError("mark display seconds mismatch native frame");
        }
        previousPosition = mark.positionFrame;
        marks.push_back(std::move(mark));
        first = false;
    }
    return marks;
}

VdrRecordingNativeMarks parsePayload(
    const std::string& expectedRecordingKey,
    const std::string& json)
{
    if (json.empty() ||
        json.size() > VdrRecordingNativeMarks::MaximumPayloadBytes)
    {
        throw ParseError("payload size is invalid");
    }

    Parser parser(json);
    VdrRecordingNativeMarks result;
    parser.need('{');
    parser.field("schema", true);
    result.schema = parser.integer();
    parser.field("found");
    result.found = parser.boolean();
    parser.field("reason");
    result.reason = parser.string();
    parser.field("recordingIdentitySchema");
    result.recordingIdentitySchema = parser.integer();
    parser.field("recordingKey");
    result.recordingKey = parser.string();
    parser.field("state");
    result.state = parser.string();
    parser.field("framesPerSecond");
    result.framesPerSecond = parser.number();
    parser.field("isPesRecording");
    result.isPesRecording = parser.boolean();
    parser.field("inUseFlags");
    result.inUseFlags = parser.integer();
    parser.field("marksFilePresent");
    result.marksFilePresent = parser.boolean();
    parser.field("sequenceCount");
    result.sequenceCount = parser.integer();
    parser.field("marksRevision");
    result.marksRevision = parser.string();
    parser.field("marks");

    if (result.schema != VdrRecordingNativeMarks::SupportedSchema ||
        result.recordingIdentitySchema !=
            VdrRecordingNativeMarks::SupportedIdentitySchema ||
        result.recordingKey != expectedRecordingKey ||
        !VdrRecordingNativeIdentity::isValidKey(result.recordingKey))
    {
        throw ParseError("schema or recording identity mismatch");
    }
    bounded(result.reason, 64, "reason");
    bounded(result.state, 32, "state");
    bounded(result.marksRevision, 64, "marksRevision");
    if (result.inUseFlags < 0 || result.sequenceCount < 0)
    {
        throw ParseError("invalid native marks status");
    }

    if (result.found && result.state != "unreadable")
    {
        if (result.framesPerSecond <= 0.0 ||
            (result.state != "none" && result.state != "present"))
        {
            throw ParseError("invalid readable native marks state");
        }
        result.marks = parseMarks(parser, result.framesPerSecond);
    }
    else
    {
        parser.need('[');
        parser.need(']');
    }

    parser.need('}');
    if (!parser.done())
    {
        throw ParseError("trailing payload data");
    }

    if (!result.found)
    {
        if ((result.reason != "recording_not_found" &&
             result.reason != "identity_ambiguous") ||
            result.state != "none" ||
            result.framesPerSecond != 0.0 ||
            result.inUseFlags != 0 ||
            result.marksFilePresent ||
            result.sequenceCount != 0 ||
            !result.marksRevision.empty())
        {
            throw ParseError("invalid missing recording marks state");
        }
        result.availability =
            VdrRecordingNativeMarksAvailability::RecordingNotFound;
        return result;
    }

    if (result.reason != "none")
    {
        throw ParseError("invalid found recording marks reason");
    }

    if (result.state == "unreadable")
    {
        if (!result.marksRevision.empty() || !result.marks.empty())
        {
            throw ParseError("unreadable marks must not claim canonical revision");
        }
        result.availability =
            VdrRecordingNativeMarksAvailability::NativeUnreadable;
        return result;
    }

    if (!revisionValid(result.marksRevision) ||
        (result.state == "present" && result.marks.empty()) ||
        (result.state == "none" && !result.marks.empty()) ||
        (result.state == "present" && !result.marksFilePresent))
    {
        throw ParseError("invalid canonical marks payload");
    }

    result.availability = VdrRecordingNativeMarksAvailability::Available;
    return result;
}
}

SuiteBridgeRecordingMarksResolver::SuiteBridgeRecordingMarksResolver(
    ISuiteBridgeRecordingMarksTransport& transport)
    : transport_(transport)
{
}

VdrRecordingNativeMarks SuiteBridgeRecordingMarksResolver::resolve(
    const std::string& recordingKey)
{
    if (!VdrRecordingNativeIdentity::isValidKey(recordingKey))
    {
        VdrRecordingNativeMarks result;
        result.availability =
            VdrRecordingNativeMarksAvailability::InvalidPayload;
        result.diagnostic = "invalid recording key";
        return result;
    }

    return parseReply(
        recordingKey,
        transport_.requestRecordingMarks(recordingKey));
}

VdrRecordingNativeMarks SuiteBridgeRecordingMarksResolver::parseReply(
    const std::string& expectedRecordingKey,
    const SuiteBridgeRecordingMarksCommandReply& reply)
{
    if (!reply.transportSucceeded)
    {
        VdrRecordingNativeMarks result;
        result.recordingKey = expectedRecordingKey;
        result.availability =
            VdrRecordingNativeMarksAvailability::TransportError;
        result.diagnostic = reply.payload.empty()
            ? "SuiteBridge recording marks transport failed"
            : reply.payload;
        return result;
    }

    if (reply.replyCode != 250)
    {
        VdrRecordingNativeMarks result;
        result.recordingKey = expectedRecordingKey;
        result.availability =
            VdrRecordingNativeMarksAvailability::TransportError;
        result.diagnostic = "unexpected SuiteBridge reply code";
        return result;
    }

    try
    {
        return parsePayload(expectedRecordingKey, reply.payload);
    }
    catch (const std::exception& error)
    {
        VdrRecordingNativeMarks result;
        result.recordingKey = expectedRecordingKey;
        result.availability =
            VdrRecordingNativeMarksAvailability::InvalidPayload;
        result.diagnostic = error.what();
        return result;
    }
}

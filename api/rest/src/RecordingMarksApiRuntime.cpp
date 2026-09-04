#include "RecordingMarksApiRuntime.h"

#include "VdrRecordingNativeIdentity.h"

#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <utility>

namespace
{
constexpr const char* RecordingMarksRoute =
    "/api/vdr/recordings/marks";
constexpr std::size_t MaximumBackendIdBytes = 128U;
constexpr std::size_t MaximumRecordingIdBytes = 4096U;

std::string requestPath(const std::string& target)
{
    const std::size_t query = target.find('?');
    return query == std::string::npos
        ? target
        : target.substr(0, query);
}

int hexValue(char character)
{
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

bool urlDecode(
    const std::string& input,
    std::string& output,
    std::size_t maximumBytes)
{
    output.clear();
    if (input.size() > maximumBytes * 3U) return false;

    for (std::size_t index = 0; index < input.size(); ++index)
    {
        const unsigned char character =
            static_cast<unsigned char>(input[index]);
        if (character == '+')
        {
            output.push_back(' ');
        }
        else if (character == '%')
        {
            if (index + 2U >= input.size()) return false;
            const int high = hexValue(input[index + 1U]);
            const int low = hexValue(input[index + 2U]);
            if (high < 0 || low < 0) return false;
            const unsigned char decoded =
                static_cast<unsigned char>((high << 4) | low);
            if (decoded < 0x20U || decoded == 0x7fU) return false;
            output.push_back(static_cast<char>(decoded));
            index += 2U;
        }
        else
        {
            if (character < 0x20U || character == 0x7fU) return false;
            output.push_back(static_cast<char>(character));
        }

        if (output.size() > maximumBytes) return false;
    }

    return true;
}

bool validBackendId(const std::string& value)
{
    return !value.empty() && value.size() <= MaximumBackendIdBytes &&
        std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character) {
                return (character >= 'A' && character <= 'Z') ||
                    (character >= 'a' && character <= 'z') ||
                    (character >= '0' && character <= '9') ||
                    character == '.' || character == '_' || character == '-';
            });
}

bool validRecordingId(const std::string& value)
{
    return !value.empty() && value.size() <= MaximumRecordingIdBytes &&
        std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character) {
                return character >= 0x20U && character != 0x7fU;
            });
}

struct Request
{
    std::string backendId;
    std::string recordingId;
};

bool parseRequest(const std::string& target, Request& request)
{
    request = {};
    if (requestPath(target) != RecordingMarksRoute) return false;

    const std::size_t queryStart = target.find('?');
    if (queryStart == std::string::npos || queryStart + 1U >= target.size())
        return false;

    std::map<std::string, std::string> parameters;
    std::size_t start = queryStart + 1U;
    while (start <= target.size())
    {
        const std::size_t end = target.find('&', start);
        const std::string item = target.substr(
            start,
            end == std::string::npos
                ? std::string::npos
                : end - start);
        if (item.empty()) return false;

        const std::size_t equals = item.find('=');
        if (equals == std::string::npos || equals == 0U) return false;
        const std::string key = item.substr(0, equals);
        if (key != "backend" && key != "recordingId") return false;
        if (parameters.count(key) != 0U) return false;

        std::string decoded;
        const std::size_t maximum =
            key == "backend"
                ? MaximumBackendIdBytes
                : MaximumRecordingIdBytes;
        if (!urlDecode(item.substr(equals + 1U), decoded, maximum))
            return false;
        parameters.emplace(key, std::move(decoded));

        if (end == std::string::npos) break;
        start = end + 1U;
    }

    const auto backend = parameters.find("backend");
    const auto recording = parameters.find("recordingId");
    if (backend == parameters.end() || recording == parameters.end())
        return false;

    request.backendId = backend->second;
    request.recordingId = recording->second;
    return validBackendId(request.backendId) &&
        validRecordingId(request.recordingId);
}

std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\b': escaped += "\\b"; break;
        case '\f': escaped += "\\f"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default:
            if (character >= 0x20U)
                escaped.push_back(static_cast<char>(character));
            break;
        }
    }
    return escaped;
}

ApiResponse errorResponse(
    int statusCode,
    const std::string& code)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body =
        "{\"error\":{\"code\":\"" + jsonEscape(code) + "\"}}";
    return response;
}

ApiResponse serializeAvailable(
    const Request& request,
    const VdrRecordingNativeMarks& nativeMarks)
{
    std::ostringstream json;
    json << std::setprecision(12);
    json << "{\"backendId\":\"" << jsonEscape(request.backendId)
         << "\",\"recordingId\":\"" << jsonEscape(request.recordingId)
         << "\",\"availability\":\"available\""
         << ",\"state\":\"" << jsonEscape(nativeMarks.state) << "\""
         << ",\"framesPerSecond\":" << nativeMarks.framesPerSecond
         << ",\"isPesRecording\":"
         << (nativeMarks.isPesRecording ? "true" : "false")
         << ",\"inUse\":" << (nativeMarks.inUseFlags != 0 ? "true" : "false")
         << ",\"inUseFlags\":" << nativeMarks.inUseFlags
         << ",\"marksFilePresent\":"
         << (nativeMarks.marksFilePresent ? "true" : "false")
         << ",\"sequenceCount\":" << nativeMarks.sequenceCount
         << ",\"marksRevision\":\""
         << jsonEscape(nativeMarks.marksRevision)
         << "\",\"marks\":[";

    for (std::size_t index = 0; index < nativeMarks.marks.size(); ++index)
    {
        const VdrRecordingNativeMark& mark = nativeMarks.marks[index];
        if (index != 0U) json << ',';
        json << "{\"positionFrame\":" << mark.positionFrame
             << ",\"timecode\":\"" << jsonEscape(mark.timecode)
             << "\",\"positionSeconds\":" << mark.positionSeconds
             << ",\"comment\":\"" << jsonEscape(mark.comment)
             << "\"}";
    }

    json << "]}";

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body = json.str();
    return response;
}
}

RecordingMarksApiRuntime& RecordingMarksApiRuntime::instance()
{
    static RecordingMarksApiRuntime runtime;
    return runtime;
}

bool RecordingMarksApiRuntime::configure(
    RecordingLookup recordingLookup,
    BackendResolver backendResolver)
{
    if (!recordingLookup || !backendResolver) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    recordingLookup_ = std::move(recordingLookup);
    backendResolver_ = std::move(backendResolver);
    return true;
}

void RecordingMarksApiRuntime::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    recordingLookup_ = {};
    backendResolver_ = {};
}

bool RecordingMarksApiRuntime::configured() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<bool>(recordingLookup_) &&
        static_cast<bool>(backendResolver_);
}

bool RecordingMarksApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    ApiResponse& response) const
{
    if (requestPath(requestTarget) != RecordingMarksRoute) return false;

    Request request;
    if (!parseRequest(requestTarget, request))
    {
        response = errorResponse(400, "recording_marks_request_invalid");
        return true;
    }

    RecordingLookup recordingLookup;
    BackendResolver backendResolver;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        recordingLookup = recordingLookup_;
        backendResolver = backendResolver_;
    }

    if (!recordingLookup || !backendResolver)
    {
        response = errorResponse(503, "recording_marks_runtime_unavailable");
        return true;
    }

    const std::vector<VdrRecording> recordings =
        recordingLookup(request.backendId);

    const VdrRecording* selected = nullptr;
    std::size_t matches = 0U;
    for (const VdrRecording& recording : recordings)
    {
        if (recording.backendId != request.backendId ||
            recording.id != request.recordingId)
        {
            continue;
        }
        selected = &recording;
        ++matches;
    }

    if (matches == 0U || selected == nullptr)
    {
        response = errorResponse(404, "recording_not_found");
        return true;
    }
    if (matches != 1U)
    {
        response = errorResponse(409, "recording_identity_ambiguous");
        return true;
    }
    if (selected->backendNativeId.empty() ||
        selected->backendNativeId.size() >
            VdrRecordingNativeIdentity::MaximumNativeIdBytes)
    {
        response = errorResponse(409, "recording_native_identity_unavailable");
        return true;
    }

    const std::string recordingKey =
        VdrRecordingNativeIdentity::keyForNativeId(
            selected->backendNativeId);
    if (!VdrRecordingNativeIdentity::isValidKey(recordingKey))
    {
        response = errorResponse(409, "recording_native_identity_unavailable");
        return true;
    }

    const RecordingMarksBackendAccess access =
        backendResolver(request.backendId);
    if (access.availability ==
        RecordingMarksBackendAvailability::BackendNotFound)
    {
        response = errorResponse(404, "backend_not_found");
        return true;
    }
    if (access.availability !=
            RecordingMarksBackendAvailability::Available ||
        access.resolver == nullptr)
    {
        response = errorResponse(
            503,
            "recording_marks_capability_unavailable");
        return true;
    }

    const VdrRecordingNativeMarks nativeMarks =
        access.resolver->resolve(recordingKey);

    if (!nativeMarks.recordingKey.empty() &&
        nativeMarks.recordingKey != recordingKey)
    {
        response = errorResponse(
            502,
            "recording_marks_invalid_native_payload");
        return true;
    }

    switch (nativeMarks.availability)
    {
    case VdrRecordingNativeMarksAvailability::Available:
        response = serializeAvailable(request, nativeMarks);
        return true;
    case VdrRecordingNativeMarksAvailability::RecordingNotFound:
        response = errorResponse(409, "recording_native_state_stale");
        return true;
    case VdrRecordingNativeMarksAvailability::NativeUnreadable:
        response = errorResponse(503, "recording_marks_unreadable");
        return true;
    case VdrRecordingNativeMarksAvailability::TransportError:
        response = errorResponse(503, "recording_marks_transport_unavailable");
        return true;
    case VdrRecordingNativeMarksAvailability::InvalidPayload:
        response = errorResponse(502, "recording_marks_invalid_native_payload");
        return true;
    }

    response = errorResponse(502, "recording_marks_invalid_native_payload");
    return true;
}

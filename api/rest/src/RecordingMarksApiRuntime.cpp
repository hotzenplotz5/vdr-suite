#include "RecordingMarksApiRuntime.h"

#include "VdrRecordingNativeIdentity.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>

namespace
{
constexpr const char* RecordingMarksRoute =
    "/api/vdr/recordings/marks";
constexpr std::size_t MaximumBackendIdBytes = 128U;
constexpr std::size_t MaximumRecordingIdBytes = 4096U;
constexpr std::size_t MaximumMutationBodyBytes = 16384U;
constexpr std::size_t MaximumOperationIdBytes = 192U;
constexpr std::size_t MaximumReplacementFrames = 256U;
constexpr const char* ReplayNotFoundReason =
    "recording_marks_modify_assignment_not_found";

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

bool validOperationToken(const std::string& value)
{
    return !value.empty() && value.size() <= MaximumOperationIdBytes &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}

bool validRevisionToken(const std::string& value)
{
    return value.size() == 32U &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return (character >= '0' && character <= '9') ||
                (character >= 'a' && character <= 'f');
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

void skipWhitespace(const std::string& input, std::size_t& position)
{
    while (position < input.size() &&
        std::isspace(static_cast<unsigned char>(input[position])) != 0)
    {
        ++position;
    }
}

bool parseJsonString(
    const std::string& input,
    std::size_t& position,
    std::string& value,
    std::size_t maximumBytes)
{
    skipWhitespace(input, position);
    if (position >= input.size() || input[position] != '"') return false;
    ++position;
    value.clear();
    while (position < input.size())
    {
        const unsigned char character =
            static_cast<unsigned char>(input[position++]);
        if (character == '"') return true;
        if (character < 0x20U || character == 0x7fU) return false;
        if (character != '\\')
        {
            value.push_back(static_cast<char>(character));
        }
        else
        {
            if (position >= input.size()) return false;
            const char escaped = input[position++];
            switch (escaped)
            {
            case '"': value.push_back('"'); break;
            case '\\': value.push_back('\\'); break;
            case '/': value.push_back('/'); break;
            case 'b': value.push_back('\b'); break;
            case 'f': value.push_back('\f'); break;
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            default: return false;
            }
        }
        if (value.size() > maximumBytes) return false;
    }
    return false;
}

bool parseJsonInt(
    const std::string& input,
    std::size_t& position,
    int& value)
{
    skipWhitespace(input, position);
    if (position >= input.size()) return false;
    bool negative = false;
    if (input[position] == '-')
    {
        negative = true;
        ++position;
    }
    if (position >= input.size() ||
        !std::isdigit(static_cast<unsigned char>(input[position]))) return false;

    std::uint64_t parsed = 0;
    while (position < input.size() &&
        std::isdigit(static_cast<unsigned char>(input[position])) != 0)
    {
        const unsigned digit = static_cast<unsigned>(input[position++] - '0');
        const std::uint64_t limit =
            static_cast<std::uint64_t>(std::numeric_limits<int>::max()) +
            (negative ? 1U : 0U);
        if (parsed > (limit - digit) / 10U) return false;
        parsed = parsed * 10U + digit;
    }

    if (negative)
    {
        const std::uint64_t negativeLimit =
            static_cast<std::uint64_t>(std::numeric_limits<int>::max()) + 1U;
        if (parsed > negativeLimit) return false;
        value = parsed == negativeLimit
            ? std::numeric_limits<int>::min()
            : -static_cast<int>(parsed);
    }
    else
    {
        value = static_cast<int>(parsed);
    }
    return true;
}

bool parseJsonIntArray(
    const std::string& input,
    std::size_t& position,
    std::vector<int>& values)
{
    skipWhitespace(input, position);
    if (position >= input.size() || input[position] != '[') return false;
    ++position;
    values.clear();
    skipWhitespace(input, position);
    if (position < input.size() && input[position] == ']')
    {
        ++position;
        return true;
    }

    while (position < input.size())
    {
        int value = -1;
        if (!parseJsonInt(input, position, value) || value < 0) return false;
        values.push_back(value);
        if (values.size() > MaximumReplacementFrames) return false;
        skipWhitespace(input, position);
        if (position >= input.size()) return false;
        if (input[position] == ']')
        {
            ++position;
            return true;
        }
        if (input[position] != ',') return false;
        ++position;
    }
    return false;
}

struct MutationBody
{
    Request request;
    RecordingMarksMutationRequest mutation;
};

bool mutationKind(
    const std::string& value,
    RecordingMarksMutationKind& kind)
{
    if (value == "add") kind = RecordingMarksMutationKind::Add;
    else if (value == "delete") kind = RecordingMarksMutationKind::Delete;
    else if (value == "move") kind = RecordingMarksMutationKind::Move;
    else if (value == "reset") kind = RecordingMarksMutationKind::Reset;
    else if (value == "replace") kind = RecordingMarksMutationKind::Replace;
    else return false;
    return true;
}

bool mutationFrameShapeValid(const RecordingMarksMutationRequest& request)
{
    switch (request.kind)
    {
    case RecordingMarksMutationKind::Add:
        return request.sourceFrame < 0 && request.targetFrame >= 0 &&
            request.replacementFrames.empty();
    case RecordingMarksMutationKind::Delete:
        return request.sourceFrame >= 0 && request.targetFrame < 0 &&
            request.replacementFrames.empty();
    case RecordingMarksMutationKind::Move:
        return request.sourceFrame >= 0 && request.targetFrame >= 0 &&
            request.replacementFrames.empty();
    case RecordingMarksMutationKind::Reset:
        return request.sourceFrame < 0 && request.targetFrame < 0 &&
            request.replacementFrames.empty();
    case RecordingMarksMutationKind::Replace:
        return request.sourceFrame < 0 && request.targetFrame < 0 &&
            !request.replacementFrames.empty() &&
            request.replacementFrames.size() <= MaximumReplacementFrames;
    }
    return false;
}

bool parseMutationBody(const std::string& body, MutationBody& parsed)
{
    parsed = {};
    if (body.empty() || body.size() > MaximumMutationBodyBytes) return false;

    std::size_t position = 0;
    skipWhitespace(body, position);
    if (position >= body.size() || body[position] != '{') return false;
    ++position;

    std::set<std::string> seen;
    std::string kindName;
    while (true)
    {
        skipWhitespace(body, position);
        if (position >= body.size()) return false;
        if (body[position] == '}')
        {
            ++position;
            break;
        }

        std::string key;
        if (!parseJsonString(body, position, key, 64U) ||
            !seen.insert(key).second) return false;
        skipWhitespace(body, position);
        if (position >= body.size() || body[position] != ':') return false;
        ++position;

        if (key == "backendId")
        {
            if (!parseJsonString(
                    body, position, parsed.request.backendId,
                    MaximumBackendIdBytes)) return false;
        }
        else if (key == "recordingId")
        {
            if (!parseJsonString(
                    body, position, parsed.request.recordingId,
                    MaximumRecordingIdBytes)) return false;
        }
        else if (key == "operationId")
        {
            if (!parseJsonString(
                    body, position, parsed.mutation.operationId,
                    MaximumOperationIdBytes)) return false;
        }
        else if (key == "operationRevision")
        {
            if (!parseJsonString(
                    body, position, parsed.mutation.operationRevision,
                    MaximumOperationIdBytes)) return false;
        }
        else if (key == "expectedMarksRevision")
        {
            if (!parseJsonString(
                    body, position, parsed.mutation.expectedMarksRevision,
                    32U)) return false;
        }
        else if (key == "kind")
        {
            if (!parseJsonString(body, position, kindName, 16U)) return false;
        }
        else if (key == "sourceFrame")
        {
            if (!parseJsonInt(body, position, parsed.mutation.sourceFrame))
                return false;
        }
        else if (key == "targetFrame")
        {
            if (!parseJsonInt(body, position, parsed.mutation.targetFrame))
                return false;
        }
        else if (key == "replacementFrames")
        {
            if (!parseJsonIntArray(
                    body, position, parsed.mutation.replacementFrames))
                return false;
        }
        else
        {
            return false;
        }

        skipWhitespace(body, position);
        if (position >= body.size()) return false;
        if (body[position] == '}')
        {
            ++position;
            break;
        }
        if (body[position] != ',') return false;
        ++position;
    }

    skipWhitespace(body, position);
    if (position != body.size()) return false;
    if (seen.count("backendId") == 0U ||
        seen.count("recordingId") == 0U ||
        seen.count("operationId") == 0U ||
        seen.count("operationRevision") == 0U ||
        seen.count("expectedMarksRevision") == 0U ||
        seen.count("kind") == 0U ||
        !validBackendId(parsed.request.backendId) ||
        !validRecordingId(parsed.request.recordingId) ||
        !validOperationToken(parsed.mutation.operationId) ||
        !validOperationToken(parsed.mutation.operationRevision) ||
        !validRevisionToken(parsed.mutation.expectedMarksRevision) ||
        !mutationKind(kindName, parsed.mutation.kind))
    {
        return false;
    }

    parsed.mutation.backendId = parsed.request.backendId;
    return mutationFrameShapeValid(parsed.mutation);
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

ApiResponse serializeAssigned(
    const MutationBody& request,
    const RecordingMarksMutationDispatchResult& dispatch)
{
    ApiResponse response;
    response.statusCode = 202;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body =
        "{\"backendId\":\"" + jsonEscape(request.request.backendId) +
        "\",\"recordingId\":\"" + jsonEscape(request.request.recordingId) +
        "\",\"operationId\":\"" + jsonEscape(request.mutation.operationId) +
        "\",\"accepted\":true,\"replayed\":" +
        std::string(dispatch.replayed ? "true" : "false") +
        ",\"state\":\"queued\",\"verification\":\"readback_required\"" +
        ",\"commandId\":\"" + jsonEscape(dispatch.commandId) +
        "\",\"requestFingerprint\":\"" +
        jsonEscape(dispatch.requestFingerprint) + "\"}";
    return response;
}

int dispatchFailureStatus(const std::string& reason)
{
    if (reason.find("conflict") != std::string::npos ||
        reason.find("stale") != std::string::npos)
    {
        return 409;
    }
    if (reason == "invalid_recording_marks_modify_assignment_request")
        return 400;
    return 503;
}

bool resolveNativeIdentity(
    const Request& request,
    const RecordingMarksApiRuntime::RecordingLookup& recordingLookup,
    std::string& recordingKey,
    ApiResponse& response)
{
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
        return false;
    }
    if (matches != 1U)
    {
        response = errorResponse(409, "recording_identity_ambiguous");
        return false;
    }
    if (selected->backendNativeId.empty() ||
        selected->backendNativeId.size() >
            VdrRecordingNativeIdentity::MaximumNativeIdBytes)
    {
        response = errorResponse(409, "recording_native_identity_unavailable");
        return false;
    }

    recordingKey = VdrRecordingNativeIdentity::keyForNativeId(
        selected->backendNativeId);
    if (!VdrRecordingNativeIdentity::isValidKey(recordingKey))
    {
        response = errorResponse(409, "recording_native_identity_unavailable");
        return false;
    }
    return true;
}

bool resolveBackendAccess(
    const Request& request,
    const RecordingMarksApiRuntime::BackendResolver& backendResolver,
    RecordingMarksBackendAccess& access,
    ApiResponse& response)
{
    access = backendResolver(request.backendId);
    if (access.availability == RecordingMarksBackendAvailability::BackendNotFound)
    {
        response = errorResponse(404, "backend_not_found");
        return false;
    }
    if (access.availability != RecordingMarksBackendAvailability::Available ||
        access.resolver == nullptr)
    {
        response = errorResponse(503, "recording_marks_capability_unavailable");
        return false;
    }
    return true;
}

bool validateNativeMarks(
    const std::string& recordingKey,
    const RecordingMarksBackendAccess& access,
    VdrRecordingNativeMarks& nativeMarks,
    ApiResponse& response)
{
    nativeMarks = access.resolver->resolve(recordingKey);
    if (!nativeMarks.recordingKey.empty() &&
        nativeMarks.recordingKey != recordingKey)
    {
        response = errorResponse(502, "recording_marks_invalid_native_payload");
        return false;
    }

    switch (nativeMarks.availability)
    {
    case VdrRecordingNativeMarksAvailability::Available:
        return true;
    case VdrRecordingNativeMarksAvailability::RecordingNotFound:
        response = errorResponse(409, "recording_native_state_stale");
        return false;
    case VdrRecordingNativeMarksAvailability::NativeUnreadable:
        response = errorResponse(503, "recording_marks_unreadable");
        return false;
    case VdrRecordingNativeMarksAvailability::TransportError:
        response = errorResponse(503, "recording_marks_transport_unavailable");
        return false;
    case VdrRecordingNativeMarksAvailability::InvalidPayload:
        response = errorResponse(502, "recording_marks_invalid_native_payload");
        return false;
    }

    response = errorResponse(502, "recording_marks_invalid_native_payload");
    return false;
}

bool validAcceptedDispatch(
    const RecordingMarksMutationDispatchResult& dispatch)
{
    return dispatch.accepted &&
        !dispatch.commandId.empty() &&
        !dispatch.requestFingerprint.empty();
}
}

RecordingMarksApiRuntime& RecordingMarksApiRuntime::instance()
{
    static RecordingMarksApiRuntime runtime;
    return runtime;
}

bool RecordingMarksApiRuntime::configure(
    RecordingLookup recordingLookup,
    BackendResolver backendResolver,
    BackendWritePolicy backendWritePolicy,
    MutationDispatcher mutationDispatcher)
{
    if (!recordingLookup || !backendResolver) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    recordingLookup_ = std::move(recordingLookup);
    backendResolver_ = std::move(backendResolver);
    backendWritePolicy_ = std::move(backendWritePolicy);
    mutationDispatcher_ = std::move(mutationDispatcher);
    return true;
}

void RecordingMarksApiRuntime::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    recordingLookup_ = {};
    backendResolver_ = {};
    backendWritePolicy_ = {};
    mutationDispatcher_ = {};
}

bool RecordingMarksApiRuntime::configured() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<bool>(recordingLookup_) &&
        static_cast<bool>(backendResolver_);
}

bool RecordingMarksApiRuntime::mutationConfigured() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<bool>(recordingLookup_) &&
        static_cast<bool>(backendResolver_) &&
        static_cast<bool>(backendWritePolicy_) &&
        static_cast<bool>(mutationDispatcher_);
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

    std::string recordingKey;
    if (!resolveNativeIdentity(request, recordingLookup, recordingKey, response))
        return true;

    RecordingMarksBackendAccess access;
    if (!resolveBackendAccess(request, backendResolver, access, response))
        return true;

    VdrRecordingNativeMarks nativeMarks;
    if (!validateNativeMarks(recordingKey, access, nativeMarks, response))
        return true;

    response = serializeAvailable(request, nativeMarks);
    return true;
}

bool RecordingMarksApiRuntime::tryHandlePost(
    const std::string& requestTarget,
    const std::string& body,
    ApiResponse& response) const
{
    if (requestPath(requestTarget) != RecordingMarksRoute) return false;
    if (requestTarget != RecordingMarksRoute)
    {
        response = errorResponse(400, "recording_marks_mutation_request_invalid");
        return true;
    }

    MutationBody request;
    if (!parseMutationBody(body, request))
    {
        response = errorResponse(400, "recording_marks_mutation_request_invalid");
        return true;
    }

    RecordingLookup recordingLookup;
    BackendResolver backendResolver;
    BackendWritePolicy backendWritePolicy;
    MutationDispatcher mutationDispatcher;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        recordingLookup = recordingLookup_;
        backendResolver = backendResolver_;
        backendWritePolicy = backendWritePolicy_;
        mutationDispatcher = mutationDispatcher_;
    }

    if (!recordingLookup || !backendResolver ||
        !backendWritePolicy || !mutationDispatcher)
    {
        response = errorResponse(503, "recording_marks_mutation_runtime_unavailable");
        return true;
    }

    const RecordingMarksBackendWriteAccess writeAccess =
        backendWritePolicy(request.request.backendId);
    if (!writeAccess.allowed)
    {
        const int status = writeAccess.statusCode >= 400 &&
                writeAccess.statusCode <= 599
            ? writeAccess.statusCode
            : 503;
        response = errorResponse(
            status,
            writeAccess.reasonCode.empty()
                ? "recording_marks_backend_write_unavailable"
                : writeAccess.reasonCode);
        return true;
    }

    std::string recordingKey;
    if (!resolveNativeIdentity(
            request.request, recordingLookup, recordingKey, response))
        return true;

    RecordingMarksBackendAccess access;
    if (!resolveBackendAccess(request.request, backendResolver, access, response))
        return true;

    VdrRecordingNativeMarks nativeMarks;
    if (!validateNativeMarks(recordingKey, access, nativeMarks, response))
        return true;

    request.mutation.recordingKey = recordingKey;

    const bool recordingInUse = nativeMarks.inUseFlags != 0;
    const bool revisionConflict =
        nativeMarks.marksRevision != request.mutation.expectedMarksRevision;
    if (recordingInUse || revisionConflict)
    {
        RecordingMarksMutationRequest replayRequest = request.mutation;
        replayRequest.replayOnly = true;
        const RecordingMarksMutationDispatchResult replay =
            mutationDispatcher(replayRequest);

        if (validAcceptedDispatch(replay))
        {
            if (!replay.replayed)
            {
                response = errorResponse(
                    502,
                    "recording_marks_mutation_replay_probe_invalid");
                return true;
            }
            response = serializeAssigned(request, replay);
            return true;
        }

        if (!replay.reasonCode.empty() &&
            replay.reasonCode != ReplayNotFoundReason)
        {
            response = errorResponse(
                dispatchFailureStatus(replay.reasonCode),
                replay.reasonCode);
            return true;
        }

        response = errorResponse(
            409,
            recordingInUse
                ? "recording_in_use"
                : "recording_marks_revision_conflict");
        return true;
    }

    const RecordingMarksMutationDispatchResult dispatch =
        mutationDispatcher(request.mutation);
    if (!dispatch.accepted)
    {
        response = errorResponse(
            dispatchFailureStatus(dispatch.reasonCode),
            dispatch.reasonCode.empty()
                ? "recording_marks_mutation_dispatch_failed"
                : dispatch.reasonCode);
        return true;
    }
    if (!validAcceptedDispatch(dispatch))
    {
        response = errorResponse(502, "recording_marks_mutation_dispatch_invalid");
        return true;
    }

    response = serializeAssigned(request, dispatch);
    return true;
}
#include "RecordingCutApiRuntime.h"

#include "VdrRecordingNativeIdentity.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>

namespace
{
constexpr const char* RecordingCutRoute = "/api/vdr/recordings/cut";
constexpr std::size_t MaximumBackendIdBytes = 128U;
constexpr std::size_t MaximumRecordingIdBytes = 4096U;
constexpr std::size_t MaximumMutationBodyBytes = 8192U;
constexpr std::size_t MaximumOperationIdBytes = 192U;
constexpr const char* ReplayNotFoundReason =
    "recording_cut_assignment_not_found";

std::string requestPath(const std::string& target)
{
    const std::size_t query = target.find('?');
    return query == std::string::npos ? target : target.substr(0, query);
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
    if (requestPath(target) != RecordingCutRoute) return false;

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
            end == std::string::npos ? std::string::npos : end - start);
        if (item.empty()) return false;

        const std::size_t equals = item.find('=');
        if (equals == std::string::npos || equals == 0U) return false;
        const std::string key = item.substr(0, equals);
        if (key != "backend" && key != "recordingId") return false;
        if (parameters.count(key) != 0U) return false;

        std::string decoded;
        const std::size_t maximum =
            key == "backend" ? MaximumBackendIdBytes : MaximumRecordingIdBytes;
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

struct StartBody
{
    Request request;
    RecordingCutStartRequest start;
};

bool parseStartBody(const std::string& body, StartBody& parsed)
{
    parsed = {};
    if (body.empty() || body.size() > MaximumMutationBodyBytes) return false;

    std::size_t position = 0;
    skipWhitespace(body, position);
    if (position >= body.size() || body[position] != '{') return false;
    ++position;

    std::set<std::string> seen;
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
                    body, position, parsed.start.operationId,
                    MaximumOperationIdBytes)) return false;
        }
        else if (key == "operationRevision")
        {
            if (!parseJsonString(
                    body, position, parsed.start.operationRevision,
                    MaximumOperationIdBytes)) return false;
        }
        else if (key == "expectedMarksRevision")
        {
            if (!parseJsonString(
                    body, position, parsed.start.expectedMarksRevision,
                    32U)) return false;
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
    if (seen.size() != 5U ||
        seen.count("backendId") == 0U ||
        seen.count("recordingId") == 0U ||
        seen.count("operationId") == 0U ||
        seen.count("operationRevision") == 0U ||
        seen.count("expectedMarksRevision") == 0U ||
        !validBackendId(parsed.request.backendId) ||
        !validRecordingId(parsed.request.recordingId) ||
        !validOperationToken(parsed.start.operationId) ||
        !validOperationToken(parsed.start.operationRevision) ||
        !validRevisionToken(parsed.start.expectedMarksRevision))
    {
        return false;
    }

    parsed.start.backendId = parsed.request.backendId;
    return true;
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

ApiResponse errorResponse(int statusCode, const std::string& code)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body =
        "{\"error\":{\"code\":\"" + jsonEscape(code) + "\"}}";
    return response;
}

std::string preconditionReasonCode(const std::string& nativeReason)
{
    std::string result = "recording_cut_precondition_";
    for (const unsigned char character : nativeReason)
        result.push_back(character == '-' ? '_' : static_cast<char>(character));
    return result;
}

ApiResponse serializeAvailable(
    const Request& request,
    const VdrRecordingNativeCutState& state)
{
    std::ostringstream json;
    json << "{\"backendId\":\"" << jsonEscape(request.backendId)
         << "\",\"recordingId\":\"" << jsonEscape(request.recordingId)
         << "\",\"availability\":\"available\""
         << ",\"ready\":" << (state.ready ? "true" : "false")
         << ",\"reason\":\"" << jsonEscape(state.reason) << "\""
         << ",\"marksReadable\":" << (state.marksReadable ? "true" : "false")
         << ",\"marksRevision\":\"" << jsonEscape(state.marksRevision) << "\""
         << ",\"marksFilePresent\":"
         << (state.marksFilePresent ? "true" : "false")
         << ",\"markCount\":" << state.markCount
         << ",\"sequenceCount\":" << state.sequenceCount
         << ",\"inUse\":" << (state.inUseFlags != 0 ? "true" : "false")
         << ",\"inUseFlags\":" << state.inUseFlags
         << ",\"handlerUsage\":" << state.handlerUsage
         << ",\"editedDestinationExists\":"
         << (state.editedDestinationExists ? "true" : "false")
         << ",\"editedRecordingFound\":"
         << (state.editedRecordingFound ? "true" : "false");
    if (state.editedRecordingFound)
    {
        json << ",\"editedRecordingKey\":\""
             << jsonEscape(state.editedRecordingKey) << "\"";
    }
    json << "}";

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body = json.str();
    return response;
}

ApiResponse serializeAssigned(
    const StartBody& request,
    const RecordingCutDispatchResult& dispatch)
{
    ApiResponse response;
    response.statusCode = dispatch.verified ? 200 : 202;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body =
        "{\"backendId\":\"" + jsonEscape(request.request.backendId) +
        "\",\"recordingId\":\"" + jsonEscape(request.request.recordingId) +
        "\",\"operationId\":\"" + jsonEscape(request.start.operationId) +
        "\",\"accepted\":true,\"replayed\":" +
        std::string(dispatch.replayed ? "true" : "false") +
        ",\"state\":\"" +
        std::string(dispatch.verified ? "verified" : "queued") +
        "\",\"verification\":\"" +
        std::string(dispatch.verified ? "verified" : "readback_required") + "\"" +
        ",\"commandId\":\"" + jsonEscape(dispatch.commandId) +
        "\",\"requestFingerprint\":\"" +
        jsonEscape(dispatch.requestFingerprint) + "\"" +
        (dispatch.verified
            ? ",\"editedRecordingKey\":\"" +
                jsonEscape(dispatch.editedRecordingKey) + "\""
            : std::string()) +
        "}";
    return response;
}

int dispatchFailureStatus(const std::string& reason)
{
    if (reason.find("conflict") != std::string::npos ||
        reason.find("stale") != std::string::npos)
        return 409;
    if (reason == "invalid_recording_cut_assignment_request") return 400;
    return 503;
}

bool resolveNativeIdentity(
    const Request& request,
    const RecordingCutApiRuntime::RecordingLookup& recordingLookup,
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
            continue;
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
    const RecordingCutApiRuntime::BackendResolver& backendResolver,
    RecordingCutBackendAccess& access,
    ApiResponse& response)
{
    access = backendResolver(request.backendId);
    if (access.availability == RecordingCutBackendAvailability::BackendNotFound)
    {
        response = errorResponse(404, "backend_not_found");
        return false;
    }
    if (access.availability != RecordingCutBackendAvailability::Available ||
        access.resolver == nullptr)
    {
        response = errorResponse(503, "recording_cut_capability_unavailable");
        return false;
    }
    return true;
}

bool validateNativeState(
    const std::string& recordingKey,
    const RecordingCutBackendAccess& access,
    VdrRecordingNativeCutState& state,
    ApiResponse& response)
{
    state = access.resolver->resolve(recordingKey);
    if (!state.recordingKey.empty() && state.recordingKey != recordingKey)
    {
        response = errorResponse(502, "recording_cut_invalid_native_payload");
        return false;
    }

    switch (state.availability)
    {
    case VdrRecordingNativeCutStateAvailability::Available:
        return true;
    case VdrRecordingNativeCutStateAvailability::RecordingNotFound:
    case VdrRecordingNativeCutStateAvailability::IdentityAmbiguous:
        response = errorResponse(409, "recording_native_state_stale");
        return false;
    case VdrRecordingNativeCutStateAvailability::TransportError:
        response = errorResponse(503, "recording_cut_transport_unavailable");
        return false;
    case VdrRecordingNativeCutStateAvailability::InvalidPayload:
        response = errorResponse(502, "recording_cut_invalid_native_payload");
        return false;
    }

    response = errorResponse(502, "recording_cut_invalid_native_payload");
    return false;
}

bool validAcceptedDispatch(const RecordingCutDispatchResult& dispatch)
{
    return dispatch.accepted &&
        !dispatch.commandId.empty() &&
        !dispatch.requestFingerprint.empty() &&
        (!dispatch.verified ||
            (dispatch.replayed &&
             VdrRecordingNativeIdentity::isValidKey(
                 dispatch.editedRecordingKey)));
}
}

RecordingCutApiRuntime& RecordingCutApiRuntime::instance()
{
    static RecordingCutApiRuntime runtime;
    return runtime;
}

bool RecordingCutApiRuntime::configure(
    RecordingLookup recordingLookup,
    BackendResolver backendResolver,
    BackendWritePolicy backendWritePolicy,
    StartDispatcher startDispatcher)
{
    if (!recordingLookup || !backendResolver) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    recordingLookup_ = std::move(recordingLookup);
    backendResolver_ = std::move(backendResolver);
    backendWritePolicy_ = std::move(backendWritePolicy);
    startDispatcher_ = std::move(startDispatcher);
    return true;
}

void RecordingCutApiRuntime::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    recordingLookup_ = {};
    backendResolver_ = {};
    backendWritePolicy_ = {};
    startDispatcher_ = {};
}

bool RecordingCutApiRuntime::configured() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<bool>(recordingLookup_) &&
        static_cast<bool>(backendResolver_);
}

bool RecordingCutApiRuntime::mutationConfigured() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<bool>(recordingLookup_) &&
        static_cast<bool>(backendResolver_) &&
        static_cast<bool>(backendWritePolicy_) &&
        static_cast<bool>(startDispatcher_);
}

bool RecordingCutApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    ApiResponse& response) const
{
    if (requestPath(requestTarget) != RecordingCutRoute) return false;

    Request request;
    if (!parseRequest(requestTarget, request))
    {
        response = errorResponse(400, "recording_cut_request_invalid");
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
        response = errorResponse(503, "recording_cut_runtime_unavailable");
        return true;
    }

    std::string recordingKey;
    if (!resolveNativeIdentity(request, recordingLookup, recordingKey, response))
        return true;

    RecordingCutBackendAccess access;
    if (!resolveBackendAccess(request, backendResolver, access, response))
        return true;

    VdrRecordingNativeCutState state;
    if (!validateNativeState(recordingKey, access, state, response))
        return true;

    response = serializeAvailable(request, state);
    return true;
}

bool RecordingCutApiRuntime::tryHandlePost(
    const std::string& requestTarget,
    const std::string& body,
    ApiResponse& response) const
{
    if (requestPath(requestTarget) != RecordingCutRoute) return false;
    if (requestTarget != RecordingCutRoute)
    {
        response = errorResponse(400, "recording_cut_start_request_invalid");
        return true;
    }

    StartBody request;
    if (!parseStartBody(body, request))
    {
        response = errorResponse(400, "recording_cut_start_request_invalid");
        return true;
    }

    RecordingLookup recordingLookup;
    BackendResolver backendResolver;
    BackendWritePolicy backendWritePolicy;
    StartDispatcher startDispatcher;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        recordingLookup = recordingLookup_;
        backendResolver = backendResolver_;
        backendWritePolicy = backendWritePolicy_;
        startDispatcher = startDispatcher_;
    }

    if (!recordingLookup || !backendResolver ||
        !backendWritePolicy || !startDispatcher)
    {
        response = errorResponse(503, "recording_cut_start_runtime_unavailable");
        return true;
    }

    const RecordingCutBackendWriteAccess writeAccess =
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
                ? "recording_cut_backend_write_unavailable"
                : writeAccess.reasonCode);
        return true;
    }

    std::string recordingKey;
    if (!resolveNativeIdentity(
            request.request, recordingLookup, recordingKey, response))
        return true;

    RecordingCutBackendAccess access;
    if (!resolveBackendAccess(request.request, backendResolver, access, response))
        return true;

    VdrRecordingNativeCutState state;
    if (!validateNativeState(recordingKey, access, state, response))
        return true;

    request.start.recordingKey = recordingKey;

    const bool revisionConflict =
        state.marksRevision != request.start.expectedMarksRevision;
    if (!state.ready || revisionConflict)
    {
        RecordingCutStartRequest replayRequest = request.start;
        replayRequest.replayOnly = true;
        const RecordingCutDispatchResult replay =
            startDispatcher(replayRequest);

        if (validAcceptedDispatch(replay))
        {
            if (!replay.replayed)
            {
                response = errorResponse(
                    502, "recording_cut_replay_probe_invalid");
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
            revisionConflict
                ? "recording_cut_marks_revision_conflict"
                : preconditionReasonCode(state.reason));
        return true;
    }

    const RecordingCutDispatchResult dispatch =
        startDispatcher(request.start);
    if (!dispatch.accepted)
    {
        response = errorResponse(
            dispatchFailureStatus(dispatch.reasonCode),
            dispatch.reasonCode.empty()
                ? "recording_cut_dispatch_failed"
                : dispatch.reasonCode);
        return true;
    }
    if (!validAcceptedDispatch(dispatch))
    {
        response = errorResponse(502, "recording_cut_dispatch_invalid");
        return true;
    }

    response = serializeAssigned(request, dispatch);
    return true;
}

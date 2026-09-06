#include "RecordingCutApiRuntime.h"

#include "VdrRecordingNativeIdentity.h"

#include <cassert>
#include <string>
#include <utility>
#include <vector>

namespace
{
class FakeCutStateResolver final : public IVdrRecordingNativeCutStateResolver
{
public:
    VdrRecordingNativeCutState state;
    int calls = 0;

    VdrRecordingNativeCutState resolve(const std::string& recordingKey) override
    {
        ++calls;
        VdrRecordingNativeCutState result = state;
        result.recordingKey = recordingKey;
        return result;
    }
};

VdrRecording recording()
{
    VdrRecording value;
    value.id = "recording-1";
    value.backendId = "default";
    value.backendNativeId = "/srv/vdr/video/Test/2026-09-06.12.00.00-0.rec";
    return value;
}

VdrRecordingNativeCutState readyState()
{
    VdrRecordingNativeCutState state;
    state.availability = VdrRecordingNativeCutStateAvailability::Available;
    state.found = true;
    state.reason = "ready";
    state.ready = true;
    state.marksReadable = true;
    state.marksRevision = "0123456789abcdef0123456789abcdef";
    state.marksFilePresent = true;
    state.markCount = 4;
    state.sequenceCount = 2;
    state.editedRecordingKey = "fedcba9876543210fedcba9876543210";
    return state;
}

std::string body(const std::string& revision =
    "0123456789abcdef0123456789abcdef")
{
    return
        "{\"backendId\":\"default\","
        "\"recordingId\":\"recording-1\","
        "\"operationId\":\"cut-op-1\","
        "\"operationRevision\":\"1\","
        "\"expectedMarksRevision\":\"" + revision + "\"}";
}
}

int main()
{
    RecordingCutApiRuntime& runtime = RecordingCutApiRuntime::instance();
    runtime.reset();

    FakeCutStateResolver resolver;
    resolver.state = readyState();

    std::vector<RecordingCutStartRequest> dispatched;
    const bool configured = runtime.configure(
        [](const std::string& backendId) {
            return backendId == "default"
                ? std::vector<VdrRecording>{recording()}
                : std::vector<VdrRecording>{};
        },
        [&resolver](const std::string& backendId) {
            return backendId == "default"
                ? RecordingCutBackendAccess{
                    RecordingCutBackendAvailability::Available, &resolver}
                : RecordingCutBackendAccess{
                    RecordingCutBackendAvailability::BackendNotFound, nullptr};
        },
        [](const std::string& backendId) {
            RecordingCutBackendWriteAccess access;
            access.allowed = backendId == "default";
            access.statusCode = access.allowed ? 200 : 404;
            access.reasonCode = access.allowed
                ? "recording_cut_backend_write_allowed"
                : "backend_not_found";
            return access;
        },
        [&dispatched](const RecordingCutStartRequest& request) {
            dispatched.push_back(request);
            RecordingCutDispatchResult result;
            if (request.replayOnly)
            {
                result.reasonCode = "recording_cut_assignment_not_found";
                return result;
            }
            result.accepted = true;
            result.commandId = "cmd_cut_1";
            result.requestFingerprint = "fp1:cut";
            result.reasonCode = "recording_cut_assigned";
            return result;
        });
    assert(configured);
    assert(runtime.configured());
    assert(runtime.mutationConfigured());

    ApiResponse response;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/cut?backend=default&recordingId=recording-1",
        response));
    assert(response.statusCode == 200);
    assert(response.body.find("\"ready\":true") != std::string::npos);
    assert(response.body.find("\"sequenceCount\":2") != std::string::npos);
    assert(response.body.find(
        "\"marksRevision\":\"0123456789abcdef0123456789abcdef\"") !=
        std::string::npos);

    response = {};
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/cut", body(), response));
    assert(response.statusCode == 202);
    assert(dispatched.size() == 1U);
    assert(!dispatched.back().replayOnly);
    assert(dispatched.back().backendId == "default");
    assert(dispatched.back().expectedMarksRevision ==
        "0123456789abcdef0123456789abcdef");
    assert(VdrRecordingNativeIdentity::isValidKey(
        dispatched.back().recordingKey));
    assert(response.body.find("\"state\":\"queued\"") != std::string::npos);

    response = {};
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/cut",
        body("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
        response));
    assert(response.statusCode == 409);
    assert(dispatched.size() == 2U);
    assert(dispatched.back().replayOnly);
    assert(response.body.find(
        "recording_cut_marks_revision_conflict") != std::string::npos);

    resolver.state = readyState();
    resolver.state.ready = false;
    resolver.state.reason = "recording-handler-busy";
    resolver.state.handlerUsage = 4;
    response = {};
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/cut", body(), response));
    assert(response.statusCode == 409);
    assert(dispatched.size() == 3U);
    assert(dispatched.back().replayOnly);
    assert(response.body.find(
        "recording_cut_precondition_recording_handler_busy") !=
        std::string::npos);

    runtime.reset();
    dispatched.clear();
    resolver.state = readyState();
    assert(runtime.configure(
        [](const std::string&) {
            return std::vector<VdrRecording>{recording()};
        },
        [&resolver](const std::string&) {
            return RecordingCutBackendAccess{
                RecordingCutBackendAvailability::Available, &resolver};
        },
        [](const std::string&) {
            RecordingCutBackendWriteAccess access;
            access.allowed = false;
            access.statusCode = 403;
            access.reasonCode = "backend_read_only";
            return access;
        },
        [&dispatched](const RecordingCutStartRequest& request) {
            dispatched.push_back(request);
            RecordingCutDispatchResult result;
            result.accepted = true;
            result.commandId = "must-not-dispatch";
            result.requestFingerprint = "must-not-dispatch";
            return result;
        }));

    response = {};
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/cut", body(), response));
    assert(response.statusCode == 403);
    assert(dispatched.empty());
    assert(response.body.find("backend_read_only") != std::string::npos);

    runtime.reset();
    dispatched.clear();
    resolver.state = readyState();
    resolver.state.ready = false;
    resolver.state.reason = "edited-destination-exists";
    resolver.state.editedDestinationExists = true;
    resolver.state.editedRecordingFound = true;
    const std::string editedKey = resolver.state.editedRecordingKey;

    assert(runtime.configure(
        [](const std::string&) {
            return std::vector<VdrRecording>{recording()};
        },
        [&resolver](const std::string&) {
            return RecordingCutBackendAccess{
                RecordingCutBackendAvailability::Available, &resolver};
        },
        [](const std::string&) {
            RecordingCutBackendWriteAccess access;
            access.allowed = true;
            access.statusCode = 200;
            return access;
        },
        [editedKey, &dispatched](const RecordingCutStartRequest& request) {
            dispatched.push_back(request);
            RecordingCutDispatchResult result;
            assert(request.replayOnly);
            result.accepted = true;
            result.replayed = true;
            result.verified = true;
            result.commandId = "cmd_cut_1";
            result.requestFingerprint = "fp1:cut";
            result.editedRecordingKey = editedKey;
            result.reasonCode = "recording_cut_verified_replayed";
            return result;
        }));

    response = {};
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/cut", body(), response));
    assert(response.statusCode == 200);
    assert(dispatched.size() == 1U);
    assert(dispatched.front().replayOnly);
    assert(response.body.find("\"state\":\"verified\"") != std::string::npos);
    assert(response.body.find("\"replayed\":true") != std::string::npos);
    assert(response.body.find(editedKey) != std::string::npos);

    response = {};
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/cut",
        body("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
        response));
    assert(response.statusCode == 200);
    assert(dispatched.size() == 2U);
    assert(dispatched.back().replayOnly);

    response = {};
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/cut",
        "{\"backendId\":\"default\",\"recordingId\":\"recording-1\","
        "\"operationId\":\"bad op\",\"operationRevision\":\"1\","
        "\"expectedMarksRevision\":\"0123456789abcdef0123456789abcdef\"}",
        response));
    assert(response.statusCode == 400);

    runtime.reset();
    return 0;
}

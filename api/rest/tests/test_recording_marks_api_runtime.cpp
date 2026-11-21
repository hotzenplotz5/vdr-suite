#include "RecordingMarksApiRuntime.h"

#include "VdrRecordingNativeIdentity.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
class FakeResolver final : public IVdrRecordingNativeMarksResolver
{
public:
    VdrRecordingNativeMarks resolve(
        const std::string& recordingKey) override
    {
        ++calls;
        lastKey = recordingKey;
        VdrRecordingNativeMarks result = next;
        if (result.recordingKey.empty()) result.recordingKey = recordingKey;
        return result;
    }

    int calls = 0;
    std::string lastKey;
    VdrRecordingNativeMarks next;
};

VdrRecording makeRecording(
    const std::string& id = "7")
{
    VdrRecording recording;
    recording.id = id;
    recording.backendId = "default";
    recording.backendNativeId =
        "/srv/vdr/video/Movies/Test/2026-09-04.07.00.1-0.rec";
    recording.title = "Test";
    return recording;
}

VdrRecordingNativeMarks availableMarks()
{
    VdrRecordingNativeMarks marks;
    marks.availability = VdrRecordingNativeMarksAvailability::Available;
    marks.schema = 1;
    marks.found = true;
    marks.reason = "none";
    marks.recordingIdentitySchema = 1;
    marks.state = "present";
    marks.framesPerSecond = 25.0;
    marks.isPesRecording = false;
    marks.inUseFlags = 2;
    marks.marksFilePresent = true;
    marks.sequenceCount = 1;
    marks.marksRevision = "0123456789abcdef0123456789abcdef";
    VdrRecordingNativeMark mark;
    mark.positionFrame = 250;
    mark.timecode = "0:00:10.00";
    mark.positionSeconds = 10.0;
    mark.comment = "begin";
    marks.marks.push_back(mark);
    return marks;
}

std::string addMutationBody(
    const std::string& expectedRevision =
        "0123456789abcdef0123456789abcdef")
{
    return
        "{\"backendId\":\"default\","
        "\"recordingId\":\"7\","
        "\"operationId\":\"marks-op-1\","
        "\"operationRevision\":\"rev-1\","
        "\"expectedMarksRevision\":\"" + expectedRevision + "\","
        "\"kind\":\"add\","
        "\"targetFrame\":500}";
}
}

int main()
{
    RecordingMarksApiRuntime& runtime = RecordingMarksApiRuntime::instance();
    runtime.reset();

    ApiResponse response;
    assert(!runtime.tryHandleGet("/api/vdr/recordings/query", response));
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 503);

    FakeResolver resolver;
    resolver.next = availableMarks();
    std::vector<VdrRecording> recordings{makeRecording()};

    RecordingMarksBackendAvailability backendAvailability =
        RecordingMarksBackendAvailability::Available;
    int lookupCalls = 0;
    int backendCalls = 0;

    assert(runtime.configure(
        [&](const std::string& backendId) {
            ++lookupCalls;
            assert(backendId == "default");
            return recordings;
        },
        [&](const std::string& backendId) {
            ++backendCalls;
            assert(backendId == "default");
            RecordingMarksBackendAccess access;
            access.availability = backendAvailability;
            access.resolver =
                backendAvailability == RecordingMarksBackendAvailability::Available
                    ? &resolver
                    : nullptr;
            return access;
        }));
    assert(runtime.configured());
    assert(!runtime.mutationConfigured());

    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 200);
    assert(response.headers.at("Cache-Control") == "no-store");
    assert(response.body.find("\"backendId\":\"default\"") != std::string::npos);
    assert(response.body.find("\"recordingId\":\"7\"") != std::string::npos);
    assert(response.body.find("\"inUse\":true") != std::string::npos);
    assert(response.body.find("\"positionFrame\":250") != std::string::npos);
    assert(response.body.find("\"positionSeconds\":10") != std::string::npos);
    assert(response.body.find("0123456789abcdef0123456789abcdef") != std::string::npos);
    assert(response.body.find("/srv/vdr") == std::string::npos);
    assert(response.body.find("backendNativeId") == std::string::npos);
    assert(response.body.find("recordingKey") == std::string::npos);
    assert(resolver.calls == 1);
    assert(resolver.lastKey ==
        VdrRecordingNativeIdentity::keyForNativeId(
            recordings.front().backendNativeId));
    assert(lookupCalls == 1);
    assert(backendCalls == 1);

    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=%37",
        response));
    assert(response.statusCode == 200);
    assert(resolver.calls == 2);

    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default",
        response));
    assert(response.statusCode == 400);
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7&backendNativeId=%2Fsrv%2Fvdr",
        response));
    assert(response.statusCode == 400);
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7&path=%2Fsrv%2Fvdr",
        response));
    assert(response.statusCode == 400);
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7&recordingId=8",
        response));
    assert(response.statusCode == 400);

    recordings.clear();
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 404);
    assert(resolver.calls == 2);

    recordings = {makeRecording(), makeRecording()};
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 409);
    assert(resolver.calls == 2);

    recordings = {makeRecording()};
    backendAvailability = RecordingMarksBackendAvailability::CapabilityUnavailable;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 503);
    assert(resolver.calls == 2);

    backendAvailability = RecordingMarksBackendAvailability::BackendNotFound;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 404);

    backendAvailability = RecordingMarksBackendAvailability::Available;
    resolver.next = availableMarks();
    resolver.next.availability =
        VdrRecordingNativeMarksAvailability::RecordingNotFound;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 409);

    resolver.next = availableMarks();
    resolver.next.availability =
        VdrRecordingNativeMarksAvailability::NativeUnreadable;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 503);

    resolver.next = availableMarks();
    resolver.next.availability =
        VdrRecordingNativeMarksAvailability::TransportError;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 503);

    resolver.next = availableMarks();
    resolver.next.availability =
        VdrRecordingNativeMarksAvailability::InvalidPayload;
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 502);

    resolver.next = availableMarks();
    resolver.next.recordingKey =
        "ffffffffffffffffffffffffffffffff";
    assert(runtime.tryHandleGet(
        "/api/vdr/recordings/marks?backend=default&recordingId=7",
        response));
    assert(response.statusCode == 502);

    assert(!runtime.tryHandlePost("/api/vdr/recordings/query", "{}", response));
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks",
        addMutationBody(),
        response));
    assert(response.statusCode == 503);
    assert(response.body.find("recording_marks_mutation_runtime_unavailable") !=
        std::string::npos);

    RecordingMarksBackendWriteAccess writeAccess;
    writeAccess.allowed = true;
    writeAccess.statusCode = 200;
    writeAccess.reasonCode.clear();
    RecordingMarksMutationDispatchResult dispatchResult;
    dispatchResult.accepted = true;
    dispatchResult.commandId = "cmd-marks-1";
    dispatchResult.requestFingerprint = "fp-marks-1";
    int writePolicyCalls = 0;
    int dispatchCalls = 0;
    int replayProbeCalls = 0;
    bool replayExists = false;
    RecordingMarksMutationRequest lastMutation;

    assert(runtime.configure(
        [&](const std::string& backendId) {
            ++lookupCalls;
            assert(backendId == "default");
            return recordings;
        },
        [&](const std::string& backendId) {
            ++backendCalls;
            assert(backendId == "default");
            RecordingMarksBackendAccess access;
            access.availability = backendAvailability;
            access.resolver = &resolver;
            return access;
        },
        [&](const std::string& backendId) {
            ++writePolicyCalls;
            assert(backendId == "default");
            return writeAccess;
        },
        [&](const RecordingMarksMutationRequest& request) {
            lastMutation = request;
            if (request.replayOnly)
            {
                ++replayProbeCalls;
                if (!replayExists)
                {
                    RecordingMarksMutationDispatchResult missing;
                    missing.reasonCode =
                        "recording_marks_modify_assignment_not_found";
                    return missing;
                }
            }
            else
            {
                ++dispatchCalls;
            }
            return dispatchResult;
        }));
    assert(runtime.mutationConfigured());

    resolver.next = availableMarks();
    resolver.next.inUseFlags = 0;

    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks?backend=default",
        addMutationBody(),
        response));
    assert(response.statusCode == 400);
    assert(dispatchCalls == 0);
    assert(replayProbeCalls == 0);

    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks",
        "{}",
        response));
    assert(response.statusCode == 400);
    assert(dispatchCalls == 0);
    assert(replayProbeCalls == 0);

    writeAccess.allowed = false;
    writeAccess.statusCode = 403;
    writeAccess.reasonCode = "recording_marks_write_forbidden";
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks",
        addMutationBody(),
        response));
    assert(response.statusCode == 403);
    assert(response.body.find("recording_marks_write_forbidden") != std::string::npos);
    assert(dispatchCalls == 0);
    assert(replayProbeCalls == 0);

    writeAccess.allowed = true;
    writeAccess.statusCode = 200;
    writeAccess.reasonCode.clear();
    resolver.next = availableMarks();
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks",
        addMutationBody(),
        response));
    assert(response.statusCode == 409);
    assert(response.body.find("recording_in_use") != std::string::npos);
    assert(dispatchCalls == 0);
    assert(replayProbeCalls == 1);
    assert(lastMutation.replayOnly);

    resolver.next = availableMarks();
    resolver.next.inUseFlags = 0;
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks",
        addMutationBody("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
        response));
    assert(response.statusCode == 409);
    assert(response.body.find("recording_marks_revision_conflict") != std::string::npos);
    assert(dispatchCalls == 0);
    assert(replayProbeCalls == 2);
    assert(lastMutation.replayOnly);

    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks",
        addMutationBody(),
        response));
    assert(response.statusCode == 202);
    assert(response.headers.at("Cache-Control") == "no-store");
    assert(response.body.find("\"accepted\":true") != std::string::npos);
    assert(response.body.find("\"state\":\"queued\"") != std::string::npos);
    assert(response.body.find("\"verification\":\"readback_required\"") != std::string::npos);
    assert(response.body.find("cmd-marks-1") != std::string::npos);
    assert(response.body.find("fp-marks-1") != std::string::npos);
    assert(response.body.find("/srv/vdr") == std::string::npos);
    assert(response.body.find("recordingKey") == std::string::npos);
    assert(dispatchCalls == 1);
    assert(!lastMutation.replayOnly);
    assert(lastMutation.kind == RecordingMarksMutationKind::Add);
    assert(lastMutation.backendId == "default");
    assert(lastMutation.operationId == "marks-op-1");
    assert(lastMutation.operationRevision == "rev-1");
    assert(lastMutation.expectedMarksRevision ==
        "0123456789abcdef0123456789abcdef");
    assert(lastMutation.sourceFrame == -1);
    assert(lastMutation.targetFrame == 500);
    assert(lastMutation.recordingKey ==
        VdrRecordingNativeIdentity::keyForNativeId(
            recordings.front().backendNativeId));
    replayExists = true;

    dispatchResult.replayed = true;
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks",
        addMutationBody(),
        response));
    assert(response.statusCode == 202);
    assert(response.body.find("\"replayed\":true") != std::string::npos);
    assert(dispatchCalls == 2);
    assert(replayProbeCalls == 2);
    assert(!lastMutation.replayOnly);

    resolver.next.marksRevision =
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks",
        addMutationBody(),
        response));
    assert(response.statusCode == 202);
    assert(response.body.find("\"replayed\":true") != std::string::npos);
    assert(dispatchCalls == 2);
    assert(replayProbeCalls == 3);
    assert(lastMutation.replayOnly);

    resolver.next = availableMarks();
    resolver.next.inUseFlags = 0;
    dispatchResult.accepted = false;
    dispatchResult.replayed = false;
    dispatchResult.reasonCode = "recording_marks_revision_conflict";
    dispatchResult.commandId.clear();
    dispatchResult.requestFingerprint.clear();
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks",
        addMutationBody(),
        response));
    assert(response.statusCode == 409);
    assert(dispatchCalls == 3);

    dispatchResult.accepted = true;
    dispatchResult.reasonCode.clear();
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks",
        addMutationBody(),
        response));
    assert(response.statusCode == 502);
    assert(response.body.find("recording_marks_mutation_dispatch_invalid") !=
        std::string::npos);
    assert(dispatchCalls == 4);

    assert(writePolicyCalls >= 8);

    runtime.reset();
    assert(!runtime.configured());
    assert(!runtime.mutationConfigured());
    return 0;
}
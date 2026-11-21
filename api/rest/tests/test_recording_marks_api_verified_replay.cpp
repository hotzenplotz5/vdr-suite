#include "RecordingMarksApiRuntime.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
class FakeResolver final : public IVdrRecordingNativeMarksResolver
{
public:
    VdrRecordingNativeMarks resolve(const std::string& recordingKey) override
    {
        ++calls;
        VdrRecordingNativeMarks result;
        result.availability = VdrRecordingNativeMarksAvailability::Available;
        result.schema = 1;
        result.found = true;
        result.reason = "none";
        result.recordingIdentitySchema = 1;
        result.recordingKey = recordingKey;
        result.state = "present";
        result.framesPerSecond = 25.0;
        result.isPesRecording = false;
        result.inUseFlags = 0;
        result.marksFilePresent = true;
        result.sequenceCount = 1;
        result.marksRevision = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
        return result;
    }

    int calls = 0;
};

VdrRecording recording()
{
    VdrRecording value;
    value.id = "7";
    value.backendId = "default";
    value.backendNativeId =
        "/srv/vdr/video/Movies/Test/2026-09-05.18.00.1-0.rec";
    value.title = "Test";
    return value;
}
}

int main()
{
    RecordingMarksApiRuntime& runtime = RecordingMarksApiRuntime::instance();
    runtime.reset();

    FakeResolver resolver;
    std::vector<VdrRecording> recordings{recording()};
    int dispatchCalls = 0;

    assert(runtime.configure(
        [&](const std::string& backendId) {
            assert(backendId == "default");
            return recordings;
        },
        [&](const std::string& backendId) {
            assert(backendId == "default");
            return RecordingMarksBackendAccess{
                RecordingMarksBackendAvailability::Available,
                &resolver};
        },
        [&](const std::string& backendId) {
            assert(backendId == "default");
            RecordingMarksBackendWriteAccess access;
            access.allowed = true;
            access.statusCode = 200;
            access.reasonCode = "recording_marks_backend_write_allowed";
            return access;
        },
        [&](const RecordingMarksMutationRequest& request) {
            ++dispatchCalls;
            assert(request.replayOnly);
            assert(request.operationId == "marks-op-lost-response");
            assert(request.operationRevision == "rev-1");
            assert(request.expectedMarksRevision ==
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
            RecordingMarksMutationDispatchResult result;
            result.accepted = true;
            result.replayed = true;
            result.verified = true;
            result.reasonCode = "recording_marks_modify_verified_replayed";
            result.commandId = "cmd-marks-lost-response";
            result.requestFingerprint = "fp-marks-lost-response";
            result.canonicalMarksRevision =
                "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
            return result;
        }));

    ApiResponse response;
    const std::string body =
        "{\"backendId\":\"default\","
        "\"recordingId\":\"7\","
        "\"operationId\":\"marks-op-lost-response\","
        "\"operationRevision\":\"rev-1\","
        "\"expectedMarksRevision\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\","
        "\"kind\":\"add\","
        "\"targetFrame\":500}";

    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks", body, response));
    assert(response.statusCode == 200);
    assert(response.headers.at("Cache-Control") == "no-store");
    assert(response.body.find("\"accepted\":true") != std::string::npos);
    assert(response.body.find("\"replayed\":true") != std::string::npos);
    assert(response.body.find("\"state\":\"verified\"") != std::string::npos);
    assert(response.body.find("\"verification\":\"verified\"") !=
        std::string::npos);
    assert(response.body.find(
        "\"canonicalMarksRevision\":\"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\"") !=
        std::string::npos);
    assert(dispatchCalls == 1);
    assert(resolver.calls == 1);

    runtime.reset();
    return 0;
}

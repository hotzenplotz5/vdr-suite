#include "RecordingMarksApiRuntime.h"

#include <cassert>
#include <string>

namespace
{
std::string mutationBody(
    const std::string& operationId,
    const std::string& operationRevision)
{
    return
        "{\"backendId\":\"default\","
        "\"recordingId\":\"7\","
        "\"operationId\":\"" + operationId + "\","
        "\"operationRevision\":\"" + operationRevision + "\","
        "\"expectedMarksRevision\":\"0123456789abcdef0123456789abcdef\","
        "\"kind\":\"add\","
        "\"targetFrame\":500}";
}

void expectInvalid(
    RecordingMarksApiRuntime& runtime,
    const std::string& body)
{
    ApiResponse response;
    assert(runtime.tryHandlePost(
        "/api/vdr/recordings/marks",
        body,
        response));
    assert(response.statusCode == 400);
    assert(response.body.find("recording_marks_mutation_request_invalid") !=
        std::string::npos);
}
}

int main()
{
    RecordingMarksApiRuntime& runtime = RecordingMarksApiRuntime::instance();
    runtime.reset();

    // Operation tokens are durable replay keys. Reject whitespace, slash,
    // control escapes, empties and oversize values before runtime lookup or
    // any mutation dispatcher can be reached.
    expectInvalid(runtime, mutationBody("marks op 1", "rev-1"));
    expectInvalid(runtime, mutationBody("marks/op/1", "rev-1"));
    expectInvalid(runtime, mutationBody("marks-op-1", "rev/1"));
    expectInvalid(runtime, mutationBody("", "rev-1"));
    expectInvalid(runtime, mutationBody("marks-op-1", ""));
    expectInvalid(runtime, mutationBody(std::string(193, 'a'), "rev-1"));
    expectInvalid(runtime, mutationBody("marks-op-1", std::string(193, 'b')));

    assert(!runtime.configured());
    assert(!runtime.mutationConfigured());
    return 0;
}

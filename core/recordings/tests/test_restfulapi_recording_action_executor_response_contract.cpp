#include "RestfulApiRecordingActionBackendExecutorAdapter.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
class SequenceHttpClient final : public IHttpClient
{
public:
    explicit SequenceHttpClient(std::vector<HttpResponse> responses)
        : responses_(std::move(responses))
    {
    }

    HttpResponse execute(const HttpRequest& request) const override
    {
        requests_.push_back(request);
        assert(nextResponse_ < responses_.size());
        return responses_.at(nextResponse_++);
    }

    std::size_t requestCount() const
    {
        return requests_.size();
    }

    const HttpRequest& requestAt(std::size_t index) const
    {
        return requests_.at(index);
    }

private:
    std::vector<HttpResponse> responses_;
    mutable std::vector<HttpRequest> requests_;
    mutable std::size_t nextResponse_ = 0;
};

HttpResponse response(int statusCode, const std::string& body)
{
    HttpResponse result;
    result.statusCode = statusCode;
    result.body = body;
    return result;
}

RecordingActionJobPayload makeDeletePayload(bool dryRun)
{
    RecordingActionJobPayload payload;
    payload.backendId = "default";
    payload.recordingId = "Movies/Tatort/2026-06-16.20.15.1-0.rec";
    payload.type = RecordingActionType::Delete;
    payload.dryRun = dryRun;
    payload.parameters["recordingPath"] = payload.recordingId;
    payload.parameters["backendNativeId"] =
        "/srv/vdr/video/Movies/Tatort/2026-06-16.20.15.1-0.rec";
    return payload;
}

RestfulApiRecordingActionBackendConfig makeConfig(
    bool readOnly,
    bool allowExecution)
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = "default";
    config.basePath = "/api";
    config.readOnly = readOnly;
    config.allowExecution = allowExecution;
    return config;
}

const std::string readyPreview =
    "{"
    "\"executable\":true,"
    "\"recording_file\":\"/srv/vdr/video/test.rec\","
    "\"blockers\":[],"
    "\"warnings\":[],"
    "\"revision_recordings_state\":12345,"
    "\"revision_timers_state\":67890"
    "}";
}

int main()
{
    {
        SequenceHttpClient httpClient({
            response(200, readyPreview),
            response(200, "{\"status\":\"ready\",\"blockers\":[],\"warnings\":[]}"),
            response(
                200,
                "{\"status\":\"trashed\","
                "\"recording_file\":\"/srv/vdr/video/test.rec\","
                "\"deleted_recording_file\":\"/srv/vdr/video/test.del\","
                "\"message\":\"Recording moved to the VDR trash.\"}")
        });

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(result.success);
        assert(result.type == RecordingActionType::Delete);
        assert(result.backendId == "default");
        assert(result.message == "Recording moved to the VDR trash.");
        assert(result.errors.empty());
        assert(result.upstreamHttpStatus == 200);
        assert(result.upstreamEndpoint == "/api/recordings/trash.json");
        assert(httpClient.requestCount() == 3);
        assert(httpClient.requestAt(0).url ==
            "/api/recordings/trash/preview.json");
        assert(httpClient.requestAt(1).url ==
            "/api/recordings/trash/validate.json");
        assert(httpClient.requestAt(2).url ==
            "/api/recordings/trash.json");
        assert(httpClient.requestAt(1).body.find(
            "\"revision_recordings_state\":\"12345\"") !=
            std::string::npos);
        assert(httpClient.requestAt(2).body ==
            httpClient.requestAt(1).body);
    }

    {
        SequenceHttpClient httpClient({
            response(200, readyPreview),
            response(200, "{\"status\":\"ready\",\"blockers\":[],\"warnings\":[]}"),
            response(
                200,
                "{\"status\":\"already-trashed\","
                "\"message\":\"Recording is already present in the VDR trash.\"}")
        });

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(result.success);
        assert(result.message ==
            "Recording is already present in the VDR trash.");
        assert(httpClient.requestCount() == 3);
    }

    {
        SequenceHttpClient httpClient({
            response(
                200,
                "{"
                "\"executable\":false,"
                "\"recording_file\":\"/srv/vdr/video/test.rec\","
                "\"blockers\":[\"local-timer-active\"],"
                "\"warnings\":[],"
                "\"revision_recordings_state\":1,"
                "\"revision_timers_state\":2"
                "}")
        });

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(!result.success);
        assert(result.message ==
            "restfulapi recording trash preview blocked");
        assert(result.errors.size() == 1);
        assert(result.errors.at(0) ==
            "recording trash blocker: local-timer-active");
        assert(httpClient.requestCount() == 1);
    }

    {
        SequenceHttpClient httpClient({
            response(200, readyPreview),
            response(
                200,
                "{\"status\":\"conflict\","
                "\"blockers\":[\"recording-missing\"],"
                "\"warnings\":[]}")
        });

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(!result.success);
        assert(result.message ==
            "restfulapi recording trash validation blocked");
        assert(result.errors.at(0) ==
            "recording trash blocker: recording-missing");
        assert(httpClient.requestCount() == 2);
    }

    {
        SequenceHttpClient httpClient({response(409, "state changed")});

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(!result.success);
        assert(result.message ==
            "restfulapi recording trash preview request failed");
        assert(result.upstreamHttpStatus == 409);
        assert(result.errors.at(0) ==
            "restfulapi backend returned HTTP status 409");
        assert(result.errors.at(1) == "state changed");
    }

    {
        SequenceHttpClient httpClient({response(200, readyPreview)});

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, false),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(true));

        assert(result.success);
        assert(result.message ==
            "restfulapi recording trash preview ready");
        assert(httpClient.requestCount() == 1);
        assert(httpClient.requestAt(0).url ==
            "/api/recordings/trash/preview.json");
    }

    {
        SequenceHttpClient httpClient({});
        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(true, true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(!result.success);
        assert(result.message ==
            "restfulapi backend executor backend is read-only");
        assert(httpClient.requestCount() == 0);
    }

    {
        SequenceHttpClient httpClient({});
        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, false),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(!result.success);
        assert(result.message ==
            "restfulapi backend executor execution disabled");
        assert(httpClient.requestCount() == 0);
    }

    return 0;
}

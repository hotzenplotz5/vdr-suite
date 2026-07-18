#include "IHttpClient.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace
{

class SequenceHttpClient final : public IHttpClient
{
public:
    explicit SequenceHttpClient(
        std::vector<HttpResponse> responses)
        : responses_(std::move(responses))
    {
    }

    HttpResponse execute(
        const HttpRequest& request) const override
    {
        requests_.push_back(request);

        assert(
            nextResponse_ <
            responses_.size());

        return responses_.at(
            nextResponse_++);
    }

    const std::vector<HttpRequest>& requests() const
    {
        return requests_;
    }

private:
    std::vector<HttpResponse> responses_;
    mutable std::vector<HttpRequest> requests_;
    mutable std::size_t nextResponse_ = 0;
};

RestfulApiRecordingActionBackendConfig safeConfig(
    bool allowExecution)
{
    RestfulApiRecordingActionBackendConfig config;

    config.backendId = "default";
    config.basePath = "";
    config.videoDirectory = "/srv/vdr/video";
    config.apiMode =
        RestfulApiRecordingActionApiMode::SafeMutation;
    config.allowExecution = allowExecution;
    config.readOnly = false;

    return config;
}

RecordingActionJobPayload missionPayload(
    bool dryRun)
{
    RecordingActionJobPayload payload;

    payload.backendId = "default";
    payload.recordingId = "990";
    payload.type = RecordingActionType::Move;
    payload.dryRun = dryRun;

    payload.parameters["recordingPath"] =
        "/Mission#3A_Impossible/"
        "2026-05-24.20.13.8-0.rec";

    payload.parameters["backendNativeId"] =
        "/srv/vdr/video/"
        "Mission#3A_Impossible/"
        "2026-05-24.20.13.8-0.rec";

    payload.parameters["recordingTitle"] =
        "Mission: Impossible";

    payload.parameters["targetPath"] =
        "__VDR-SUITE-TEST-MOVE__";

    return payload;
}

HttpResponse response(
    int statusCode,
    const std::string& body)
{
    HttpResponse value;
    value.statusCode = statusCode;
    value.body = body;
    return value;
}

}

int main()
{
    {
        SequenceHttpClient httpClient({
            response(
                200,
                "{"
                "\"executable\":true,"
                "\"revision_recordings_state\":111,"
                "\"revision_timers_state\":222"
                "}"),
            response(
                200,
                "{\"status\":\"ready\"}"),
            response(
                200,
                "{"
                "\"status\":\"moved\","
                "\"recording_file\":"
                "\"/srv/vdr/video/"
                "Mission#3A_Impossible/"
                "2026-05-24.20.13.8-0.rec\","
                "\"target_file\":"
                "\"/srv/vdr/video/"
                "__VDR-SUITE-TEST-MOVE__/"
                "Mission#3A_Impossible/"
                "2026-05-24.20.13.8-0.rec\""
                "}")
        });

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            safeConfig(true),
            httpClient);

        assert(adapter.supportsAction(
            RecordingActionType::Move));

        assert(adapter.supportsAction(
            RecordingActionType::Rename));

        assert(adapter.supportsAction(
            RecordingActionType::Delete));

        const RecordingActionExecutionResult result =
            adapter.execute(
                missionPayload(false));

        assert(result.success);

        assert(result.message ==
               "restfulapi safe move request accepted");

        assert(result.upstreamHttpStatus == 200);

        assert(result.upstreamEndpoint ==
               "/recordings/move.json");

        assert(result.upstreamResponseBody.find(
            "\"status\":\"moved\"") !=
               std::string::npos);

        assert(httpClient.requests().size() == 3);

        const HttpRequest& preview =
            httpClient.requests().at(0);

        const HttpRequest& validate =
            httpClient.requests().at(1);

        const HttpRequest& execute =
            httpClient.requests().at(2);

        assert(preview.url ==
               "/recordings/move/preview.json");

        assert(validate.url ==
               "/recordings/move/validate.json");

        assert(execute.url ==
               "/recordings/move.json");

        const std::string expectedSource =
            "\"file\":"
            "\"/srv/vdr/video/"
            "Mission#3A_Impossible/"
            "2026-05-24.20.13.8-0.rec\"";

        const std::string expectedTarget =
            "\"target_file\":"
            "\"/srv/vdr/video/"
            "__VDR-SUITE-TEST-MOVE__/"
            "Mission#3A_Impossible/"
            "2026-05-24.20.13.8-0.rec\"";

        assert(preview.body.find(expectedSource) !=
               std::string::npos);

        assert(preview.body.find(expectedTarget) !=
               std::string::npos);

        assert(preview.body.find(
            "\"source\"") ==
               std::string::npos);

        assert(preview.body.find(
            "\"target\"") ==
               std::string::npos);

        assert(preview.body.find(
            "\"copy_only\"") ==
               std::string::npos);

        assert(validate.body.find(
            expectedSource) !=
               std::string::npos);

        assert(validate.body.find(
            expectedTarget) !=
               std::string::npos);

        assert(validate.body.find(
            "\"revision_recordings_state\":"
            "\"111\"") !=
               std::string::npos);

        assert(validate.body.find(
            "\"revision_timers_state\":"
            "\"222\"") !=
               std::string::npos);

        assert(execute.body == validate.body);
    }

    {
        SequenceHttpClient httpClient({
            response(
                200,
                "{"
                "\"executable\":true,"
                "\"revision_recordings_state\":333,"
                "\"revision_timers_state\":444"
                "}")
        });

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            safeConfig(false),
            httpClient);

        RecordingActionJobPayload payload =
            missionPayload(true);

        payload.parameters["targetPath"] =
            "Archive: New Folder";

        const RecordingActionExecutionResult result =
            adapter.execute(payload);

        assert(result.success);

        assert(result.message ==
               "restfulapi safe move preview accepted");

        assert(result.upstreamEndpoint ==
               "/recordings/move/preview.json");

        assert(httpClient.requests().size() == 1);

        assert(httpClient.requests().at(0).body.find(
            "\"target_file\":"
            "\"/srv/vdr/video/"
            "Archive#3A_New_Folder/"
            "Mission#3A_Impossible/"
            "2026-05-24.20.13.8-0.rec\"") !=
               std::string::npos);
    }

    {
        SequenceHttpClient httpClient({
            response(
                200,
                "{"
                "\"executable\":false,"
                "\"blockers\":["
                "\"replay-active\"],"
                "\"revision_recordings_state\":555,"
                "\"revision_timers_state\":666"
                "}")
        });

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            safeConfig(true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(
                missionPayload(false));

        assert(!result.success);

        assert(result.message ==
               "restfulapi safe move preview blocked");

        assert(result.errors.size() == 2);

        assert(result.errors.at(0) ==
               "safe move preview is not executable");

        assert(result.errors.at(1).find(
            "replay-active") !=
               std::string::npos);

        assert(result.upstreamHttpStatus == 200);

        assert(result.upstreamEndpoint ==
               "/recordings/move/preview.json");

        assert(httpClient.requests().size() == 1);
    }

    {
        SequenceHttpClient httpClient({
            response(
                400,
                "")
        });

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            safeConfig(true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(
                missionPayload(false));

        assert(!result.success);

        assert(result.message ==
               "restfulapi safe move preview failed");

        assert(result.errors.size() == 1);

        assert(result.errors.at(0) ==
               "restfulapi safe move preview "
               "returned HTTP status 400");

        assert(result.upstreamHttpStatus == 400);

        assert(result.upstreamEndpoint ==
               "/recordings/move/preview.json");

        assert(httpClient.requests().size() == 1);
    }

    return 0;
}

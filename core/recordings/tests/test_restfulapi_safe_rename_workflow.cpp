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


RecordingActionJobPayload renamePayload(
    bool dryRun,
    const std::string& newName =
        "VDR-SUITE-TEST-RENAME")
{
    RecordingActionJobPayload payload;

    payload.backendId = "default";
    payload.recordingId = "990";
    payload.type = RecordingActionType::Rename;
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

    payload.parameters["newName"] =
        newName;

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
                "\"recording_file\":"
                "\"/srv/vdr/video/"
                "Mission#3A_Impossible/"
                "2026-05-24.20.13.8-0.rec\","
                "\"name\":"
                "\"VDR-SUITE-TEST-RENAME\","
                "\"target_file\":"
                "\"/srv/vdr/video/"
                "VDR-SUITE-TEST-RENAME/"
                "2026-05-24.20.13.8-0.rec\","
                "\"rename_status\":\"ready\","
                "\"revision_recordings_state\":111,"
                "\"revision_timers_state\":222"
                "}"),
            response(
                200,
                "{"
                "\"status\":\"ready\","
                "\"rename_status\":\"ready\""
                "}"),
            response(
                200,
                "{"
                "\"status\":\"renamed\","
                "\"name\":"
                "\"VDR-SUITE-TEST-RENAME\","
                "\"target_file\":"
                "\"/srv/vdr/video/"
                "VDR-SUITE-TEST-RENAME/"
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
                renamePayload(false));

        assert(result.success);

        assert(result.message ==
               "restfulapi safe rename request accepted");

        assert(result.upstreamHttpStatus == 200);

        assert(result.upstreamEndpoint ==
               "/recordings/rename.json");

        assert(result.upstreamResponseBody.find(
            "\"status\":\"renamed\"") !=
               std::string::npos);

        assert(httpClient.requests().size() == 3);

        const HttpRequest& preview =
            httpClient.requests().at(0);

        const HttpRequest& validate =
            httpClient.requests().at(1);

        const HttpRequest& execute =
            httpClient.requests().at(2);

        assert(preview.url ==
               "/recordings/rename/preview.json");

        assert(validate.url ==
               "/recordings/rename/validate.json");

        assert(execute.url ==
               "/recordings/rename.json");

        const std::string expectedSource =
            "\"file\":"
            "\"/srv/vdr/video/"
            "Mission#3A_Impossible/"
            "2026-05-24.20.13.8-0.rec\"";

        const std::string expectedName =
            "\"name\":"
            "\"VDR-SUITE-TEST-RENAME\"";

        assert(preview.body.find(expectedSource) !=
               std::string::npos);

        assert(preview.body.find(expectedName) !=
               std::string::npos);

        assert(preview.body.find(
            "\"target_file\"") ==
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
            expectedName) !=
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
                "\"target_file\":"
                "\"/srv/vdr/video/"
                "Mission#3A_Impossible/"
                "2026-05-24.20.13.8-0.rec\","
                "\"revision_recordings_state\":333,"
                "\"revision_timers_state\":444"
                "}")
        });

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            safeConfig(false),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(
                renamePayload(
                    true,
                    "Mission: Impossible"));

        assert(result.success);

        assert(result.message ==
               "restfulapi safe rename preview accepted");

        assert(result.upstreamEndpoint ==
               "/recordings/rename/preview.json");

        assert(httpClient.requests().size() == 1);

        assert(httpClient.requests().at(0).body.find(
            "\"name\":"
            "\"Mission#3A_Impossible\"") !=
               std::string::npos);
    }

    {
        SequenceHttpClient httpClient({
            response(
                200,
                "{"
                "\"executable\":false,"
                "\"rename_status\":"
                "\"target-same-as-source\","
                "\"blockers\":["
                "\"rename-target-same-as-source\"],"
                "\"revision_recordings_state\":555,"
                "\"revision_timers_state\":666"
                "}")
        });

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            safeConfig(true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(
                renamePayload(false));

        assert(!result.success);

        assert(result.message ==
               "restfulapi safe rename preview blocked");

        assert(result.errors.size() == 2);

        assert(result.errors.at(0) ==
               "safe rename preview is not executable");

        assert(result.errors.at(1).find(
            "rename-target-same-as-source") !=
               std::string::npos);

        assert(result.upstreamEndpoint ==
               "/recordings/rename/preview.json");

        assert(httpClient.requests().size() == 1);
    }

    {
        SequenceHttpClient httpClient({
            response(
                409,
                "Recording state changed after preview.")
        });

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            safeConfig(true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(
                renamePayload(false));

        assert(!result.success);

        assert(result.message ==
               "restfulapi safe rename preview failed");

        assert(result.upstreamHttpStatus == 409);

        assert(result.upstreamEndpoint ==
               "/recordings/rename/preview.json");

        assert(result.errors.size() == 2);

        assert(result.errors.at(0) ==
               "restfulapi safe rename preview "
               "returned HTTP status 409");

        assert(result.errors.at(1) ==
               "Recording state changed after preview.");

        assert(httpClient.requests().size() == 1);
    }

    return 0;
}

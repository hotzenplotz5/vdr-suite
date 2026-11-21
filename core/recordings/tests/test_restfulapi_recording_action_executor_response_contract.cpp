#include "MockHttpClient.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"

#include <cassert>
#include <string>

namespace
{
RecordingActionJobPayload makeDeletePayload(
    bool dryRun)
{
    RecordingActionJobPayload payload;
    payload.backendId = "default";
    payload.recordingId = "Movies/Tatort/2026-06-16.20.15.1-0.rec";
    payload.type = RecordingActionType::Delete;
    payload.dryRun = dryRun;
    payload.parameters["recordingPath"] =
        "Movies/Tatort/2026-06-16.20.15.1-0.rec";
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
}

int main()
{
    {
        MockHttpClient httpClient;
        HttpResponse response;
        response.statusCode = 200;
        response.body = "Recording deleted!";
        httpClient.setResponse(response);

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(result.success);
        assert(result.type == RecordingActionType::Delete);
        assert(result.backendId == "default");
        assert(result.recordingId == "Movies/Tatort/2026-06-16.20.15.1-0.rec");
        assert(result.message == "restfulapi backend executor request accepted");
        assert(result.errors.empty());
        assert(httpClient.requestCount() == 1);
        assert(httpClient.lastRequest().method == "POST");
        assert(httpClient.lastRequest().url == "/api/recordings/delete.json");
    }

    {
        MockHttpClient httpClient;
        HttpResponse response;
        response.statusCode = 201;
        response.body = "{\"recordings\":[]}";
        httpClient.setResponse(response);

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(result.success);
        assert(result.message == "restfulapi backend executor request accepted");
        assert(result.errors.empty());
        assert(httpClient.requestCount() == 1);
    }

    {
        MockHttpClient httpClient;
        HttpResponse response;
        response.statusCode = 204;
        response.body = "";
        httpClient.setResponse(response);

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(result.success);
        assert(result.message == "restfulapi backend executor request accepted");
        assert(result.errors.empty());
        assert(httpClient.requestCount() == 1);
    }

    {
        MockHttpClient httpClient;
        HttpResponse response;
        response.statusCode = 404;
        response.body = "Recording not found!";
        httpClient.setResponse(response);

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(!result.success);
        assert(result.message == "restfulapi backend executor request failed");
        assert(result.errors.size() == 2);
        assert(result.errors.at(0) ==
               "restfulapi backend returned HTTP status 404");
        assert(result.errors.at(1) == "Recording not found!");
        assert(httpClient.requestCount() == 1);
    }

    {
        MockHttpClient httpClient;
        HttpResponse response;
        response.statusCode = 500;
        response.body = "";
        httpClient.setResponse(response);

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(!result.success);
        assert(result.message == "restfulapi backend executor request failed");
        assert(result.errors.size() == 1);
        assert(result.errors.at(0) ==
               "restfulapi backend returned HTTP status 500");
        assert(httpClient.requestCount() == 1);
    }

    {
        MockHttpClient httpClient;
        HttpResponse response;
        response.statusCode = 200;
        httpClient.setResponse(response);

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(true, true),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(!result.success);
        assert(result.message ==
               "restfulapi backend executor backend is read-only");
        assert(result.errors.size() == 1);
        assert(result.errors.at(0) ==
               "recording action execution is blocked by read-only backend config");
        assert(httpClient.requestCount() == 0);
    }

    {
        MockHttpClient httpClient;
        HttpResponse response;
        response.statusCode = 200;
        httpClient.setResponse(response);

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, false),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(false));

        assert(!result.success);
        assert(result.message ==
               "restfulapi backend executor execution disabled");
        assert(result.errors.size() == 1);
        assert(result.errors.at(0) ==
               "real recording action execution is disabled by restfulapi backend config");
        assert(httpClient.requestCount() == 0);
    }

    {
        MockHttpClient httpClient;
        HttpResponse response;
        response.statusCode = 200;
        response.body = "Dry run accepted by test transport";
        httpClient.setResponse(response);

        RestfulApiRecordingActionBackendExecutorAdapter adapter(
            makeConfig(false, false),
            httpClient);

        const RecordingActionExecutionResult result =
            adapter.execute(makeDeletePayload(true));

        assert(result.success);
        assert(result.message == "restfulapi backend executor request accepted");
        assert(httpClient.requestCount() == 1);
    }

    return 0;
}

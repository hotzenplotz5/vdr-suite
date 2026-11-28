#include "Database.h"
#include "RecordingSeriesHierarchyApiRuntime.h"

#include <cassert>
#include <filesystem>
#include <string>

int main()
{
    const std::filesystem::path databasePath =
        std::filesystem::temp_directory_path() /
        "vdr-suite-series-hierarchy-api-test.db";

    std::error_code error;
    std::filesystem::remove(
        databasePath,
        error);

    Database database;

    assert(
        database.open(
            databasePath.string()));

    auto& runtime =
        RecordingSeriesHierarchyApiRuntime::
            instance();

    runtime.reset();

    assert(runtime.configure(database));
    assert(runtime.configured());

    ApiResponse response;
    const std::string publicId = "rec_0123456789abcdef0123456789abcdef";
    runtime.registerPublicRecordingKeyLookup([&](const std::string& backend, const std::string& id) {
        return backend == "default" && id == publicId ? std::string("Series/Opaque") : std::string{};
    });
    const std::string mutation = "{\"operation\":\"set\",\"recordingId\":\"" + publicId +
        "\",\"groupType\":\"season\",\"seasonNumber\":3}";
    assert(runtime.tryHandlePost("/api/backends/default/recordings/series-hierarchy", mutation, "test-actor", response));
    assert(response.statusCode == 200);
    assert(response.body.find(publicId) != std::string::npos);
    assert(response.body.find("Series/Opaque") == std::string::npos);
    assert(runtime.tryHandleGet("/api/backends/default/recordings/series-hierarchy?resourceKey=Series%2FOpaque", response));
    assert(response.statusCode == 200 && response.body.find("\"seasonNumber\":3") != std::string::npos);
    assert(runtime.tryHandlePost("/api/backends/other/recordings/series-hierarchy", mutation, "test-actor", response));
    assert(response.statusCode == 404);
    const std::string publicRoute = "/api/v1/backends/default/recordings/series-hierarchy";
    const std::string update = "{\"operation\":\"set\",\"recordingId\":\"" + publicId +
        "\",\"groupType\":\"season\",\"seasonNumber\":4,\"expectedRevision\":1}";
    assert(runtime.tryHandlePost(publicRoute, update, "test-actor", response, "other", "application/json"));
    assert(response.statusCode == 403);
    assert(runtime.tryHandlePost(publicRoute, update, "test-actor", response, "default", "text/plain"));
    assert(response.statusCode == 415);
    assert(runtime.tryHandlePost(publicRoute, "{\"operation\":\"clear\",\"resourceKey\":\"Series/Opaque\"}",
        "test-actor", response, "default", "application/json"));
    assert(response.statusCode == 400);
    assert(runtime.tryHandlePost(publicRoute, update, "test-actor", response, "default", "application/json"));
    assert(response.statusCode == 200);
    assert(response.body.find("\"recordingId\":\"" + publicId + "\"") != std::string::npos);
    assert(response.body.find("recordingKey") == std::string::npos);
    assert(response.body.find("Series/Opaque") == std::string::npos);
    assert(runtime.tryHandleGet("/api/backends/default/recordings/series-hierarchy?resourceKey=Series%2FOpaque", response));
    assert(response.statusCode == 200 && response.body.find("\"seasonNumber\":4") != std::string::npos);
    assert(runtime.tryHandlePost(publicRoute, update, "test-actor", response, "default", "application/json"));
    assert(response.statusCode == 409);
    const std::string clear = "{\"operation\":\"clear\",\"recordingId\":\"" + publicId + "\",\"expectedRevision\":2}";
    assert(runtime.tryHandlePost(publicRoute, clear, "test-actor", response, "default", "application/json"));
    assert(response.statusCode == 200 && response.body.find("\"available\":false") != std::string::npos);
    assert(runtime.tryHandleGet("/api/backends/default/recordings/series-hierarchy?resourceKey=Series%2FOpaque", response));
    assert(response.statusCode == 200 && response.body == "{\"available\":false}");
    response = {};

    assert(runtime.tryHandleGet(
        "/api/backends/default/recordings/"
        "series-hierarchy"
        "?resourceKey="
        "Battlestar_Galactica%2FPilot",
        response));

    assert(response.statusCode == 200);
    assert(
        response.body ==
        "{\"available\":false}");

    response = {};

    assert(runtime.tryHandlePost(
        "/api/backends/default/recordings/"
        "series-hierarchy",
        "{\"operation\":\"set\","
        "\"resourceKey\":"
        "\"Battlestar_Galactica/Pilot\","
        "\"groupType\":\"special\","
        "\"groupLabel\":"
        "\"Pilot / Miniserie\","
        "\"sortOrder\":-100,"
        "\"episodeStart\":1,"
        "\"episodeEnd\":2}",
        "",
        response));

    assert(response.statusCode == 401);

    response = {};

    assert(runtime.tryHandlePost(
        "/api/backends/default/recordings/"
        "series-hierarchy",
        "{\"operation\":\"set\","
        "\"resourceKey\":"
        "\"Battlestar_Galactica/Pilot\","
        "\"groupType\":\"special\","
        "\"groupLabel\":"
        "\"Pilot / Miniserie\","
        "\"sortOrder\":-100,"
        "\"episodeStart\":1,"
        "\"episodeEnd\":2}",
        "test-actor",
        response));

    assert(response.statusCode == 200);

    assert(
        response.body.find(
            "\"available\":true") !=
        std::string::npos);

    assert(
        response.body.find(
            "\"groupType\":\"special\"") !=
        std::string::npos);

    assert(
        response.body.find(
            "\"groupLabel\":"
            "\"Pilot / Miniserie\"") !=
        std::string::npos);

    assert(
        response.body.find(
            "\"episodeStart\":1") !=
        std::string::npos);

    assert(
        response.body.find(
            "\"episodeEnd\":2") !=
        std::string::npos);

    assert(
        response.body.find(
            "\"revision\":1") !=
        std::string::npos);

    response = {};

    assert(runtime.tryHandleGet(
        "/api/backends/default/recordings/"
        "series-hierarchy"
        "?resourceKey="
        "Battlestar_Galactica%2FPilot",
        response));

    assert(response.statusCode == 200);

    assert(
        response.body.find(
            "\"groupLabel\":"
            "\"Pilot / Miniserie\"") !=
        std::string::npos);

    response = {};

    assert(runtime.tryHandleGet(
        "/api/backends/house-b/recordings/"
        "series-hierarchy"
        "?resourceKey="
        "Battlestar_Galactica%2FPilot",
        response));

    assert(response.statusCode == 200);

    assert(
        response.body ==
        "{\"available\":false}");

    response = {};

    assert(runtime.tryHandlePost(
        "/api/backends/default/recordings/"
        "series-hierarchy",
        "{\"operation\":\"set\","
        "\"resourceKey\":"
        "\"Battlestar_Galactica/Razor\","
        "\"groupType\":\"special\","
        "\"groupLabel\":"
        "\"TV-Filme / Specials\","
        "\"sortOrder\":1000}",
        "test-actor",
        response));

    assert(response.statusCode == 200);

    assert(
        response.body.find(
            "\"groupLabel\":"
            "\"TV-Filme / Specials\"") !=
        std::string::npos);

    response = {};

    assert(runtime.tryHandlePost(
        "/api/backends/default/recordings/"
        "series-hierarchy",
        "{\"operation\":\"set\","
        "\"resourceKey\":"
        "\"Battlestar_Galactica/Regular\","
        "\"groupType\":\"season\","
        "\"seasonNumber\":2}",
        "test-actor",
        response));

    assert(response.statusCode == 200);

    assert(
        response.body.find(
            "\"seasonNumber\":2") !=
        std::string::npos);

    response = {};

    assert(runtime.tryHandlePost(
        "/api/backends/default/recordings/"
        "series-hierarchy",
        "{\"operation\":\"clear\","
        "\"resourceKey\":"
        "\"Battlestar_Galactica/Pilot\","
        "\"expectedRevision\":1}",
        "test-actor",
        response));

    assert(response.statusCode == 200);

    assert(
        response.body ==
        "{\"available\":false}");

    runtime.reset();

    std::filesystem::remove(
        databasePath,
        error);

    return 0;
}

#include "Database.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingFolderController.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

static VdrRecording makeRecording(
    const std::string& id,
    const std::string& title,
    const std::string& path)
{
    VdrRecording recording;

    recording.id = id;
    recording.backendNativeId = "/srv/vdr/video" + path;
    recording.title = title;
    recording.path = path;
    recording.startTime = "1782928800";
    recording.durationSeconds = 3600;
    recording.sizeMb = 1024;

    return recording;
}

static bool contains(
    const std::string& text,
    const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

int main()
{
    std::remove("/tmp/test_vdr_recording_folder_controller.db");

    Database database;
    assert(database.open("/tmp/test_vdr_recording_folder_controller.db"));

    VdrRecordingCacheRepository repository(database);

    assert(repository.replaceRecordingsForBackend(
        "default",
        {
            makeRecording(
                "series-1",
                "Series Episode",
                "/Series/Show/2026-07-01.20.15.1-0.rec"),
            makeRecording(
                "movie-1",
                "Movie",
                "/Movies/Movie/2026-07-02.20.15.1-0.rec"),
            makeRecording(
                "root-1",
                "Root Recording",
                "/2026-07-03.20.15.1-0.rec")
        }));

    assert(repository.markRefreshFinished("default", 3));

    VdrRecordingFolderController controller(repository);

    const ApiResponse status =
        controller.getStatus("default");

    assert(status.statusCode == 200);
    assert(contains(status.body, "\"state\":\"ready\""));
    assert(contains(status.body, "\"totalCount\":3"));

    const ApiResponse root =
        controller.getFolder("default", "", 20, 0);

    assert(root.statusCode == 200);
    assert(contains(root.body, "\"recordingFolder\":true"));
    assert(contains(root.body, "\"Series\""));
    assert(contains(root.body, "\"Movies\""));
    assert(contains(root.body, "\"Root Recording\""));
    assert(contains(root.body, "\"metadata\":{"));
    assert(contains(root.body, "\"providerAvailable\":false"));
    assert(contains(root.body, "\"artworkPrepared\":false"));
    assert(contains(root.body, "\"placeholderVariant\":"));

    const ApiResponse series =
        controller.getFolder("default", "Series", 20, 0);

    assert(series.statusCode == 200);
    assert(contains(series.body, "\"Show\""));
    assert(contains(series.body, "\"parentPath\":\"\""));

    const ApiResponse show =
        controller.getFolder("default", "Series/Show", 20, 0);

    assert(show.statusCode == 200);
    assert(contains(show.body, "\"Series Episode\""));
    assert(contains(show.body, "\"parentPath\":\"Series\""));
    assert(contains(show.body, "\"metadata\":{"));

    std::cout
        << "test_vdr_recording_folder_controller passed"
        << std::endl;

    return 0;
}

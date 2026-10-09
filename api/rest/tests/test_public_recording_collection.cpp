#include "PublicApiRuntime.h"

#include <cassert>
#include <string>
#include <vector>

namespace
{
std::string nextCursor(const std::string& body)
{
    const std::string marker = "\"nextCursor\":\"";
    const auto start = body.find(marker);
    if (start == std::string::npos) return {};
    const auto value = start + marker.size();
    const auto end = body.find('"', value);
    return end == std::string::npos ? std::string() :
        body.substr(value, end - value);
}

PublicRecordingCollectionItem recording(const char* id)
{
    PublicRecordingCollectionItem value;
    value.recordingId = id;
    value.backendId = "backend-a";
    value.title = "Film";
    value.recordedAt = "2026-10-09T20:15:00Z";
    value.durationSeconds = 3600;
    value.durationKnown = true;
    return value;
}
}

int main()
{
    auto& runtime = PublicApiRuntime::instance();
    runtime.resetRecordingCollectionLookup();
    runtime.registerRecordingCollectionLookup(
        [](const PublicRecordingCollectionRequest& request)
        {
            PublicRecordingCollectionResult page;
            if (request.backendId == "backend-down")
                return page;
            page.status = PublicRecordingCollectionStatus::ok;
            for (const auto& item : {
                recording("rec_00000000000000000000000000000001"), recording("rec_00000000000000000000000000000002"), recording("rec_00000000000000000000000000000003")})
            {
                if (!request.afterRecordingId.empty() &&
                    item.recordingId <= request.afterRecordingId)
                    continue;
                if (page.recordings.size() >= request.limit)
                {
                    page.hasMore = true;
                    break;
                }
                page.recordings.push_back(item);
            }
            return page;
        });

    ApiResponse first;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&limit=2",
        "actor:tv", "recording-first", "", first, "", "",
        {"backend-a"}));
    assert(first.statusCode == 200);
    assert(first.body.find("\"recordingId\":\"rec_00000000000000000000000000000001\"") != std::string::npos);
    assert(first.body.find("\"durationSeconds\":3600") != std::string::npos);
    assert(first.body.find("\"durationKnown\":true") != std::string::npos);
    assert(first.body.find("\"hasMore\":true") != std::string::npos);
    assert(first.body.find("backendNativeId") == std::string::npos);
    assert(first.body.find("recordingPath") == std::string::npos);
    assert(first.body.find("/private") == std::string::npos);
    assert(first.headers.at("Cache-Control") == "no-store");

    const auto cursor = nextCursor(first.body);
    assert(cursor.rfind("rc1_", 0U) == 0U);
    ApiResponse second;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&limit=2&cursor=" + cursor,
        "actor:tv", "recording-second", "", second, "", "",
        {"backend-a"}));
    assert(second.statusCode == 200);
    assert(second.body.find("\"recordingId\":\"rec_00000000000000000000000000000003\"") != std::string::npos);
    assert(second.body.find("\"hasMore\":false") != std::string::npos);

    // Folder traversal is an additive view of the same recordings.view route.
    runtime.resetRecordingCollectionLookup();
    runtime.registerRecordingCollectionLookup(
        [](const PublicRecordingCollectionRequest& request)
        {
            PublicRecordingCollectionResult page;
            if (!request.browseFolders || request.backendId != "backend-a")
                return page;
            page.status = PublicRecordingCollectionStatus::ok;
            page.totalEntries = 2U;
            if (request.offset == 0U)
            {
                PublicRecordingFolderItem folder;
                folder.folderId = "fld1_0123456789abcdef0123456789abcdef";
                folder.name = "Serien";
                folder.recordingCount = 1;
                page.folders.push_back(folder);
            }
            else if (request.offset == 1U)
                page.recordings.push_back(
                    recording("rec_00000000000000000000000000000001"));
            page.hasMore = request.offset == 0U;
            return page;
        });
    ApiResponse folders;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&view=folders&limit=1",
        "actor:tv", "browse-root", "", folders, "", "", {"backend-a"}));
    assert(folders.statusCode == 200);
    assert(folders.body.find("\"kind\":\"folder\"") != std::string::npos);
    assert(folders.body.find("Serien") != std::string::npos);
    assert(folders.body.find("folderId") != std::string::npos);
    assert(folders.body.find("backendNativeId") == std::string::npos);
    ApiResponse children;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&view=folders&limit=1&offset=1",
        "actor:tv", "browse-child", "", children, "", "", {"backend-a"}));
    assert(children.statusCode == 200);
    assert(children.body.find("\"kind\":\"recording\"") != std::string::npos);
    ApiResponse invalidFolder;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&view=folders&folderId=/srv/private",
        "actor:tv", "browse-invalid", "", invalidFolder, "", "", {"backend-a"}));
    assert(invalidFolder.statusCode == 400);
    ApiResponse unauthorizedBrowse;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a&view=folders",
        "", "browse-unauth", "", unauthorizedBrowse, "", "", {"backend-a"}));
    assert(unauthorizedBrowse.statusCode == 401);
    runtime.resetRecordingCollectionLookup();
    runtime.registerRecordingCollectionLookup(
        [](const PublicRecordingCollectionRequest& request)
        {
            PublicRecordingCollectionResult page;
            if (request.backendId == "backend-down")
                return page;
            page.status = PublicRecordingCollectionStatus::ok;
            for (const auto& item : {
                recording("rec_00000000000000000000000000000001"),
                recording("rec_00000000000000000000000000000002"),
                recording("rec_00000000000000000000000000000003")})
            {
                if (!request.afterRecordingId.empty() &&
                    item.recordingId <= request.afterRecordingId)
                    continue;
                if (page.recordings.size() >= request.limit)
                {
                    page.hasMore = true;
                    break;
                }
                page.recordings.push_back(item);
            }
            return page;
        });

    ApiResponse wrongScope;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-b&cursor=" + cursor,
        "actor:tv", "recording-scope", "", wrongScope, "", "",
        {"backend-b"}));
    assert(wrongScope.statusCode == 409);

    for (const std::string& invalid : {
             "limit=0", "limit=101", "sort=startTime",
             "order=desc", "offset=5", "backendId=backend-b"})
    {
        ApiResponse response;
        assert(runtime.tryHandleGet(
            "/api/v1/recordings?backendId=backend-a&" + invalid,
            "actor:tv", "recording-invalid", "", response, "", "",
            {"backend-a"}));
        assert(response.statusCode == 400);
    }

    ApiResponse absent;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-down",
        "actor:tv", "recording-down", "", absent, "", "",
        {"backend-down"}));
    assert(absent.statusCode == 503);

    ApiResponse unauthenticated;
    assert(runtime.tryHandleGet(
        "/api/v1/recordings?backendId=backend-a",
        "", "recording-auth", "", unauthenticated, "", "",
        {"backend-a"}));
    assert(unauthenticated.statusCode == 401);

    runtime.resetRecordingCollectionLookup();
    return 0;
}

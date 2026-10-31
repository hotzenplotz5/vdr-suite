#include "PublicApiRuntime.h"
#include <cassert>
#include <string>
#include <vector>
int main()
{
    auto& api = PublicApiRuntime::instance();
    api.resetMediaImportSourceLookup();
    const std::string url =
        "/api/v1/backends/backend-a/media-import-sources";
    const std::vector<std::string> scopes{"backend-a"};
    auto get = [&](const std::string& actor,
                   const std::vector<std::string>& granted)
    {
        ApiResponse response;
        assert(api.tryHandleGet(url, actor, "list", "", response,
                                "", "", granted));
        return response;
    };
    assert(get("actor:one", scopes).statusCode == 503);
    bool staleCatalog = false;
    api.registerMediaImportSourceLookup(
        [&staleCatalog](const std::string& actor, const std::string& backend)
        {
            PublicApiRuntime::MediaImportSourceResult r;
            if (actor != "actor:one" || backend != "backend-a")
            {
                r.status = PublicApiRuntime::MediaImportSourceStatus::forbidden;
                return r;
            }
            r.status = PublicApiRuntime::MediaImportSourceStatus::ok;
            r.stale = staleCatalog;
            r.ids = {"source_00000000001", "source_00000000002"};
            r.labels = {"Die Herrschaft des Feuers", "Zweiter Film"};
            r.folders = {"", "Serien/Andor"};
            r.statuses = {"unverified", "recording_present"};
            return r;
        });
    const auto ok = get("actor:one", scopes);
    assert(ok.statusCode == 200);
    assert(ok.body.find("\"sourceId\":\"source_00000000001\"") !=
           std::string::npos);
    assert(ok.body.find("\"sourceId\":\"source_00000000002\"") !=
           std::string::npos);
    assert(ok.body.find("\"label\":\"Die Herrschaft des Feuers\"") !=
           std::string::npos);
    assert(ok.body.find("\"label\":\"Zweiter Film\"") !=
           std::string::npos);
    assert(ok.body.find("\"folder\":\"\"") != std::string::npos);
    assert(ok.body.find("\"folder\":\"Serien/Andor\"") != std::string::npos);
    assert(ok.body.find("\"recordingStatus\":\"recording_present\"") !=
           std::string::npos);
    assert(ok.body.find("\"recordingStatus\":\"unverified\"") !=
           std::string::npos);
    assert(ok.body.find("/media/input") == std::string::npos);
    assert(ok.headers.at("Cache-Control") == "no-store");
    assert(ok.body.find("\"stale\":true") == std::string::npos);
    staleCatalog = true;
    const auto oldButReadable = get("actor:one", scopes);
    assert(oldButReadable.statusCode == 200);
    assert(oldButReadable.body.find("\"stale\":true") != std::string::npos);
    assert(oldButReadable.body.find("source_00000000001") != std::string::npos);
    staleCatalog = false;
    assert(get("actor:two", scopes).statusCode == 403);
    assert(get("actor:one", {"backend-b"}).statusCode == 404);
    assert(get("", scopes).statusCode == 401);
    ApiResponse query;
    assert(api.tryHandleGet(url + "?path=/tmp", "actor:one", "", "",
                            query, "", "", scopes));
    assert(query.statusCode == 400);
    // Folder traversal or absolute paths in an Agent observation must not
    // reach Public v1, even as advisory metadata.
    api.registerMediaImportSourceLookup(
        [](const std::string&, const std::string&) {
            PublicApiRuntime::MediaImportSourceResult bad;
            bad.status = PublicApiRuntime::MediaImportSourceStatus::ok;
            bad.ids = {"source_00000000001"};
            bad.labels = {"Film"};
            bad.folders = {"../etc"};
            return bad;
        });
    assert(get("actor:one", scopes).statusCode == 503);
    // A status must be sourced from the same bounded snapshot and revision.
    api.registerMediaImportSourceLookup(
        [](const std::string&, const std::string&) {
            PublicApiRuntime::MediaImportSourceResult bad;
            bad.status = PublicApiRuntime::MediaImportSourceStatus::ok;
            bad.ids = {"source_00000000001"};
            bad.labels = {"Film"};
            bad.statuses = {"trusted_imported_without_readback"};
            return bad;
        });
    assert(get("actor:one", scopes).statusCode == 503);
    // Three bounded pages, no path disclosure and no import side effect.
    // Continuation is bound to the exact actor/backend/source observation.
    std::vector<std::string> many;
    std::vector<std::string> manyFolders(70, "");
    std::vector<std::string> manyStatuses(70, "unverified");
    bool scanComplete = false;
    for (int i = 0; i < 70; ++i)
        many.push_back("source_" + std::to_string(10000000000ULL + i));
    api.registerMediaImportSourceLookup(
        [&many, &manyFolders, &manyStatuses, &scanComplete](const std::string& actor, const std::string& backend)
        {
            PublicApiRuntime::MediaImportSourceResult result;
            if (actor != "actor:one" || backend != "backend-a") {
                result.status = PublicApiRuntime::MediaImportSourceStatus::forbidden;
                return result;
            }
            result.status = PublicApiRuntime::MediaImportSourceStatus::ok;
            result.ids = many;
            result.labels.resize(many.size(), "Film");
            result.folders = manyFolders;
            result.statuses = manyStatuses;
            result.partial = !scanComplete;
            return result;
        });
    const auto getPage = [&](const std::string& target,
                             const std::string& actor = "actor:one")
    {
        ApiResponse response;
        assert(api.tryHandleGet(target, actor, "page", "",
                                response, "", "", scopes));
        return response;
    };
    const auto nextFrom = [](const std::string& body) {
        const std::string name = "\"next\":\"";
        const auto start = body.find(name);
        if (start == std::string::npos) return std::string();
        const auto offset = start + name.size();
        const auto end = body.find('"', offset);
        assert(end != std::string::npos);
        return body.substr(offset, end - offset);
    };
    auto first = getPage(url);
    assert(first.statusCode == 200);
    assert(first.body.find("\"meta\":{\"partial\":true}") != std::string::npos);
    assert(first.body.find(many[0]) != std::string::npos);
    assert(first.body.find(many[31]) != std::string::npos);
    assert(first.body.find(many[32]) == std::string::npos);
    const std::string secondUrl = nextFrom(first.body);
    assert(secondUrl.find(url + "?cursor=mi1_") == 0);
    // A server-confirmed folder change makes the previously issued cursor
    // stale even if IDs and display names did not change.
    manyFolders[0] = "Serien/Andor";
    assert(getPage(secondUrl).statusCode == 409);
    manyFolders[0].clear();
    // Recording presence is part of the very same snapshot revision.
    manyStatuses[0] = "recording_present";
    assert(getPage(secondUrl).statusCode == 409);
    manyStatuses[0] = "unverified";
    auto second = getPage(secondUrl);
    assert(second.statusCode == 200);
    assert(second.body.find(many[32]) != std::string::npos);
    assert(second.body.find(many[63]) != std::string::npos);
    const std::string thirdUrl = nextFrom(second.body);
    assert(!thirdUrl.empty());
    auto third = getPage(thirdUrl);
    assert(third.statusCode == 200);
    assert(third.body.find(many[64]) != std::string::npos);
    assert(third.body.find(many[69]) != std::string::npos);
    assert(nextFrom(third.body).empty());
    // A change in completion status is a different authoritative snapshot;
    // an old continuation must never silently cross it.
    scanComplete = true;
    assert(getPage(url).body.find("\"meta\":{\"partial\":false}") != std::string::npos);
    assert(getPage(secondUrl).statusCode == 409);
    scanComplete = false;
    assert(getPage(secondUrl, "actor:two").statusCode == 403);
    many[0] = "source_09999999999"; // valid new snapshot, old continuation stale
    assert(getPage(secondUrl).statusCode == 409);
    assert(getPage(url + "?cursor=mi1_bad_32").statusCode == 409);
    assert(getPage(url + "?cursor=mi1_1_00").statusCode == 409);
    assert(getPage(url + "?cursor=mi1_1_32&path=/tmp").statusCode == 400);
    assert(getPage(url + "?cursor=").statusCode == 400);
    many.resize(101, "source_unreachable_00000000000");
    assert(getPage(url).statusCode == 503);
    ApiResponse post;
    assert(api.tryHandlePost(url, "", "", post));
    assert(post.statusCode == 405);
    ApiResponse patch;
    assert(api.tryHandleUnsupportedMethod("PATCH", url, "", "", patch));
    assert(patch.statusCode == 405);
    // UX4.2: one opaque source ID, same public actor/backend scopes.
    // The preview request is read-only and may remain pending until the
    // authenticated Agent's next source observation.
    const std::string preview =
        url + "/source_00000000001/preview";
    api.resetMediaImportPreviewLookup();
    assert(getPage(preview).statusCode == 503);
    api.registerMediaImportPreviewLookup(
        [](const std::string& actor, const std::string& backend,
           const std::string& source)
        {
            PublicApiRuntime::MediaImportPreviewResult result;
            if (actor != "actor:one" || backend != "backend-a" ||
                source != "source_00000000001") {
                result.status =
                    PublicApiRuntime::MediaImportPreviewStatus::forbidden;
                return result;
            }
            result.status = PublicApiRuntime::MediaImportPreviewStatus::pending;
            return result;
        });
    auto pending = getPage(preview);
    assert(pending.statusCode == 202);
    assert(pending.body.find("\"state\":\"pending\"") != std::string::npos);
    assert(pending.headers.at("Cache-Control") == "no-store");
    assert(getPage(preview, "actor:two").statusCode == 403);
    // The same authorized read-only preview can temporarily lose its Agent.
    // A provider outage is 503, not an invented permission revocation.
    api.registerMediaImportPreviewLookup(
        [](const std::string&, const std::string&, const std::string&) {
            return PublicApiRuntime::MediaImportPreviewResult{};
        });
    assert(getPage(preview).statusCode == 503);
    // Recovery uses the same canonical route, actor and backend scope.
    assert(getPage(preview + "?path=/etc").statusCode == 400);
    ApiResponse wrongBackend;
    assert(api.tryHandleGet(preview, "actor:one", "", "",
                            wrongBackend, "", "", {"backend-b"}));
    assert(wrongBackend.statusCode == 404);
    api.registerMediaImportPreviewLookup(
        [](const std::string&, const std::string&, const std::string& source)
        {
            PublicApiRuntime::MediaImportPreviewResult result;
            result.status = PublicApiRuntime::MediaImportPreviewStatus::ready;
            result.sourceId = source;
            result.inspectionStatus = "duplicate";
            result.title = "Andor";
            result.targetFolder = "Serien/Andor";
            result.videoCodec = "vp9";
            result.sourceBytes = 1234567;
            result.availableBytes = 9000000000ULL;
            result.requiredBytes = 7000000000ULL;
            result.videoReencodeRequired = true;
            result.nfoOutline = "Kurz & klar";
            result.nfoPlot = "Lokaler <Plot> & Daten";
            result.nfoYear = "2024";
            result.nfoGenre = "Drama";
            result.durationSeconds = 3120;
            result.posterDataUri = "data:image/jpeg;base64,/9j/2Q==";
            result.fanartDataUri = "data:image/png;base64,iVBORw0KGgo=";
            return result;
        });
    const auto ready = getPage(preview);
    assert(ready.statusCode == 200);
    assert(ready.body.find("\"state\":\"ready\"") != std::string::npos);
    assert(ready.body.find("\"status\":\"duplicate\"") != std::string::npos);
    assert(ready.body.find("\"targetFolder\":\"Serien/Andor\"") !=
           std::string::npos);
    assert(ready.body.find("\"videoCodec\":\"vp9\"") != std::string::npos);
    assert(ready.body.find("\"nfoOutline\":\"Kurz & klar\"") != std::string::npos);
    assert(ready.body.find("\"nfoPlot\":\"Lokaler <Plot> & Daten\"") != std::string::npos);
    assert(ready.body.find("\"nfoYear\":\"2024\"") != std::string::npos);
    assert(ready.body.find("\"nfoGenre\":\"Drama\"") != std::string::npos);
    assert(ready.body.find("\"durationSeconds\":3120") != std::string::npos);
    assert(ready.body.find("data:image/jpeg;base64,/9j/2Q==") != std::string::npos);
    assert(ready.body.find("data:image/png;base64,iVBORw0KGgo=") != std::string::npos);
    assert(ready.body.find("\"sourceBytes\":1234567") != std::string::npos);
    assert(ready.body.find("\"videoReencodeRequired\":true") !=
           std::string::npos);
    assert(ready.body.find("/srv/vdr") == std::string::npos);
    // Capacity failures and unavailable sources are still read-only
    // observations, not implicit import/encoding authorizations.
    for (const auto& inspection : {"insufficient", "unavailable"}) {
        const std::string inspectionStatus(inspection);
        api.registerMediaImportPreviewLookup(
            [inspectionStatus](const std::string&, const std::string&,
                               const std::string& source)
            {
                PublicApiRuntime::MediaImportPreviewResult result;
                result.status = PublicApiRuntime::MediaImportPreviewStatus::ready;
                result.sourceId = source;
                result.inspectionStatus = inspectionStatus;
                if (inspectionStatus == "unavailable")
                    result.failureCode = "ffprobe_failed";
                if (inspectionStatus == "insufficient") {
                    result.title = "Andor";
                    result.targetFolder = "Serien/Andor/Staffel 1/Film";
                    result.videoCodec = "hevc";
                    result.sourceBytes = 1234;
                    result.availableBytes = 1000;
                    result.requiredBytes = 9000;
                }
                return result;
            });
        const auto checked = getPage(preview);
        assert(checked.statusCode == 200);
        assert(checked.body.find(
            "\"status\":\"" + inspectionStatus + "\"") !=
            std::string::npos);
        assert(checked.body.find("/srv/") == std::string::npos);
        if (inspectionStatus == "unavailable")
            assert(checked.body.find("\"failureCode\":\"ffprobe_failed\"") !=
                   std::string::npos);
    }
    ApiResponse unsafePost;
    assert(api.tryHandlePost(preview, "", "", unsafePost));
    assert(unsafePost.statusCode == 405);
    api.resetMediaImportPreviewLookup();
    api.resetMediaImportSourceLookup();
}

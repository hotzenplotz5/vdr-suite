#include "PublicApiRuntime.h"
#include <cassert>
#include <string>
#include <vector>

int main()
{
    auto& api = PublicApiRuntime::instance();
    api.resetGenreCollectionLookup();
    api.registerGenreCollectionLookup([](const PublicGenreCollectionRequest& request) {
        PublicGenreCollectionResult result;
        if (request.backendId != "home") return result;
        result.status = PublicGenreCollectionStatus::ok;
        if (request.recordings)
        {
            if (request.genreId != "crime") {
                result.status = PublicGenreCollectionStatus::invalid;
                return result;
            }
            result.totalCount = 1033U;
            for (std::size_t i = request.offset;
                 i < result.totalCount && i < request.offset + request.limit;
                 ++i)
            {
                PublicGenreRecordingItem item;
                item.backendId = "home";
                item.recordingId = "rec_0123456789abcdef0123456789abcdef";
                item.title = "Aufnahme";
                item.recordedAt = "2026-10-09";
                result.recordings.push_back(std::move(item));
            }
        }
        else
        {
            result.totalCount = 1U;
            if (request.offset == 0U)
            {
                PublicGenreItem item;
                item.genreId = "crime";
                item.label = "Krimi";
                item.labelDe = "Krimi";
                item.labelEn = "Crime";
                item.count = 1033U;
                result.genres.push_back(std::move(item));
            }
        }
        return result;
    });
    ApiResponse response;
    assert(api.tryHandleGet("/api/v1/capabilities", "actor:tv", "cap", "",
        response, "", "", {"home"}));
    assert(response.body.find("public-api.recording-genres-read") !=
        std::string::npos);
    ApiResponse genres;
    assert(api.tryHandleGet("/api/v1/genres?backendId=home&limit=30",
        "actor:tv","genre","",genres,"","",{"home"}));
    assert(genres.statusCode == 200);
    assert(genres.body.find("Krimi") != std::string::npos);
    assert(genres.body.find("1033") != std::string::npos);
    assert(genres.body.find("backendNativeId") == std::string::npos);
    ApiResponse recordings;
    assert(api.tryHandleGet(
        "/api/v1/genres/recordings?backendId=home&genreId=crime&limit=30&offset=0",
        "actor:tv","members","",recordings,"","",{"home"}));
    assert(recordings.statusCode == 200);
    assert(recordings.body.find("\"totalCount\":1033") != std::string::npos);
    assert(recordings.body.find("\"hasMore\":true") != std::string::npos);
    assert(recordings.body.find("recordingId") != std::string::npos);
    assert(recordings.body.find("backendNativeId") == std::string::npos);
    assert(recordings.body.find("recordingPath") == std::string::npos);
    for (const auto& invalid : {
        "/api/v1/genres?backendId=home&limit=31",
        "/api/v1/genres?backendId=home&genreId=crime",
        "/api/v1/genres/recordings?backendId=home&genreId=/private",
        "/api/v1/genres/recordings?backendId=home&limit=30",
        "/api/v1/genres?backendId=outside"})
    {
        ApiResponse bad;
        assert(api.tryHandleGet(invalid,"actor:tv","invalid","",bad,"","",
            {"home"}));
        assert(bad.statusCode == 400);
    }
    ApiResponse unauth;
    assert(api.tryHandleGet("/api/v1/genres?backendId=home",
        "","unauth","",unauth,"","",{"home"}));
    assert(unauth.statusCode == 401);
    api.resetGenreCollectionLookup();
    return 0;
}

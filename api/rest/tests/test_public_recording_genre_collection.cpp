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
                item.edited = true;
                item.recordedAt = "2026-10-09";
                item.sizeMb = 2048;
                item.genreIds = {"crime", "series"};
                item.cardTitle = "Testserie";
                item.cardMediaType = "series";
                if (i == 0U)
                {
                    item.preferredArtworkId = "fedcba9876543210fedcba9876543210";
                    item.seriesGroupKey = "sg1_0123456789abcdef0123456789abcdef";
                    item.seriesFolderTitle = "Testserie";
                    item.seriesAncestorFolderIds = {
                        "fld1_0123456789abcdef0123456789abcdef",
                        "fld1_fedcba9876543210fedcba9876543210"};
                    item.seriesNativeMetadataAvailable = true;
                    item.seriesHierarchyOverrideAvailable = true;
                    item.seriesHierarchyGroupType = "special";
                    item.seriesHierarchyGroupLabel = "Pilot / Miniserie";
                    item.seriesHierarchySortOrder = -100;
                    item.seriesHierarchyEpisodeStart = 1;
                    item.seriesHierarchyEpisodeEnd = 2;
                    item.seriesHierarchyRevision = 3;
                }
                if (i == 1032U)
                    item.preferredArtworkId = "/private/poster.jpg";
                item.episodeName = "Folge sieben";
                item.seasonNumber = 2;
                item.episodeNumber = 7;
                result.recordings.push_back(std::move(item));
            }
        }
        else
        {
            result.totalCount = 1U;
            result.totalItems = 1033U;
            result.assignmentCount = 1050U;
            if (request.offset == 0U)
            {
                PublicGenreItem item;
                item.genreId = "crime";
                item.label = "Krimi";
                item.labelDe = "Krimi";
                item.labelEn = "Crime";
                item.count = 1033U;
                item.known = true;
                item.activeCount = 1000U;
                item.staleCount = 20U;
                item.conflictCount = 13U;
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
    assert(genres.body.find("\"totalItems\":1033") != std::string::npos);
    assert(genres.body.find("\"assignmentCount\":1050") != std::string::npos);
    assert(genres.body.find("\"known\":true") != std::string::npos);
    assert(genres.body.find("\"activeCount\":1000") != std::string::npos);
    assert(genres.body.find("\"staleCount\":20") != std::string::npos);
    assert(genres.body.find("\"conflictCount\":13") != std::string::npos);
    assert(genres.body.find("\"displayOrder\":0") != std::string::npos);
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
    assert(recordings.body.find("assignmentCount") == std::string::npos);
    assert(recordings.body.find("displayOrder") == std::string::npos);
    assert(recordings.body.find("\"edited\":true") != std::string::npos);
    assert(recordings.body.find("\"sizeMb\":2048") != std::string::npos);
    assert(recordings.body.find("\"genreIds\":[\"crime\",\"series\"]") != std::string::npos);
    assert(recordings.body.find("\"cardMetadata\":{\"mediaType\":\"series\",\"title\":\"Testserie\"") != std::string::npos);
    assert(recordings.body.find("\"episodeName\":\"Folge sieben\"") != std::string::npos);
    assert(recordings.body.find("\"seasonNumber\":2,\"episodeNumber\":7") != std::string::npos);
    assert(recordings.body.find("resourceKey") == std::string::npos);
    assert(recordings.body.find("providerId") == std::string::npos);
    const std::string artworkField =
        "\"preferredArtworkId\":\"fedcba9876543210fedcba9876543210\"";
    const auto artworkPos = recordings.body.find(artworkField);
    assert(artworkPos != std::string::npos);
    assert(recordings.body.find("\"preferredArtworkId\"", artworkPos + artworkField.size()) ==
        std::string::npos);
    ApiResponse largePage;
    assert(api.tryHandleGet(
        "/api/v1/genres/recordings?backendId=home&genreId=crime&limit=100&offset=100",
        "actor:tv","members-large-page","",largePage,"","",{"home"}));
    assert(largePage.statusCode == 200);
    assert(largePage.body.find("\"limit\":100") != std::string::npos);
    ApiResponse withoutArtwork;
    assert(api.tryHandleGet(
        "/api/v1/genres/recordings?backendId=home&genreId=crime&limit=1&offset=1",
        "actor:tv","members-no-artwork","",withoutArtwork,"","",{"home"}));
    assert(withoutArtwork.statusCode == 200);
    assert(withoutArtwork.body.find("preferredArtworkId") == std::string::npos);
    assert(withoutArtwork.body.find("seriesHierarchyOverride") == std::string::npos);
    assert(withoutArtwork.body.find("seriesGrouping") == std::string::npos);
    assert(recordings.body.find("\"seriesGrouping\":{\"groupKey\":\"sg1_0123456789abcdef0123456789abcdef\",\"folderTitle\":\"Testserie\",\"nativeMetadataAvailable\":true,\"ancestorFolderIds\":[\"fld1_0123456789abcdef0123456789abcdef\",\"fld1_fedcba9876543210fedcba9876543210\"]}") != std::string::npos);
    assert(recordings.body.find("\"seriesHierarchyOverride\":{\"available\":true,\"groupType\":\"special\",\"seasonNumber\":0,\"groupLabel\":\"Pilot / Miniserie\",\"sortOrder\":-100,\"episodeStart\":1,\"episodeEnd\":2,\"revision\":3}") != std::string::npos);
    ApiResponse badArtwork;
    assert(api.tryHandleGet(
        "/api/v1/genres/recordings?backendId=home&genreId=crime&limit=1&offset=1032",
        "actor:tv","members-invalid-artwork","",badArtwork,"","",{"home"}));
    assert(badArtwork.statusCode == 503);
    assert(badArtwork.body.find("/private/poster.jpg") == std::string::npos);
    for (const auto& invalid : {
        "/api/v1/genres?backendId=home&limit=31",
        "/api/v1/genres/recordings?backendId=home&genreId=crime&limit=101",
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
    // Same canonical Web EPG taxonomy projected to an independent,
    // grant-scoped Public-v1 route (never a client-side parallel index).
    api.resetEpgGenreLookup();
    api.registerEpgGenreLookup([](const PublicEpgGenreRequest& request) {
        PublicEpgGenreResult result;
        if (request.backendId != "home") return result;
        result.status = PublicEpgGenreStatus::ok;
        result.fromTime = request.fromTime ? request.fromTime : 1792060000;
        result.untilTime = request.untilTime ? request.untilTime : 1792232800;
        result.totalItems = 7U;
        result.assignmentCount = 10U;
        if (request.events)
        {
            result.totalCount = 2U;
            if (request.offset == 0U && request.contentClass == "series")
                for (int i = 0; i < 2; ++i)
                {
                    PublicEpgGenreEvent item;
                    item.backendId = "home";
                    item.channelId = "S19.2E-1-1";
                    item.eventId = std::to_string(i + 1);
                    item.title = "Young Sheldon";
                    item.subtitle = "Hobbits und die Wissenschaftspause";
                    item.startTime = "1792060000";
                    item.endTime = "1792062000";
                    result.events.push_back(item);
                }
        }
        else
        {
            result.totalCount = 2U;
            if (request.offset == 0U)
            {
                PublicEpgGenreItem parent;
                parent.genreId = "movie";
                parent.contentClass = "movie";
                parent.label = "Film";
                parent.count = 7U;
                result.genres.push_back(parent);
                PublicEpgGenreItem child;
                child.genreId = "adventure";
                child.contentClass = "movie";
                child.label = "Film › Abenteuer";
                child.count = 3U;
                result.genres.push_back(child);
            }
        }
        return result;
    });
    ApiResponse epgCaps;
    assert(api.tryHandleGet("/api/v1/capabilities",
        "actor:tv","epg-caps","",epgCaps,"","",{"home"}));
    assert(epgCaps.body.find("public-api.epg-genres-read") != std::string::npos);
    ApiResponse epgGenres;
    assert(api.tryHandleGet(
        "/api/v1/genres/epg?backendId=home&limit=30&offset=0&fromTime=1792060000&untilTime=1792232800",
        "actor:tv","epg-genres","",epgGenres,"","",{"home"}));
    assert(epgGenres.statusCode == 200);
    assert(epgGenres.body.find("\"fromTime\":1792060000") != std::string::npos);
    assert(epgGenres.body.find("\"untilTime\":1792232800") != std::string::npos);
    assert(epgGenres.body.find("\"totalItems\":7") != std::string::npos);
    assert(epgGenres.body.find("\"assignmentCount\":10") != std::string::npos);
    assert(epgGenres.body.find("Film") != std::string::npos);
    assert(epgGenres.body.find("Abenteuer") != std::string::npos);
    assert(epgGenres.body.find("backendNativeId") == std::string::npos);
    ApiResponse epgEvents;
    assert(api.tryHandleGet(
        "/api/v1/genres/epg/events?backendId=home&contentClass=series&limit=30&offset=0",
        "actor:tv","epg-events","",epgEvents,"","",{"home"}));
    assert(epgEvents.statusCode == 200);
    assert(epgEvents.body.find("\"totalCount\":2") != std::string::npos);
    assert(epgEvents.body.find("Hobbits und die Wissenschaftspause") !=
        std::string::npos);
    assert(epgEvents.body.find("recordingPath") == std::string::npos);
    for (const auto& invalid : {
        "/api/v1/genres/epg?backendId=home&contentClass=movie",
        "/api/v1/genres/epg/events?backendId=home&contentClass=unknown",
        "/api/v1/genres/epg/events?backendId=home&genreId=adventure",
        "/api/v1/genres/epg/events?backendId=home&contentClass=series&genreId=adventure",
        "/api/v1/genres/epg?backendId=home&limit=31",
        "/api/v1/genres/epg?backendId=home&fromTime=1792060000",
        "/api/v1/genres/epg?backendId=home&fromTime=1792060000&untilTime=1792060000",
        "/api/v1/genres/epg?backendId=other"})
    {
        ApiResponse bad;
        assert(api.tryHandleGet(
            invalid,"actor:tv","epg-invalid","",bad,"","",{"home"}));
        assert(bad.statusCode == 400);
    }
    ApiResponse noActor;
    assert(api.tryHandleGet("/api/v1/genres/epg?backendId=home",
        "", "epg-anon", "", noActor, "", "", {"home"}));
    assert(noActor.statusCode == 401);
    api.resetEpgGenreLookup();
    api.resetGenreCollectionLookup();
    return 0;
}

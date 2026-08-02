#include "Database.h"
#include "GenreIndexRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

namespace
{
GenreEvidenceInput evidence(
    const std::string& providerId,
    const std::string& genre,
    std::int64_t observedAt,
    const std::string& sourceKind = "test-provider")
{
    GenreEvidenceInput input;
    input.backendId = "default";
    input.targetType = "program-event";
    input.resourceKey = "C-1\n100";
    input.nativeId = "100";
    input.channelId = "C-1";
    input.startTime = 1000;
    input.endTime = 2000;
    input.providerId = providerId;
    input.sourceKind = sourceKind;
    input.originalValues = {genre};
    input.confidence = 0.9;
    input.observedAt = observedAt;
    return input;
}

GenreOverviewEntry findGenre(
    const GenreOverview& overview,
    const std::string& genreId)
{
    for (const GenreOverviewEntry& entry : overview.genres)
    {
        if (entry.genreId == genreId) return entry;
    }
    return {};
}
}

int main()
{
    const std::string filename =
        "/tmp/vdr-suite-genre-conflict-test.sqlite";
    std::remove(filename.c_str());

    Database database;
    assert(database.open(filename));
    assert(database.execute(
        "CREATE TABLE epg_events("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,title TEXT,"
        "subtitle TEXT,description TEXT,start_time TEXT,end_time TEXT,"
        "duration_seconds INTEGER,content_descriptors TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_event_artwork("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,provider TEXT,"
        "path TEXT,width INTEGER,height INTEGER,resolved_at INTEGER,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "INSERT INTO epg_events VALUES("
        "'default','C-1','100','Series','Episode','Text',"
        "'1000','2000',1000,'Movie');"));

    GenreIndexRepository repository(database);
    assert(repository.ensureSchema());

    assert(repository.replaceEvidence(evidence(
        "vdr-epg",
        "Drama",
        1000,
        "dvb-content-descriptor")));
    assert(repository.replaceEvidence(evidence(
        "tvscraper",
        "Action",
        1100,
        "scraper-metadata")));

    GenreOverview conflicting = repository.overview(
        "default", "program-event", 900, 2100);
    assert(findGenre(conflicting, "drama").conflictCount == 1);
    assert(findGenre(conflicting, "action").conflictCount == 1);

    assert(repository.replaceEvidence(evidence(
        "tvscraper-media-type",
        "Series",
        1150,
        "scraper-media-type")));
    GenreOverview withMediaType = repository.overview(
        "default", "program-event", 900, 2100);
    assert(findGenre(withMediaType, "series").conflictCount == 0);
    assert(repository.reconcileEpgBrowseClassification(
        "default", "C-1\n100"));

    GenreEpgBrowseOverview browse = repository.epgBrowseOverview(
        "default", 900, 2100);
    assert(browse.categories.size() == 4);
    assert(browse.categories[0].id == "movie");
    assert(browse.categories[0].itemCount == 0);
    assert(browse.categories[1].id == "series");
    assert(browse.categories[1].itemCount == 1);

    assert(repository.replaceEvidence(evidence(
        "tvscraper",
        "Drama",
        1200,
        "scraper-metadata")));
    GenreOverview agreed = repository.overview(
        "default", "program-event", 900, 2100);
    assert(findGenre(agreed, "drama").conflictCount == 0);
    assert(findGenre(agreed, "drama").activeCount == 1);

    GenreEvidenceInput stale = evidence(
        "tvscraper",
        "",
        1300,
        "scraper-metadata");
    stale.originalValues.clear();
    stale.state = "stale";
    assert(repository.replaceEvidence(stale));

    GenreOverview degraded = repository.overview(
        "default", "program-event", 900, 2100);
    assert(findGenre(degraded, "drama").conflictCount == 0);
    assert(findGenre(degraded, "drama").staleCount == 1);
    assert(findGenre(degraded, "drama").activeCount == 1);

    database.close();
    std::remove(filename.c_str());
    std::cout << "genre conflict repository hierarchy ok\n";
    return 0;
}

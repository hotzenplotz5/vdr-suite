#include "CanonicalGenreRegistry.h"
#include "Database.h"
#include "GenreIndexRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace
{
void createSourceSchemas(Database& database)
{
    assert(database.execute(
        "CREATE TABLE vdr_recording_cache(backend_id TEXT,cache_key TEXT,recording_id TEXT,backend_native_id TEXT,title TEXT,path TEXT,start_time TEXT,duration_seconds INTEGER,size_mb INTEGER,metadata_payload TEXT,PRIMARY KEY(backend_id,cache_key));"
        "CREATE TABLE vdr_recording_native_metadata(backend_id TEXT,recording_key TEXT,backend_native_id TEXT,content_state TEXT,last_attempt_state TEXT,provider TEXT,PRIMARY KEY(backend_id,recording_key));"
        "CREATE TABLE vdr_recording_native_text_list(backend_id TEXT,recording_key TEXT,kind TEXT,ordinal INTEGER,value TEXT,PRIMARY KEY(backend_id,recording_key,kind,ordinal));"
        "CREATE TABLE epg_events(backend_id TEXT,channel_id TEXT,event_id TEXT,title TEXT,subtitle TEXT,description TEXT,start_time TEXT,end_time TEXT,duration_seconds INTEGER,content_descriptors TEXT,PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_event_artwork(backend_id TEXT,channel_id TEXT,event_id TEXT,provider TEXT,path TEXT,width INTEGER,height INTEGER,resolved_at INTEGER,PRIMARY KEY(backend_id,channel_id,event_id));"));
}

void seed(Database& database)
{
    assert(database.execute(
        "INSERT INTO vdr_recording_cache VALUES"
        "('a','r1','id1','native1','Space','Movies/Space','100',5400,1000,'{}'),"
        "('a','r2','id2','native2','Comedy','Movies/Comedy','200',3600,500,'{}'),"
        "('a','r3','id3','native3','No genre','Movies/Unknown','300',3600,500,'{}'),"
        "('b','r1','id4','native4','Other backend','Movies/Space','400',3600,500,'{}');"
        "INSERT INTO vdr_recording_native_metadata VALUES"
        "('a','m1','native1','found','success','tvscraper'),"
        "('a','m2','native2','found','failure','tvscraper'),"
        "('a','m3','native3','not-found','success','tvscraper'),"
        "('b','m4','native4','found','success','tvscraper');"
        "INSERT INTO vdr_recording_native_text_list VALUES"
        "('a','m1','genre',0,'Science Fiction'),"
        "('a','m1','genre',1,'Drama'),"
        "('a','m2','genre',0,'Komödie'),"
        "('b','m4','genre',0,'Sci-Fi');"
        "INSERT INTO epg_events VALUES"
        "('a','C1','10','News','Heute','Text','1000','2000',1000,'News'),"
        "('a','C2','20','Movie','Später','Text','1500','2500',1000,'Film/Drama\nDetektiv/Thriller'),"
        "('a','C3','30','Hartz Rot Gold','Episode','Text','1600','2600',1000,'Film/Drama'),"
        "('a','C4','40','Doku','Natur','Text','1700','2700',1000,'Doku/Ökonomie'),"
        "('a','C5','50','Live sport','Live','Text','1800','2800',1000,'Sport'),"
        "('b','C1','10','Other','Other','Text','1000','2000',1000,'Film/Action');"
        "INSERT INTO epg_event_artwork VALUES"
        "('a','C2','20','tvscraper','/tmp/epg.jpg',1280,720,1700);"));
}

GenreEvidenceInput scraperEvidence(
    const std::string& resourceKey,
    const std::string& nativeId,
    const std::string& channelId,
    std::int64_t startTime,
    std::int64_t endTime,
    const std::string& providerId,
    const std::string& sourceKind,
    const std::vector<std::string>& values)
{
    GenreEvidenceInput input;
    input.backendId = "a";
    input.targetType = "program-event";
    input.resourceKey = resourceKey;
    input.nativeId = nativeId;
    input.channelId = channelId;
    input.startTime = startTime;
    input.endTime = endTime;
    input.providerId = providerId;
    input.sourceKind = sourceKind;
    input.originalValues = values;
    input.confidence = 0.95;
    input.observedAt = 3000;
    return input;
}

const GenreBrowseCategory& category(
    const GenreEpgBrowseOverview& overview,
    const std::string& id)
{
    for (const GenreBrowseCategory& entry : overview.categories)
    {
        if (entry.id == id) return entry;
    }
    assert(false);
    return overview.categories.front();
}
}

int main()
{
    CanonicalGenreRegistry registry;
    assert(registry.classify("Science Fiction").id == "science-fiction");
    assert(registry.classify("Komödie").id == "comedy");
    assert(registry.classify("Doku").id == "documentary");
    assert(registry.classify("Serien").id == "series");
    assert(registry.classify("Talk Show").id == "talk-show");
    assert(registry.classify("Reality-TV").id == "reality");
    assert(registry.classify("").id == "unclassified");

    const std::string filename = "/tmp/vdr-suite-genre-index-test.sqlite";
    std::remove(filename.c_str());
    {
        Database database;
        assert(database.open(filename));
        createSourceSchemas(database);
        seed(database);
        GenreIndexRepository repository(database);
        assert(repository.ensureSchema());
        assert(repository.synchronizeRecordingCache("a"));
        assert(repository.synchronizeRecordingCache("b"));
        assert(repository.synchronizeEpgCache("a", 900, 3000));
        assert(repository.synchronizeEpgCache("b", 900, 3000));

        GenreOverview recordings = repository.overview("a", "recording", 0, 0);
        assert(recordings.distinctItemCount == 3);
        assert(recordings.genres.size() == 4);

        GenreRecordingPage science = repository.recordingsByGenre("a", "science-fiction", 1, 0);
        assert(science.totalCount == 1);
        assert(science.recordings.front().title == "Space");

        GenreEpgBrowseOverview dvbBrowse = repository.epgBrowseOverview(
            "a", 900, 3000);
        assert(dvbBrowse.categories.size() == 4);
        assert(category(dvbBrowse, "movie").itemCount == 0);
        assert(category(dvbBrowse, "movie").children.empty());
        assert(category(dvbBrowse, "series").itemCount == 0);
        assert(category(dvbBrowse, "documentary").itemCount == 1);
        assert(category(dvbBrowse, "sports").itemCount == 1);

        GenreEpgPage dvbMovies = repository.epgByBrowse(
            "a", "movie", "", 900, 3000, 50, 0);
        assert(dvbMovies.totalCount == 0);
        assert(dvbMovies.events.empty());

        assert(repository.replaceEvidence(scraperEvidence(
            "C2\n20", "20", "C2", 1500, 2500,
            "tvscraper", "scraper-metadata", {"Thriller", "Drama"})));
        assert(repository.replaceEvidence(scraperEvidence(
            "C2\n20", "20", "C2", 1500, 2500,
            "tvscraper-media-type", "scraper-media-type", {"Movie"})));
        assert(repository.reconcileEpgBrowseClassification("a", "C2\n20"));

        assert(repository.replaceEvidence(scraperEvidence(
            "C3\n30", "30", "C3", 1600, 2600,
            "tvscraper", "scraper-metadata", {"Reality"})));
        assert(repository.replaceEvidence(scraperEvidence(
            "C3\n30", "30", "C3", 1600, 2600,
            "tvscraper-media-type", "scraper-media-type", {"Series"})));
        assert(repository.reconcileEpgBrowseClassification("a", "C3\n30"));

        GenreEpgBrowseOverview browse = repository.epgBrowseOverview("a", 900, 3000);
        assert(browse.categories.size() == 4);
        assert(category(browse, "movie").itemCount == 1);
        assert(category(browse, "series").itemCount == 1);
        assert(category(browse, "documentary").itemCount == 1);
        assert(category(browse, "sports").itemCount == 1);

        bool sawThriller = false;
        for (const GenreBrowseCategory& child : category(browse, "movie").children)
        {
            if (child.id == "thriller")
            {
                sawThriller = child.itemCount == 1;
            }
            assert(child.id != "news");
            assert(child.id != "reality");
            assert(child.id != "series");
        }
        assert(sawThriller);

        GenreEpgPage thriller = repository.epgByBrowse(
            "a", "movie", "thriller", 900, 3000, 50, 0);
        assert(thriller.totalCount == 1);
        assert(thriller.events.front().eventId == "20");
        assert(thriller.events.front().artworkAvailable);

        GenreEpgPage movies = repository.epgByBrowse(
            "a", "movie", "", 900, 3000, 50, 0);
        assert(movies.totalCount == 1);
        assert(movies.events.front().eventId == "20");

        GenreEpgPage series = repository.epgByBrowse(
            "a", "series", "", 900, 3000, 50, 0);
        assert(series.totalCount == 1);
        assert(series.events.front().eventId == "30");
        assert(series.events.front().title == "Hartz Rot Gold");

        GenreEpgPage documentary = repository.epgByBrowse(
            "a", "documentary", "", 900, 3000, 50, 0);
        assert(documentary.totalCount == 1);
        assert(documentary.events.front().eventId == "40");

        GenreEpgPage sports = repository.epgByBrowse(
            "a", "sports", "", 900, 3000, 50, 0);
        assert(sports.totalCount == 1);
        assert(sports.events.front().eventId == "50");

        GenreOverview rawEvidence = repository.overview(
            "a", "program-event", 900, 3000);
        bool newsPreserved = false;
        for (const GenreOverviewEntry& entry : rawEvidence.genres)
        {
            if (entry.genreId == "news") newsPreserved = entry.itemCount == 1;
        }
        assert(newsPreserved);

        GenreEpgPage isolated = repository.epgByBrowse(
            "b", "movie", "action", 900, 3000, 50, 0);
        assert(isolated.totalCount == 0);
        assert(isolated.events.empty());
    }

    {
        Database database;
        assert(database.open(filename));
        GenreIndexRepository repository(database);
        GenreEpgPage persisted = repository.epgByBrowse(
            "a", "series", "", 900, 3000, 50, 0);
        assert(persisted.totalCount == 1);
    }

    std::remove(filename.c_str());
    std::cout << "genre index repository hierarchy ok\n";
    return 0;
}

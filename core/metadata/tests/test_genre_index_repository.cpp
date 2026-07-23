#include "CanonicalGenreRegistry.h"
#include "Database.h"
#include "GenreIndexRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

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
        "('a','C1','10','Doku','Heute','Text','1000','2000',1000,'Doku'),"
        "('a','C2','20','Mystery','Später','Text','1500','2500',1000,'Mystery\nDrama'),"
        "('b','C1','10','Other','Other','Text','1000','2000',1000,'Action');"
        "INSERT INTO epg_event_artwork VALUES"
        "('a','C2','20','tvscraper','/tmp/epg.jpg',1280,720,1700);"));
}
}

int main()
{
    CanonicalGenreRegistry registry;
    assert(registry.classify("Science Fiction").id == "science-fiction");
    assert(registry.classify("Science-Fiction").id == "science-fiction");
    assert(registry.classify("Sci-Fi").id == "science-fiction");
    assert(registry.classify("Scifi").id == "science-fiction");
    assert(registry.classify("Komödie").id == "comedy");
    assert(registry.classify("Dokumentation").id == "documentary");
    assert(registry.classify("Doku").id == "documentary");
    assert(registry.classify("Serien").id == "series");
    assert(registry.classify("Historienfilm").id == "history");
    assert(registry.classify("Ärger & Spaß").id == "unknown-aerger-spass");
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
        assert(science.recordings.size() == 1);
        assert(science.recordings.front().title == "Space");
        assert(science.recordings.front().genreIds.size() == 2);

        GenreRecordingPage comedy = repository.recordingsByGenre("a", "comedy", 50, 0);
        assert(comedy.totalCount == 1);
        assert(comedy.recordings.front().title == "Comedy");

        GenreRecordingPage isolated = repository.recordingsByGenre("b", "science-fiction", 50, 0);
        assert(isolated.totalCount == 1);
        assert(isolated.recordings.front().title == "Other backend");

        GenreRecordingPage missing = repository.recordingsByGenre("a", "unclassified", 50, 0);
        assert(missing.totalCount == 1);

        GenreOverview epg = repository.overview("a", "program-event", 900, 3000);
        assert(epg.distinctItemCount == 2);
        GenreEpgPage drama = repository.epgByGenre("a", "drama", 900, 3000, 50, 0);
        assert(drama.totalCount == 1);
        assert(drama.events.front().eventId == "20");
        assert(drama.events.front().artworkAvailable);
        assert(drama.events.front().artworkWidth == 1280);
        assert(drama.events.front().artworkHeight == 720);

        const std::vector<GenreEpgRefreshCandidate> initialCandidates =
            repository.epgRefreshCandidates(
                "a", 900, 3000, "tvscraper", 2500, 64);
        assert(initialCandidates.size() == 2);
        assert(repository.epgRefreshCandidates(
            "b", 900, 3000, "tvscraper", 2500, 64).size() == 1);

        GenreEvidenceInput scraperEvidence;
        scraperEvidence.backendId = "a";
        scraperEvidence.targetType = "program-event";
        scraperEvidence.resourceKey = "C2\n20";
        scraperEvidence.nativeId = "20";
        scraperEvidence.channelId = "C2";
        scraperEvidence.startTime = 1500;
        scraperEvidence.endTime = 2500;
        scraperEvidence.providerId = "tvscraper";
        scraperEvidence.sourceKind = "scraper-metadata";
        scraperEvidence.originalValues = {"Mystery", "Drama"};
        scraperEvidence.confidence = 0.95;
        scraperEvidence.observedAt = 3000;
        assert(repository.replaceEvidence(scraperEvidence));
        assert(!repository.providerEvidenceNeedsRefresh(
            "a", "program-event", "C2\n20", "tvscraper", 2500));
        assert(repository.epgRefreshCandidates(
            "a", 900, 3000, "tvscraper", 2500, 64).size() == 1);

        scraperEvidence.state = "stale";
        scraperEvidence.originalValues.clear();
        scraperEvidence.observedAt = 4000;
        assert(repository.replaceEvidence(scraperEvidence));
        assert(repository.providerEvidenceNeedsRefresh(
            "a", "program-event", "C2\n20", "tvscraper", 2500));
        assert(repository.epgRefreshCandidates(
            "a", 900, 3000, "tvscraper", 3500, 64).size() == 1);

        GenreOverview staleOverview = repository.overview(
            "a", "program-event", 900, 3000);
        bool sawStaleDrama = false;
        for (const GenreOverviewEntry& entry : staleOverview.genres)
        {
            if (entry.genreId == "drama")
            {
                sawStaleDrama = entry.staleCount == 1;
            }
        }
        assert(sawStaleDrama);

        GenreEvidenceInput failedFirstAttempt;
        failedFirstAttempt.backendId = "a";
        failedFirstAttempt.targetType = "program-event";
        failedFirstAttempt.resourceKey = "C1\n10";
        failedFirstAttempt.nativeId = "10";
        failedFirstAttempt.channelId = "C1";
        failedFirstAttempt.startTime = 1000;
        failedFirstAttempt.endTime = 2000;
        failedFirstAttempt.providerId = "tvscraper";
        failedFirstAttempt.sourceKind = "scraper-metadata";
        failedFirstAttempt.state = "stale";
        failedFirstAttempt.observedAt = 5000;
        assert(repository.replaceEvidence(failedFirstAttempt));
        assert(repository.epgRefreshCandidates(
            "a", 900, 3000, "tvscraper", 4500, 64).empty());

        assert(repository.genreExists("science-fiction"));
        assert(!repository.genreExists("does-not-exist"));
    }

    {
        Database database;
        assert(database.open(filename));
        GenreIndexRepository repository(database);
        GenreRecordingPage persisted = repository.recordingsByGenre("a", "science-fiction", 50, 0);
        assert(persisted.totalCount == 1);
    }

    std::remove(filename.c_str());
    std::cout << "genre index repository ok\n";
    return 0;
}

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
        "CREATE TABLE vdr_recording_native_metadata(backend_id TEXT,recording_key TEXT,backend_native_id TEXT,content_state TEXT,last_attempt_state TEXT,provider TEXT,media_type TEXT,PRIMARY KEY(backend_id,recording_key));"
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
        "('a','r4','id5','native5','Late metadata','Movies/Late','350',3600,500,'{}'),"
        "('a','r5','id6','native6','Action/Folder Fallback','Action/Folder Fallback','375',3600,500,'{}'),"
        "('a','r6','id7','native7','Serien/Prestige/S01E01 Pilot','Serien/Prestige/S01E01_Pilot','390',3600,700,'{}'),"
        "('a','r7','id8','native8','Doku/Action','Doku/Action','395',3600,700,'{}'),"
        "('b','r1','id4','native4','Other backend','Movies/Space','400',3600,500,'{}');"
        "INSERT INTO vdr_recording_native_metadata VALUES"
        "('a','m1','native1','found','success','tvscraper',''),"
        "('a','m2','native2','found','failure','tvscraper',''),"
        "('a','m3','native3','not-found','success','tvscraper',''),"
        "('a','m6','native7','found','success','tvscraper','series'),"
        "('a','m7','native8','found','success','tvscraper','series'),"
        "('b','m4','native4','found','success','tvscraper','');"
        "INSERT INTO vdr_recording_native_text_list VALUES"
        "('a','m1','genre',0,'Science Fiction'),"
        "('a','m1','genre',1,'Drama'),"
        "('a','m2','genre',0,'Komödie'),"
        "('a','m6','genre',0,'Drama'),"
        "('a','m7','genre',0,'History'),"
        "('b','m4','genre',0,'Sci-Fi');"
        "INSERT INTO epg_events VALUES"
        "('a','C1','10','News','Heute','Text','1000','2000',1000,'News'),"
        "('a','C2','20','Movie','Später','Text','1500','2500',1000,'Film/Drama\nDetektiv/Thriller'),"
        "('a','C3','30','Hartz Rot Gold','Episode','Text','1600','2600',1000,'Film/Drama'),"
        "('a','C4','40','Doku','Natur','Text','1700','2700',1000,'Doku/Ökonomie'),"
        "('a','C5','50','Live sport','Live','Text','1800','2800',1000,'Sport'),"
        "('a','C6','60','Sportschau','Magazin','Text','1850','2850',1000,''),"
        "('a','C7','70','Tagesschau','','Text','1900','2900',1000,''),"
        "('a','C8','80','Long drama programme','Film','Text','1950','7350',5400,'Film/Drama'),"
        "('a','C9','90','Tagesthemen','','Text','2000','3000',1000,''),"
        "('a','C10','100','Death in Paradise','Episode','Text','2050','5650',3600,''),"
        "('a','C11','110','The Big Bang Theory','Episode','Text','2100','3900',1800,''),"
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
    const std::vector<std::string>& values,
    const std::string& state = "active")
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
    input.state = state;
    input.confidence = state == "stale" ? 0.80 : 0.95;
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

bool pageContainsTitle(
    const GenreEpgPage& page,
    const std::string& title)
{
    for (const GenreEpgItem& item : page.events)
    {
        if (item.title == title) return true;
    }
    return false;
}

bool candidatesContainEvent(
    const std::vector<GenreEpgRefreshCandidate>& candidates,
    const std::string& eventId)
{
    for (const GenreEpgRefreshCandidate& candidate : candidates)
    {
        if (candidate.eventId == eventId) return true;
    }
    return false;
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
        assert(repository.synchronizeEpgCache("a", 900, 8000));
        assert(repository.synchronizeEpgCache("b", 900, 8000));

        GenreOverview recordings = repository.overview("a", "recording", 0, 0);
        assert(recordings.distinctItemCount == 7);

        GenreRecordingPage science = repository.recordingsByGenre(
            "a", "science-fiction", 10, 0);
        assert(science.totalCount == 1);
        assert(science.recordings.front().title == "Space");

        GenreRecordingPage folderAction = repository.recordingsByGenre(
            "a", "action", 10, 0);
        assert(folderAction.totalCount == 1);
        assert(folderAction.recordings.front().title == "Action/Folder Fallback");

        GenreRecordingPage recordingSeries = repository.recordingsByGenre(
            "a", "series", 10, 0);
        assert(recordingSeries.totalCount == 1);
        assert(recordingSeries.recordings.front().title ==
            "Serien/Prestige/S01E01 Pilot");
        bool recordingSeriesHasDrama = false;
        bool recordingSeriesHasSeries = false;
        for (const std::string& genreId :
             recordingSeries.recordings.front().genreIds)
        {
            if (genreId == "drama") recordingSeriesHasDrama = true;
            if (genreId == "series") recordingSeriesHasSeries = true;
        }
        assert(recordingSeriesHasDrama);
        assert(recordingSeriesHasSeries);

        GenreRecordingPage recordingDocumentary = repository.recordingsByGenre(
            "a", "documentary", 10, 0);
        assert(recordingDocumentary.totalCount == 1);
        assert(recordingDocumentary.recordings.front().title == "Doku/Action");
        bool recordingDocumentaryHasHistory = false;
        bool recordingDocumentaryHasDocumentary = false;
        bool recordingDocumentaryHasSeries = false;
        for (const std::string& genreId :
             recordingDocumentary.recordings.front().genreIds)
        {
            if (genreId == "history") recordingDocumentaryHasHistory = true;
            if (genreId == "documentary") recordingDocumentaryHasDocumentary = true;
            if (genreId == "series") recordingDocumentaryHasSeries = true;
        }
        assert(recordingDocumentaryHasHistory);
        assert(recordingDocumentaryHasDocumentary);
        assert(!recordingDocumentaryHasSeries);

        GenreRecordingPage unclassifiedBefore = repository.recordingsByGenre(
            "a", "unclassified", 10, 0);
        assert(unclassifiedBefore.totalCount == 2);

        assert(database.execute(
            "INSERT INTO vdr_recording_native_metadata VALUES"
            "('a','m5','native5','found','success','tvscraper','');"
            "INSERT INTO vdr_recording_native_text_list VALUES"
            "('a','m5','genre',0,'Thriller');"));
        assert(repository.synchronizeRecordingCache("a"));

        GenreRecordingPage unclassifiedAfter = repository.recordingsByGenre(
            "a", "unclassified", 10, 0);
        assert(unclassifiedAfter.totalCount == 1);
        assert(unclassifiedAfter.recordings.front().title == "No genre");

        GenreRecordingPage lateThriller = repository.recordingsByGenre(
            "a", "thriller", 10, 0);
        assert(lateThriller.totalCount == 1);
        assert(lateThriller.recordings.front().title == "Late metadata");

        GenreEpgBrowseOverview dvbBrowse = repository.epgBrowseOverview(
            "a", 900, 8000);
        assert(dvbBrowse.categories.size() == 4);
        assert(category(dvbBrowse, "movie").itemCount == 1);
        assert(category(dvbBrowse, "series").itemCount == 0);
        assert(category(dvbBrowse, "documentary").itemCount == 1);
        assert(category(dvbBrowse, "sports").itemCount == 2);

        GenreEpgPage dvbMovies = repository.epgByBrowse(
            "a", "movie", "", 900, 8000, 50, 0);
        assert(dvbMovies.totalCount == 1);
        assert(dvbMovies.events.front().eventId == "20");
        assert(!pageContainsTitle(dvbMovies, "Long drama programme"));

        assert(repository.replaceEvidence(scraperEvidence(
            "C2\n20", "20", "C2", 1500, 2500,
            "tvscraper-media-type", "scraper-media-type", {"Movie"})));
        assert(repository.reconcileEpgBrowseClassification("a", "C2\n20"));

        const std::vector<GenreEpgRefreshCandidate> mediaTypeCandidates =
            repository.epgRefreshCandidates(
                "a", 900, 8000, "tvscraper-media-type", 2500, 64);
        assert(!candidatesContainEvent(mediaTypeCandidates, "20"));

        const std::vector<GenreEpgRefreshCandidate> metadataCandidates =
            repository.epgRefreshCandidates(
                "a", 900, 8000, "tvscraper", 2500, 64);
        assert(candidatesContainEvent(metadataCandidates, "20"));

        assert(repository.replaceEvidence(scraperEvidence(
            "C2\n20", "20", "C2", 1500, 2500,
            "tvscraper", "scraper-metadata", {"Thriller", "Drama"})));
        assert(repository.replaceEvidence(scraperEvidence(
            "C2\n20", "20", "C2", 1500, 2500,
            "tvscraper-media-type", "scraper-media-type", {"Movie"})));
        assert(repository.reconcileEpgBrowseClassification("a", "C2\n20"));

        const std::vector<GenreEpgRefreshCandidate> refreshedMetadataCandidates =
            repository.epgRefreshCandidates(
                "a", 900, 8000, "tvscraper", 2500, 64);
        assert(!candidatesContainEvent(refreshedMetadataCandidates, "20"));

        assert(repository.replaceEvidence(scraperEvidence(
            "C3\n30", "30", "C3", 1600, 2600,
            "tvscraper", "scraper-metadata", {"Reality"})));
        assert(repository.replaceEvidence(scraperEvidence(
            "C3\n30", "30", "C3", 1600, 2600,
            "tvscraper-media-type", "scraper-media-type", {"Series"})));
        assert(repository.reconcileEpgBrowseClassification("a", "C3\n30"));

        assert(repository.replaceEvidence(scraperEvidence(
            "C6\n60", "60", "C6", 1850, 2850,
            "tvscraper-media-type", "scraper-media-type", {"Series"})));
        assert(repository.reconcileEpgBrowseClassification("a", "C6\n60"));

        assert(repository.replaceEvidence(scraperEvidence(
            "C7\n70", "70", "C7", 1900, 2900,
            "tvscraper-media-type", "scraper-media-type", {"Series"})));
        assert(repository.reconcileEpgBrowseClassification("a", "C7\n70"));

        assert(repository.replaceEvidence(scraperEvidence(
            "C9\n90", "90", "C9", 2000, 3000,
            "tvscraper-media-type", "scraper-media-type", {"Series"}, "stale")));
        assert(repository.reconcileEpgBrowseClassification("a", "C9\n90"));

        assert(repository.replaceEvidence(scraperEvidence(
            "C10\n100", "100", "C10", 2050, 5650,
            "tvscraper", "scraper-metadata", {"Crime"}, "stale")));
        assert(repository.replaceEvidence(scraperEvidence(
            "C10\n100", "100", "C10", 2050, 5650,
            "tvscraper-media-type", "scraper-media-type", {"Series"}, "stale")));
        assert(repository.reconcileEpgBrowseClassification("a", "C10\n100"));

        assert(repository.replaceEvidence(scraperEvidence(
            "C11\n110", "110", "C11", 2100, 3900,
            "tvscraper", "scraper-metadata", {"Comedy"}, "stale")));
        assert(repository.replaceEvidence(scraperEvidence(
            "C11\n110", "110", "C11", 2100, 3900,
            "tvscraper-media-type", "scraper-media-type", {"Series"}, "stale")));
        assert(repository.reconcileEpgBrowseClassification("a", "C11\n110"));

        GenreEpgBrowseOverview browse = repository.epgBrowseOverview(
            "a", 900, 8000);
        assert(browse.categories.size() == 4);
        assert(category(browse, "movie").itemCount == 1);
        assert(category(browse, "series").itemCount == 2);
        assert(category(browse, "documentary").itemCount == 1);
        assert(category(browse, "sports").itemCount == 2);

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
            "a", "movie", "thriller", 900, 8000, 50, 0);
        assert(thriller.totalCount == 1);
        assert(thriller.events.front().eventId == "20");
        assert(thriller.events.front().artworkAvailable);

        GenreEpgPage series = repository.epgByBrowse(
            "a", "series", "", 900, 8000, 50, 0);
        assert(series.totalCount == 2);
        assert(!pageContainsTitle(series, "Hartz Rot Gold"));
        assert(pageContainsTitle(series, "Death in Paradise"));
        assert(pageContainsTitle(series, "The Big Bang Theory"));
        assert(!pageContainsTitle(series, "Tagesschau"));
        assert(!pageContainsTitle(series, "Tagesthemen"));
        assert(!pageContainsTitle(series, "Sportschau"));

        GenreEpgPage sports = repository.epgByBrowse(
            "a", "sports", "", 900, 8000, 50, 0);
        assert(sports.totalCount == 2);
        assert(pageContainsTitle(sports, "Live sport"));
        assert(pageContainsTitle(sports, "Sportschau"));

        GenreEpgPage documentary = repository.epgByBrowse(
            "a", "documentary", "", 900, 8000, 50, 0);
        assert(documentary.totalCount == 1);
        assert(documentary.events.front().eventId == "40");

        GenreEpgPage isolated = repository.epgByBrowse(
            "b", "movie", "", 900, 8000, 50, 0);
        assert(isolated.totalCount == 1);
        assert(isolated.events.front().title == "Other");
    }

    {
        Database database;
        assert(database.open(filename));
        GenreIndexRepository repository(database);
        GenreEpgPage persisted = repository.epgByBrowse(
            "a", "series", "", 900, 8000, 50, 0);
        assert(persisted.totalCount == 2);
        assert(!pageContainsTitle(persisted, "Hartz Rot Gold"));
        assert(pageContainsTitle(persisted, "Death in Paradise"));
        assert(pageContainsTitle(persisted, "The Big Bang Theory"));
        assert(!pageContainsTitle(persisted, "Tagesschau"));
        assert(!pageContainsTitle(persisted, "Tagesthemen"));
    }

    std::remove(filename.c_str());
    std::cout << "genre index repository hierarchy ok\n";
    return 0;
}

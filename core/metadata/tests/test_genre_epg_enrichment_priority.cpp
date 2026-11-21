#include "Database.h"
#include "GenreIndexRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace
{
GenreEvidenceInput evidence(
    const std::string& providerId,
    const std::string& sourceKind,
    const std::vector<std::string>& values)
{
    GenreEvidenceInput input;
    input.backendId = "a";
    input.targetType = "program-event";
    input.resourceKey = "C2\n2";
    input.nativeId = "2";
    input.channelId = "C2";
    input.startTime = 1200;
    input.endTime = 2200;
    input.providerId = providerId;
    input.sourceKind = sourceKind;
    input.originalValues = values;
    input.state = "active";
    input.confidence = 0.99;
    input.observedAt = 3000;
    return input;
}
}

int main()
{
    const std::string filename =
        "/tmp/vdr-suite-genre-epg-enrichment-priority-test.sqlite";
    std::remove(filename.c_str());

    Database database;
    assert(database.open(filename));
    assert(database.execute(
        "CREATE TABLE epg_events("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,title TEXT,"
        "subtitle TEXT,description TEXT,start_time TEXT,end_time TEXT,"
        "duration_seconds INTEGER,content_descriptors TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "INSERT INTO epg_events VALUES"
        "('a','C1','1','Earlier unmapped','','','1000','2000',1000,''),"
        "('a','C2','2','Later ETYPES movie','','','1200','2200',1000,'');"));

    GenreIndexRepository repository(database);
    assert(repository.ensureSchema());
    assert(repository.synchronizeEpgCache("a", 900, 2300));

    assert(repository.replaceEvidence(evidence(
        "tvscraper-media-type",
        "scraper-media-type",
        {"Movie"})));

    std::vector<GenreEpgRefreshCandidate> candidates =
        repository.epgRefreshCandidates(
            "a",
            900,
            2300,
            "tvscraper",
            2500,
            1);
    assert(candidates.size() == 1);
    assert(candidates.front().eventId == "2");

    assert(repository.replaceEvidence(evidence(
        "tvscraper",
        "scraper-metadata",
        {"Drama"})));

    candidates = repository.epgRefreshCandidates(
        "a",
        900,
        2300,
        "tvscraper",
        2500,
        1);
    assert(candidates.size() == 1);
    assert(candidates.front().eventId == "1");

    database.close();
    std::remove(filename.c_str());
    std::remove((filename + "-wal").c_str());
    std::remove((filename + "-shm").c_str());

    std::cout << "genre epg enrichment priority ok\n";
    return 0;
}

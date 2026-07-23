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
    std::int64_t observedAt)
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
    input.sourceKind = "test-provider";
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
    const std::string filename = "/tmp/vdr-suite-genre-conflict-test.sqlite";
    std::remove(filename.c_str());

    Database database;
    assert(database.open(filename));
    GenreIndexRepository repository(database);
    assert(repository.ensureSchema());

    assert(repository.replaceEvidence(evidence("vdr-epg", "Drama", 1000)));
    assert(repository.replaceEvidence(evidence("tvscraper", "Action", 1100)));

    GenreOverview conflicting = repository.overview(
        "default", "program-event", 900, 2100);
    assert(findGenre(conflicting, "drama").conflictCount == 1);
    assert(findGenre(conflicting, "action").conflictCount == 1);

    assert(repository.replaceEvidence(evidence("tvscraper", "Drama", 1200)));
    GenreOverview agreed = repository.overview(
        "default", "program-event", 900, 2100);
    assert(findGenre(agreed, "drama").conflictCount == 0);
    assert(findGenre(agreed, "drama").activeCount == 1);

    GenreEvidenceInput stale = evidence("tvscraper", "", 1300);
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
    std::cout << "genre conflict repository ok\n";
    return 0;
}

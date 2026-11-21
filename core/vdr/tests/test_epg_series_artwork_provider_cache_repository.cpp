#include "Database.h"
#include "EpgSeriesArtworkProviderCacheRepository.h"

#include <cassert>
#include <filesystem>

int main()
{
    const auto path = std::filesystem::temp_directory_path() /
        "vdr-suite-provider-cache-test.db";
    std::error_code error;
    std::filesystem::remove(path, error);

    Database database;
    assert(database.open(path.string()));
    EpgSeriesArtworkProviderCacheRepository repository(database);
    assert(repository.ensureSchema());

    SeriesArtworkProviderCacheKey key{"tmdb", "imdb", "tt1234567"};
    assert(!repository.find(key, 100).active(100));
    assert(repository.store(
        key,
        SeriesArtworkProviderCacheOutcome::NotFound,
        200));
    auto entry = repository.find(key, 150);
    assert(entry.active(150));
    assert(entry.outcome == SeriesArtworkProviderCacheOutcome::NotFound);
    assert(entry.expiresAt == 200);

    assert(repository.store(
        key,
        SeriesArtworkProviderCacheOutcome::TemporarilyUnavailable,
        300));
    entry = repository.find(key, 250);
    assert(entry.active(250));
    assert(entry.outcome ==
           SeriesArtworkProviderCacheOutcome::TemporarilyUnavailable);

    assert(!repository.find(key, 301).active(301));
    assert(!repository.find(key, 301).active(301));

    assert(repository.store(
        key,
        SeriesArtworkProviderCacheOutcome::NotFound,
        500));
    assert(repository.remove(key));
    assert(!repository.find(key, 400).active(400));

    SeriesArtworkProviderCacheKey invalid;
    assert(!repository.store(
        invalid,
        SeriesArtworkProviderCacheOutcome::NotFound,
        100));
    assert(!repository.store(
        key,
        SeriesArtworkProviderCacheOutcome::None,
        100));

    database.close();
    std::filesystem::remove(path, error);
    return 0;
}

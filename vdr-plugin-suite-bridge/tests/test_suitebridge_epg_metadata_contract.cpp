#include "suitebridge_epg_metadata_contract.h"

#include <cassert>
#include <string>

namespace {

SuiteBridgeArtworkReference artwork(
    const std::string &path,
    int width,
    int height)
{
  SuiteBridgeArtworkReference value;
  value.provider = SuiteBridgeArtworkProvider::TvScraper;
  value.path = path;
  value.width = width;
  value.height = height;
  return value;
}

SuiteBridgeEpgExternalId externalId(
    SuiteBridgeEpgExternalIdScope scope,
    const std::string &value)
{
  SuiteBridgeEpgExternalId externalId;
  externalId.provider = SuiteBridgeEpgExternalIdProvider::Imdb;
  externalId.scope = scope;
  externalId.value = value;
  return externalId;
}

} // namespace

int main()
{
  assert(!SuiteBridgeEpgMediaTypeIsResolved(SuiteBridgeEpgMediaType::None));
  assert(SuiteBridgeEpgMediaTypeIsResolved(SuiteBridgeEpgMediaType::Movie));
  assert(SuiteBridgeEpgMediaTypeIsResolved(SuiteBridgeEpgMediaType::Series));

  {
    SuiteBridgeEpgMetadataRequest request(
        "META",
        "S19.2E-1-1019-10301 12345");
    assert(request.Handled());
    assert(request.Valid());
    assert(request.ChannelId() == "S19.2E-1-1019-10301");
    assert(request.EventId() == 12345U);
  }

  {
    SuiteBridgeEpgMetadataRequest request("meta", " channel 42 ");
    assert(request.Handled());
    assert(request.Valid());
    assert(request.ChannelId() == "channel");
    assert(request.EventId() == 42U);
  }

  {
    SuiteBridgeEpgMetadataRequest request("META", "channel");
    assert(request.Handled());
    assert(!request.Valid());
  }

  {
    SuiteBridgeEpgMetadataRequest request("ARTW", "channel 42");
    assert(!request.Handled());
    assert(!request.Valid());
  }

  {
    SuiteBridgeEpgMetadata metadata;
    metadata.found = true;
    metadata.mediaType = SuiteBridgeEpgMediaType::Series;
    metadata.providerId = 123;
    metadata.seasonNumber = 4;
    metadata.episodeNumber = 9;
    metadata.absoluteEpisodeNumber = 52;
    metadata.runtimeMinutes = 48;
    metadata.durationDeviationMinutes = 2;
    metadata.scraperHd = 1;
    metadata.scraperLanguage = 2;
    metadata.popularity = 12.5F;
    metadata.voteAverage = 8.4F;
    metadata.voteCount = 77;
    metadata.collectionId = 55;
    metadata.lastSeason = 7;
    metadata.title = "Serie \"Nord\"";
    metadata.originalTitle = "Northern Series";
    metadata.episodeName = "Die Folge";
    metadata.tagline = "Eine Zeile\nmit Umbruch";
    metadata.overview = "Ausführliche Beschreibung";
    metadata.releaseDate = "2026-07-20";
    metadata.firstAired = "2026-07-19";
    // Preserve the legacy episode-level value while transmitting qualified IDs.
    metadata.imdbId = "tt7654321";
    metadata.externalIds.push_back(externalId(
        SuiteBridgeEpgExternalIdScope::Series,
        "tt1234567"));
    metadata.externalIds.push_back(externalId(
        SuiteBridgeEpgExternalIdScope::Episode,
        "tt7654321"));
    metadata.status = "Returning Series";
    metadata.collectionName = "Nord Collection";
    metadata.genres = {"Drama", "Mystery"};
    metadata.productionCountries = {"Deutschland"};
    metadata.networks = {"ZDF"};
    metadata.preferredArtwork = artwork(
        "/var/cache/tvscraper/preferred.jpg",
        1280,
        720);

    SuiteBridgeEpgPerson person;
    person.role = SuiteBridgeEpgPersonRole::Actor;
    person.name = "Erika Mustermann";
    person.characterName = "Kommissarin Nord";
    person.image = artwork(
        "/var/cache/tvscraper/person.jpg",
        300,
        450);
    metadata.people.push_back(person);

    SuiteBridgeEpgImage image;
    image.orientation = SuiteBridgeEpgImageOrientation::Portrait;
    image.artwork = artwork(
        "/var/cache/tvscraper/poster.jpg",
        600,
        900);
    metadata.images.push_back(image);

    SuiteBridgeEpgMetadataPayload payload(metadata);
    assert(payload.Complete());
    const std::string json(payload.Data(), payload.Size());

    assert(json.find("\"schema\":1") != std::string::npos);
    assert(json.find("\"found\":true") != std::string::npos);
    assert(json.find("\"provider\":\"tvscraper\"") != std::string::npos);
    assert(json.find("\"mediaType\":\"series\"") != std::string::npos);
    assert(json.find("\"providerId\":123") != std::string::npos);
    assert(json.find("\"seasonNumber\":4") != std::string::npos);
    assert(json.find("\"episodeNumber\":9") != std::string::npos);
    assert(json.find("\"scraperHd\":1") != std::string::npos);
    assert(json.find("\"scraperLanguage\":2") != std::string::npos);
    assert(json.find("Serie \\\"Nord\\\"") != std::string::npos);
    assert(json.find("Eine Zeile\\nmit Umbruch") != std::string::npos);
    assert(json.find("\"imdbId\":\"tt7654321\"") != std::string::npos);
    assert(json.find(
        "{\"provider\":\"imdb\",\"scope\":\"series\",\"value\":\"tt1234567\"}") != std::string::npos);
    assert(json.find(
        "{\"provider\":\"imdb\",\"scope\":\"episode\",\"value\":\"tt7654321\"}") != std::string::npos);
    assert(json.find(
        "{\"provider\":\"tmdb\",\"scope\":\"series\",\"value\":\"123\"}") != std::string::npos);
    assert(json.find("\"genres\":[\"Drama\",\"Mystery\"]") != std::string::npos);
    assert(json.find("\"role\":\"actor\"") != std::string::npos);
    assert(json.find("Kommissarin Nord") != std::string::npos);
    assert(json.find("\"orientation\":\"portrait\"") != std::string::npos);
    assert(json.find("\"origin\":\"primary-metadata\"") != std::string::npos);
    assert(json.find("preferred.jpg") != std::string::npos);
    assert(json.find("poster.jpg") != std::string::npos);
  }

  {
    // TVScraper documents negative getDbId() values as TheTVDB IDs.
    // Preserve that provider identity explicitly while keeping the schema-1
    // unqualified providerId non-negative for existing daemon parsers.
    SuiteBridgeEpgMetadata metadata;
    metadata.found = true;
    metadata.mediaType = SuiteBridgeEpgMediaType::Series;
    metadata.providerId = -80379;
    metadata.title = "The Big Bang Theory";

    SuiteBridgeEpgMetadataPayload payload(metadata);
    assert(payload.Complete());
    const std::string json(payload.Data(), payload.Size());

    assert(json.find("\"providerId\":0") != std::string::npos);
    assert(json.find(
        "{\"provider\":\"tvdb\",\"scope\":\"series\",\"value\":\"80379\"}") != std::string::npos);
    assert(json.find("\"value\":\"-80379\"") == std::string::npos);
  }

  {
    SuiteBridgeEpgMetadataPayload payload(SuiteBridgeEpgMetadata{});
    assert(payload.Complete());
    const std::string json(payload.Data(), payload.Size());
    assert(json.find("\"found\":false") != std::string::npos);
    assert(json.find("\"provider\":\"none\"") != std::string::npos);
    assert(json.find("\"mediaType\":\"none\"") != std::string::npos);
    assert(json.find("\"externalIds\":[]") != std::string::npos);
    assert(json.find("\"origin\":\"none\"") != std::string::npos);
  }

  {
    SuiteBridgeEpgMetadata inconsistent;
    inconsistent.found = true;
    inconsistent.mediaType = SuiteBridgeEpgMediaType::None;
    inconsistent.providerId = 999;
    inconsistent.title = "Sentinel ohne aufgelösten Typ";
    inconsistent.genres = {"Drama"};

    SuiteBridgeEpgMetadataPayload payload(inconsistent);
    assert(payload.Complete());
    const std::string json(payload.Data(), payload.Size());
    assert(json.find("\"found\":false") != std::string::npos);
    assert(json.find("\"provider\":\"none\"") != std::string::npos);
    assert(json.find("\"mediaType\":\"none\"") != std::string::npos);
    assert(json.find("\"providerId\":0") != std::string::npos);
    assert(json.find("\"genres\":[]") != std::string::npos);
    assert(json.find("Sentinel") == std::string::npos);
    assert(json.find("Drama") == std::string::npos);
  }

  {
    SuiteBridgeEpgMetadata invalidIdentity;
    invalidIdentity.found = true;
    invalidIdentity.mediaType = SuiteBridgeEpgMediaType::Series;
    invalidIdentity.externalIds.push_back(externalId(
        SuiteBridgeEpgExternalIdScope::Series,
        "tt1234567"));
    invalidIdentity.externalIds.push_back(invalidIdentity.externalIds.front());

    SuiteBridgeEpgMetadataPayload payload(invalidIdentity);
    assert(payload.Complete());
    const std::string json(payload.Data(), payload.Size());
    assert(json.find("\"found\":false") != std::string::npos);
    assert(json.find("tt1234567") == std::string::npos);
  }

  {
    SuiteBridgeEpgMetadata metadata;
    metadata.found = true;
    metadata.mediaType = SuiteBridgeEpgMediaType::Movie;
    metadata.overview.assign(9000, 'x');
    SuiteBridgeEpgMetadataPayload payload(metadata);
    assert(!payload.Complete());
  }

  return 0;
}

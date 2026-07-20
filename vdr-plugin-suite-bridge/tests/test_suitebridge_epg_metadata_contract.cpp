#include "suitebridge_epg_metadata_contract.h"

#include <cassert>
#include <string>

int main()
{
  {
    SuiteBridgeEpgMetadataRequest request(
        "EPMD",
        "S19.2E-1-1019-10301 12345");
    assert(request.Handled());
    assert(request.Valid());
    assert(request.ChannelId() == "S19.2E-1-1019-10301");
    assert(request.EventId() == 12345U);
  }

  {
    SuiteBridgeEpgMetadataRequest request("epmd", " channel 42 ");
    assert(request.Handled());
    assert(request.Valid());
    assert(request.ChannelId() == "channel");
    assert(request.EventId() == 42U);
  }

  {
    SuiteBridgeEpgMetadataRequest request("EPMD", "channel");
    assert(request.Handled());
    assert(!request.Valid());
  }

  {
    SuiteBridgeEpgMetadataRequest request("ARTW", "channel 42");
    assert(!request.Handled());
    assert(!request.Valid());
  }

  SuiteBridgeEpgMetadata metadata;
  metadata.found = true;
  metadata.mediaType = SuiteBridgeEpgMediaType::Series;
  metadata.databaseId = 815;
  metadata.title = "Bares für Rares";
  metadata.originalTitle = "Bares \"für\" Rares";
  metadata.episodeTitle = "Die Trödel-Show";
  metadata.tagline = "Wertvoll oder wertlos?";
  metadata.overview = "Eine ausführliche\nBeschreibung";
  metadata.episodeOverview = "Die heutige Folge";
  metadata.releaseDate = "2025-02-24";
  metadata.firstAired = "2025-02-24";
  metadata.imdbId = "tt1234567";
  metadata.collectionId = 77;
  metadata.collectionName = "Bares Collection";
  metadata.status = "Continuing";
  metadata.runtimeMinutes = 50;
  metadata.seasonNumber = 3;
  metadata.episodeNumber = 14;
  metadata.absoluteEpisodeNumber = 259;
  metadata.lastSeason = 8;
  metadata.voteAverage = 7.25F;
  metadata.voteCount = 144;
  metadata.genres = {"Show", "Unterhaltung"};
  metadata.productionCountries = {"Deutschland"};
  metadata.networks = {"ZDF"};

  SuiteBridgeEpgPerson person;
  person.role = SuiteBridgeEpgPersonRole::Moderator;
  person.name = "Horst Lichter";
  person.characterName = "Moderator";
  person.image.orientation = SuiteBridgeEpgImageOrientation::Portrait;
  person.image.path = "/var/cache/tvscraper/people/horst.jpg";
  person.image.width = 400;
  person.image.height = 600;
  metadata.persons.push_back(person);

  SuiteBridgeEpgImage landscape;
  landscape.orientation = SuiteBridgeEpgImageOrientation::Landscape;
  landscape.path = "/var/cache/tvscraper/shows/815_fanart.jpg";
  landscape.width = 1280;
  landscape.height = 720;
  metadata.images.push_back(landscape);

  {
    SuiteBridgeEpgMetadataPayload payload(metadata);
    assert(payload.Complete());
    const std::string json(payload.Data(), payload.Size());
    assert(json.find("\"schema\":1") != std::string::npos);
    assert(json.find("\"found\":true") != std::string::npos);
    assert(json.find("\"provider\":\"tvscraper\"") != std::string::npos);
    assert(json.find("\"mediaType\":\"series\"") != std::string::npos);
    assert(json.find("\"databaseId\":815") != std::string::npos);
    assert(json.find("Bares \\\"für\\\" Rares") != std::string::npos);
    assert(json.find("Eine ausführliche\\nBeschreibung") != std::string::npos);
    assert(json.find("\"seasonNumber\":3") != std::string::npos);
    assert(json.find("\"episodeNumber\":14") != std::string::npos);
    assert(json.find("\"absoluteEpisodeNumber\":259") != std::string::npos);
    assert(json.find("\"genres\":[\"Show\",\"Unterhaltung\"]") != std::string::npos);
    assert(json.find("\"role\":\"moderator\"") != std::string::npos);
    assert(json.find("\"name\":\"Horst Lichter\"") != std::string::npos);
    assert(json.find("\"characterName\":\"Moderator\"") != std::string::npos);
    assert(json.find("\"orientation\":\"portrait\"") != std::string::npos);
    assert(json.find("\"orientation\":\"landscape\"") != std::string::npos);
    assert(json.find("815_fanart.jpg") != std::string::npos);
  }

  {
    SuiteBridgeEpgMetadataPayload payload(SuiteBridgeEpgMetadata{});
    assert(payload.Complete());
    const std::string json(payload.Data(), payload.Size());
    assert(json ==
        "{\"schema\":1,\"found\":false,\"provider\":\"none\"}");
  }

  {
    SuiteBridgeEpgMetadata oversized = metadata;
    oversized.overview.assign(40000, 'x');
    SuiteBridgeEpgMetadataPayload payload(oversized);
    assert(!payload.Complete());
    assert(payload.Size() == 32767U);
  }

  return 0;
}

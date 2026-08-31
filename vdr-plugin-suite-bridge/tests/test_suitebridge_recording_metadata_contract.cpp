#include "suitebridge_recording_metadata_contract.h"
#include "suitebridge_recording_identity.h"

#include <cassert>
#include <string>

namespace {

SuiteBridgeRecordingPerson actor(
    const std::string &name,
    const std::string &characterName)
{
  SuiteBridgeRecordingPerson person;
  person.role = SuiteBridgeRecordingPersonRole::Actor;
  person.name = name;
  person.characterName = characterName;
  return person;
}

}

int main()
{
  const std::string key =
      SuiteBridgeRecordingIdentity::KeyForNativeId(
          "/srv/vdr/video/Forrest_Gump/2026-07-20.20.15.1-0.rec");

  SuiteBridgeRecordingMetadataRequest valid("RMETA", key.c_str());
  assert(valid.Handled());
  assert(valid.Valid());
  assert(valid.RecordingKey() == key);

  SuiteBridgeRecordingMetadataRequest spaced(
      "rmeta", ("  " + key + "  ").c_str());
  assert(spaced.Handled());
  assert(spaced.Valid());

  SuiteBridgeRecordingMetadataRequest extra(
      "RMETA", (key + " extra").c_str());
  assert(extra.Handled());
  assert(!extra.Valid());

  SuiteBridgeRecordingMetadataRequest percent("RMETA", "%2fetc%2fpasswd");
  assert(percent.Handled());
  assert(!percent.Valid());

  SuiteBridgeRecordingMetadataRequest other("META", key.c_str());
  assert(!other.Handled());
  assert(!other.Valid());

  SuiteBridgeRecordingMetadata missing;
  missing.recordingKey = key;
  missing.reason = SuiteBridgeRecordingMetadataReason::RecordingNotFound;
  SuiteBridgeRecordingMetadataPayload missingPayload(missing);
  assert(missingPayload.Complete());
  const std::string missingJson(missingPayload.Data(), missingPayload.Size());
  assert(missingJson.rfind("{\"schema\":1", 0) == 0);
  assert(missingJson.find("\"found\":false") != std::string::npos);
  assert(missingJson.find("\"reason\":\"recording-not-found\"") != std::string::npos);
  assert(missingJson.find("\"recordingKey\":\"" + key + "\"") != std::string::npos);
  assert(missingJson.back() == '}');

  SuiteBridgeRecordingMetadata metadata;
  metadata.found = true;
  metadata.recordingKey = key;
  metadata.mediaType = SuiteBridgeRecordingMediaType::Movie;
  metadata.providerId = 13;
  metadata.title = "Forrest Gump";
  metadata.overview = "A \"quoted\" overview";
  metadata.imdbId = "tt0109830";
  metadata.genres = {"Drama", "Comedy"};
  metadata.preferredArtwork.provider = SuiteBridgeArtworkProvider::TvScraper;
  metadata.preferredArtwork.path = "movies/13/poster.jpg";
  metadata.preferredArtwork.width = 780;
  metadata.preferredArtwork.height = 1170;

  const SuiteBridgeRecordingPerson person =
      actor("Tom Hanks", "Forrest Gump");
  metadata.people.push_back(person);

  SuiteBridgeRecordingMetadataPayload payload(metadata);
  assert(payload.Complete());
  const std::string json(payload.Data(), payload.Size());
  assert(json.find("{\"schema\":1,\"found\":true,\"reason\":\"none\",\"provider\":\"tvscraper\"") == 0);
  assert(json.find("\"title\":\"Forrest Gump\"") != std::string::npos);
  assert(json.find("\"name\":\"Tom Hanks\"") != std::string::npos);
  assert(json.find("/srv/vdr/video") == std::string::npos);
  assert(json.back() == '}');

  SuiteBridgeRecordingMetadata series = metadata;
  series.mediaType = SuiteBridgeRecordingMediaType::Series;
  series.providerId = -1399;
  series.seasonNumber = 10;
  series.episodeNumber = 14;
  series.title = "The Walking Dead";
  series.people.clear();
  SuiteBridgeRecordingMetadataPayload seriesPayload(series);
  assert(seriesPayload.Complete());
  const std::string seriesJson(seriesPayload.Data(), seriesPayload.Size());
  assert(seriesJson.find("\"mediaType\":\"series\"") != std::string::npos);
  assert(seriesJson.find("\"providerId\":-1399") != std::string::npos);
  assert(seriesJson.find("\"seasonNumber\":10") != std::string::npos);
  assert(seriesJson.find("\"episodeNumber\":14") != std::string::npos);

  SuiteBridgeRecordingMetadata zeroIdentity = series;
  zeroIdentity.providerId = 0;
  SuiteBridgeRecordingMetadataPayload zeroIdentityPayload(zeroIdentity);
  assert(!zeroIdentityPayload.Complete());
  assert(zeroIdentityPayload.Size() == 0);

  SuiteBridgeRecordingMetadata pulpFiction = metadata;
  pulpFiction.providerId = 680;
  pulpFiction.title = "Pulp Fiction";
  pulpFiction.people.clear();
  for (int index = 0; index < 52; ++index) {
    pulpFiction.people.push_back(actor(
        "Supporting Actor " + std::to_string(index),
        "Supporting Character " + std::to_string(index)));
  }
  pulpFiction.people[40] = actor("John Travolta", "Vincent Vega");

  SuiteBridgeRecordingMetadataPayload pulpFictionPayload(pulpFiction);
  assert(pulpFictionPayload.Complete());
  assert(pulpFictionPayload.Size() > 7680);
  const std::string pulpFictionJson(
      pulpFictionPayload.Data(),
      pulpFictionPayload.Size());
  assert(pulpFictionJson.find("\"name\":\"John Travolta\"") !=
      std::string::npos);
  assert(pulpFictionJson.find("\"characterName\":\"Vincent Vega\"") !=
      std::string::npos);

  SuiteBridgeRecordingMetadata tooManyPeople = metadata;
  tooManyPeople.people.assign(
      SuiteBridgeRecordingMetadata::kMaxPeople + 1, person);
  SuiteBridgeRecordingMetadataPayload tooManyPeoplePayload(tooManyPeople);
  assert(!tooManyPeoplePayload.Complete());
  assert(tooManyPeoplePayload.Size() == 0);

  SuiteBridgeRecordingMetadata oversized = metadata;
  oversized.overview.assign(
      SuiteBridgeRecordingMetadata::kMaximumPayloadBytes + 1024, 'x');
  SuiteBridgeRecordingMetadataPayload oversizedPayload(oversized);
  assert(!oversizedPayload.Complete());
  assert(
      oversizedPayload.Size() ==
      SuiteBridgeRecordingMetadata::kMaximumPayloadBytes);

  return 0;
}

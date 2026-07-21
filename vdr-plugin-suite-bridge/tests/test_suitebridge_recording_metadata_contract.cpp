#include "suitebridge_recording_metadata_contract.h"
#include "suitebridge_recording_identity.h"

#include <cassert>
#include <string>

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

  SuiteBridgeRecordingPerson person;
  person.role = SuiteBridgeRecordingPersonRole::Actor;
  person.name = "Tom Hanks";
  person.characterName = "Forrest Gump";
  metadata.people.push_back(person);

  SuiteBridgeRecordingMetadataPayload payload(metadata);
  assert(payload.Complete());
  const std::string json(payload.Data(), payload.Size());
  assert(json.find("{\"schema\":1,\"found\":true,\"reason\":\"none\",\"provider\":\"tvscraper\"") == 0);
  assert(json.find("\"title\":\"Forrest Gump\"") != std::string::npos);
  assert(json.find("\"name\":\"Tom Hanks\"") != std::string::npos);
  assert(json.find("/srv/vdr/video") == std::string::npos);
  assert(json.back() == '}');

  SuiteBridgeRecordingMetadata tooManyPeople = metadata;
  tooManyPeople.people.assign(
      SuiteBridgeRecordingMetadata::kMaxPeople + 1, person);
  SuiteBridgeRecordingMetadataPayload tooManyPeoplePayload(tooManyPeople);
  assert(!tooManyPeoplePayload.Complete());
  assert(tooManyPeoplePayload.Size() == 0);

  SuiteBridgeRecordingMetadata oversized = metadata;
  oversized.overview.assign(9000, 'x');
  SuiteBridgeRecordingMetadataPayload oversizedPayload(oversized);
  assert(!oversizedPayload.Complete());
  assert(oversizedPayload.Size() == 7679);

  return 0;
}

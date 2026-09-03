#include "VdrRecordingNativeMetadataPublicJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    VdrRecordingNativeMetadataRecord record;
    record.backendId = "default";
    record.backendNativeId =
        "/srv/vdr/video/Filme/Inferno/2026-07-20.20.15.1-0.rec";
    record.recordingKey = "recording-key";
    record.contentState = "found";

    record.metadata.found = true;
    record.metadata.provider = "tvscraper";
    record.metadata.mediaType = "movie";
    record.metadata.providerId = 207932;
    record.metadata.title = "Inferno";
    record.metadata.overview =
        "Robert Langdon folgt einer gefährlichen Spur.";
    record.metadata.releaseDate = "2016-10-13";
    record.metadata.imdbId = "tt3062096";
    record.metadata.voteAverage = 6.1;
    record.metadata.voteCount = 6400;
    record.metadata.genres = {"Thriller", "Mystery"};

    record.metadata.preferredArtwork.available = true;
    record.metadata.preferredArtwork.provider = "tvscraper";
    record.metadata.preferredArtwork.path =
        "/var/cache/vdr/plugins/tvscraper/movies/207932/poster.jpg";
    record.metadata.preferredArtwork.width = 1000;
    record.metadata.preferredArtwork.height = 1500;

    VdrRecordingNativePerson person;
    person.role = "actor";
    person.name = "Tom Hanks";
    person.characterName = "Robert Langdon";
    person.image.available = true;
    person.image.provider = "tvscraper";
    person.image.path =
        "/var/cache/vdr/plugins/tvscraper/actors/tom-hanks.jpg";
    record.metadata.people.push_back(person);

    VdrRecordingNativeArtwork image;
    image.available = true;
    image.provider = "tvscraper";
    image.path =
        "/var/cache/vdr/plugins/tvscraper/movies/207932/fanart.jpg";
    image.orientation = "landscape";
    record.metadata.images.push_back(image);

    const std::string json =
        VdrRecordingNativeMetadataPublicJsonSerializer().serialize(record);

    assert(json.find("\"available\":true") != std::string::npos);
    assert(json.find("\"provider\":\"tvscraper\"") != std::string::npos);
    assert(json.find("\"title\":\"Inferno\"") != std::string::npos);
    assert(json.find("\"name\":\"Tom Hanks\"") != std::string::npos);
    assert(json.find("\"characterName\":\"Robert Langdon\"") !=
        std::string::npos);
    assert(json.find(
        "/api/vdr/recordings/metadata/image?backend=default") !=
        std::string::npos);
    assert(json.find(
        "%2Fsrv%2Fvdr%2Fvideo%2FFilme%2FInferno") !=
        std::string::npos);
    assert(json.find(
        "/var/cache/vdr/plugins/tvscraper") ==
        std::string::npos);

    const VdrRecordingNativeMetadataRecord missing;
    assert(
        VdrRecordingNativeMetadataPublicJsonSerializer()
            .serialize(missing) ==
        "{\"available\":false,\"status\":\"not-found\",\"settled\":false}");

    VdrRecordingNativeMetadataRecord negative;
    negative.backendId = "default";
    negative.backendNativeId = "/srv/vdr/video/Series/negative.rec";
    negative.recordingKey = "negative-recording-key";
    negative.contentState = "not_found";
    negative.lastAttemptState = "not_found";
    negative.metadata.availability =
        VdrRecordingNativeMetadataAvailability::NotFound;
    assert(
        VdrRecordingNativeMetadataPublicJsonSerializer()
            .serialize(negative) ==
        "{\"available\":false,\"status\":\"not-found\",\"settled\":true}");

    VdrRecordingNativeMetadataRecord retryable;
    retryable.backendId = "default";
    retryable.backendNativeId = "/srv/vdr/video/Series/retryable.rec";
    retryable.recordingKey = "retryable-recording-key";
    retryable.contentState = "empty";
    retryable.lastAttemptState = "transport_error";
    retryable.metadata.availability =
        VdrRecordingNativeMetadataAvailability::TransportError;
    assert(
        VdrRecordingNativeMetadataPublicJsonSerializer()
            .serialize(retryable) ==
        "{\"available\":false,\"status\":\"not-found\",\"settled\":false}");

    std::cout
        << "test_vdr_recording_native_metadata_public_json_serializer passed"
        << std::endl;

    return 0;
}

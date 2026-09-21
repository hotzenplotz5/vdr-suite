#include "SuiteBridgeRecordingMetadataResolver.h"

#include <cassert>
#include <string>

namespace
{

class MockTransport final : public ISuiteBridgeRecordingMetadataTransport
{
public:
    SuiteBridgeRecordingMetadataCommandReply reply;
    std::string requestedKey;

    SuiteBridgeRecordingMetadataCommandReply requestRecordingMetadata(
        const std::string& recordingKey) override
    {
        requestedKey = recordingKey;
        return reply;
    }
};

std::string artwork(
    bool available,
    const std::string& path = {},
    int width = 0,
    int height = 0)
{
    return std::string("{\"available\":") +
        (available ? "true" : "false") +
        ",\"provider\":\"" +
        (available ? "tvscraper" : "none") +
        "\",\"path\":\"" + path +
        "\",\"width\":" + std::to_string(width) +
        ",\"height\":" + std::to_string(height) + "}";
}

std::string foundPayload(const std::string& key)
{
    return std::string("{\"schema\":1,\"found\":true") +
        ",\"reason\":\"none\"" +
        ",\"provider\":\"tvscraper\"" +
        ",\"recordingIdentitySchema\":1" +
        ",\"recordingKey\":\"" + key + "\"" +
        ",\"mediaType\":\"movie\"" +
        ",\"providerId\":13" +
        ",\"seasonNumber\":0" +
        ",\"episodeNumber\":0" +
        ",\"absoluteEpisodeNumber\":0" +
        ",\"runtimeMinutes\":142" +
        ",\"durationDeviationMinutes\":1" +
        ",\"scraperHd\":1" +
        ",\"scraperLanguage\":0" +
        ",\"popularity\":99.5" +
        ",\"voteAverage\":8.8" +
        ",\"voteCount\":1234" +
        ",\"adult\":false" +
        ",\"collectionId\":0" +
        ",\"lastSeason\":0" +
        ",\"title\":\"Forrest Gump\"" +
        ",\"originalTitle\":\"Forrest Gump\"" +
        ",\"episodeName\":\"\"" +
        ",\"tagline\":\"Life is like a box of chocolates\"" +
        ",\"overview\":\"A quoted overview\"" +
        ",\"releaseDate\":\"1994-07-06\"" +
        ",\"firstAired\":\"\"" +
        ",\"imdbId\":\"tt0109830\"" +
        ",\"status\":\"Released\"" +
        ",\"collectionName\":\"\"" +
        ",\"genres\":[\"Drama\",\"Comedy\"]" +
        ",\"productionCountries\":[\"US\"]" +
        ",\"networks\":[]" +
        ",\"preferredArtwork\":" +
            artwork(true, "movies/13/poster.jpg", 780, 1170) +
        ",\"people\":[{\"role\":\"actor\",\"name\":\"Tom Hanks\"" +
            ",\"characterName\":\"Forrest Gump\",\"image\":" +
            artwork(true, "actors/tom-hanks.jpg", 300, 450) + "}]" +
        ",\"images\":[{\"orientation\":\"landscape\",\"artwork\":" +
            artwork(true, "movies/13/backdrop.jpg", 1280, 720) + "}]}";
}

std::string notFoundPayload(
    const std::string& key,
    const std::string& reason)
{
    return std::string("{\"schema\":1,\"found\":false") +
        ",\"reason\":\"" + reason + "\"" +
        ",\"provider\":\"none\"" +
        ",\"recordingIdentitySchema\":1" +
        ",\"recordingKey\":\"" + key + "\"" +
        ",\"mediaType\":\"none\"" +
        ",\"providerId\":0" +
        ",\"seasonNumber\":0" +
        ",\"episodeNumber\":0" +
        ",\"absoluteEpisodeNumber\":0" +
        ",\"runtimeMinutes\":0" +
        ",\"durationDeviationMinutes\":0" +
        ",\"scraperHd\":0" +
        ",\"scraperLanguage\":0" +
        ",\"popularity\":0" +
        ",\"voteAverage\":0" +
        ",\"voteCount\":0" +
        ",\"adult\":false" +
        ",\"collectionId\":0" +
        ",\"lastSeason\":0" +
        ",\"title\":\"\"" +
        ",\"originalTitle\":\"\"" +
        ",\"episodeName\":\"\"" +
        ",\"tagline\":\"\"" +
        ",\"overview\":\"\"" +
        ",\"releaseDate\":\"\"" +
        ",\"firstAired\":\"\"" +
        ",\"imdbId\":\"\"" +
        ",\"status\":\"\"" +
        ",\"collectionName\":\"\"" +
        ",\"genres\":[]" +
        ",\"productionCountries\":[]" +
        ",\"networks\":[]" +
        ",\"preferredArtwork\":" + artwork(false) +
        ",\"people\":[]" +
        ",\"images\":[]}";
}

}

int main()
{
    const std::string key = "c94d0eb9958a85079f81f059a436003c";

    MockTransport transport;
    transport.reply.transportSucceeded = true;
    transport.reply.replyCode = 250;
    transport.reply.payload = foundPayload(key);

    SuiteBridgeRecordingMetadataResolver resolver(transport);
    const VdrRecordingNativeMetadata found = resolver.resolve(key);
    assert(transport.requestedKey == key);
    assert(found.availability ==
        VdrRecordingNativeMetadataAvailability::Found);
    assert(found.found);
    assert(found.provider == "tvscraper");
    assert(found.mediaType == "movie");
    assert(found.providerId == 13);
    assert(found.title == "Forrest Gump");
    assert(found.people.size() == 1);
    assert(found.people[0].name == "Tom Hanks");
    assert(found.people[0].characterName == "Forrest Gump");
    assert(found.people[0].image.available);
    assert(found.preferredArtwork.path ==
        "movies/13/poster.jpg");
    assert(found.images.size() == 1);
    assert(found.images[0].orientation == "landscape");

    transport.reply.payload = foundPayload(key);
    const std::string movieType = "\"mediaType\":\"movie\"";
    const std::string seriesType = "\"mediaType\":\"series\"";
    const std::string positiveProviderId = "\"providerId\":13";
    const std::string negativeProviderId = "\"providerId\":-1399";
    transport.reply.payload.replace(
        transport.reply.payload.find(movieType),
        movieType.size(),
        seriesType);
    transport.reply.payload.replace(
        transport.reply.payload.find(positiveProviderId),
        positiveProviderId.size(),
        negativeProviderId);
    const VdrRecordingNativeMetadata series = resolver.resolve(key);
    assert(series.availability ==
        VdrRecordingNativeMetadataAvailability::Found);
    assert(series.found);
    assert(series.mediaType == "series");
    assert(series.providerId == -1399);

    transport.reply.payload = foundPayload(key);
    transport.reply.payload.replace(
        transport.reply.payload.find(positiveProviderId),
        positiveProviderId.size(),
        "\"providerId\":0");
    const VdrRecordingNativeMetadata zeroIdentity = resolver.resolve(key);
    assert(zeroIdentity.availability ==
        VdrRecordingNativeMetadataAvailability::InvalidPayload);

    transport.reply.payload =
        notFoundPayload(key, "provider-no-match");
    const VdrRecordingNativeMetadata negative =
        resolver.resolve(key);
    assert(negative.availability ==
        VdrRecordingNativeMetadataAvailability::NotFound);
    assert(!negative.found);
    assert(negative.reason == "provider-no-match");

    transport.reply.payload = foundPayload(
        "901448858ce5ea8d9e990ca5227c3a6a");
    const VdrRecordingNativeMetadata mismatched =
        resolver.resolve(key);
    assert(mismatched.availability ==
        VdrRecordingNativeMetadataAvailability::InvalidPayload);

    transport.reply.payload = foundPayload(key);
    const std::string marker = "\"schema\":1";
    transport.reply.payload.replace(
        transport.reply.payload.find(marker),
        marker.size(),
        "\"schema\":2");
    const VdrRecordingNativeMetadata wrongSchema =
        resolver.resolve(key);
    assert(wrongSchema.availability ==
        VdrRecordingNativeMetadataAvailability::InvalidPayload);

    transport.reply.payload = foundPayload(key);
    transport.reply.payload.insert(
        transport.reply.payload.find(",\"found\""),
        ",\"schema\":1");
    const VdrRecordingNativeMetadata duplicate =
        resolver.resolve(key);
    assert(duplicate.availability ==
        VdrRecordingNativeMetadataAvailability::InvalidPayload);

    transport.reply.transportSucceeded = false;
    transport.reply.replyCode = 451;
    transport.reply.payload =
        "Recording metadata provider unavailable";
    const VdrRecordingNativeMetadata unavailable =
        resolver.resolve(key);
    assert(unavailable.availability ==
        VdrRecordingNativeMetadataAvailability::ProviderUnavailable);

    transport.reply.replyCode = 500;
    const VdrRecordingNativeMetadata transportError =
        resolver.resolve(key);
    assert(transportError.availability ==
        VdrRecordingNativeMetadataAvailability::TransportError);

    const VdrRecordingNativeMetadata invalidKey =
        resolver.resolve("/srv/vdr/video/test.rec");
    assert(invalidKey.availability ==
        VdrRecordingNativeMetadataAvailability::InvalidPayload);

    return 0;
}

#include "SuiteBridgeEpgMetadataResolver.h"

#include <cassert>
#include <string>
#include <utility>

namespace
{

class MockTransport final : public ISuiteBridgeMetadataTransport
{
public:
    SuiteBridgeMetadataCommandReply requestMetadata(
        const std::string& channelId,
        const std::string& eventId) override
    {
        requestedChannelId = channelId;
        requestedEventId = eventId;
        return reply;
    }

    SuiteBridgeMetadataCommandReply reply;
    std::string requestedChannelId;
    std::string requestedEventId;
};

VdrEvent event()
{
    VdrEvent value;
    value.id = "12345";
    value.channelId = "S19.2E-1-1011-11100";
    value.title = "Testserie";
    return value;
}

std::string completePayload()
{
    return
        "{"
        "\"schema\":1,"
        "\"found\":true,"
        "\"provider\":\"tvscraper\","
        "\"mediaType\":\"series\","
        "\"providerId\":77,"
        "\"seasonNumber\":3,"
        "\"episodeNumber\":12,"
        "\"absoluteEpisodeNumber\":44,"
        "\"runtimeMinutes\":48,"
        "\"durationDeviationMinutes\":2,"
        "\"scraperHd\":1,"
        "\"scraperLanguage\":2,"
        "\"popularity\":12.5,"
        "\"voteAverage\":8.4,"
        "\"voteCount\":123,"
        "\"adult\":false,"
        "\"collectionId\":9,"
        "\"lastSeason\":5,"
        "\"title\":\"Testserie\","
        "\"originalTitle\":\"Test Series\","
        "\"episodeName\":\"Die Folge\","
        "\"tagline\":\"Eine Zeile\\nmit Umbruch\","
        "\"overview\":\"Beschreibung mit \\\"Zitat\\\"\","
        "\"releaseDate\":\"2026-07-20\","
        "\"firstAired\":\"2026-07-19\","
        "\"imdbId\":\"tt7654321\","
        "\"externalIds\":["
            "{\"provider\":\"imdb\",\"scope\":\"series\",\"value\":\"tt1234567\"},"
            "{\"provider\":\"imdb\",\"scope\":\"episode\",\"value\":\"tt7654321\"}"
        "],"
        "\"status\":\"Returning Series\","
        "\"collectionName\":\"Test Collection\","
        "\"genres\":[\"Drama\",\"Mystery\"],"
        "\"productionCountries\":[\"Deutschland\"],"
        "\"networks\":[\"ZDF\"],"
        "\"preferredArtwork\":{"
            "\"available\":true,"
            "\"provider\":\"tvscraper\","
            "\"origin\":\"primary-metadata\","
            "\"path\":\"/cache/preferred.jpg\","
            "\"width\":1280,"
            "\"height\":720"
        "},"
        "\"people\":[{"
            "\"role\":\"actor\","
            "\"name\":\"Erika Mustermann\","
            "\"characterName\":\"Kommissarin Nord\","
            "\"image\":{"
                "\"available\":true,"
                "\"provider\":\"tvscraper\","
                "\"origin\":\"primary-metadata\","
                "\"path\":\"/cache/person.jpg\","
                "\"width\":300,"
                "\"height\":450"
            "}"
        "}],"
        "\"images\":[{"
            "\"orientation\":\"portrait\","
            "\"artwork\":{"
                "\"available\":true,"
                "\"provider\":\"tvscraper\","
                "\"origin\":\"primary-metadata\","
                "\"path\":\"/cache/poster.jpg\","
                "\"width\":600,"
                "\"height\":900"
            "}"
        "}]"
        "}";
}

std::string replaceAll(
    std::string value,
    const std::string& needle,
    const std::string& replacement)
{
    std::size_t position = 0;
    while ((position = value.find(needle, position)) != std::string::npos)
    {
        value.replace(position, needle.size(), replacement);
        position += replacement.size();
    }
    return value;
}

std::string legacyPayload()
{
    std::string payload = completePayload();
    payload = replaceAll(
        std::move(payload),
        "\"externalIds\":[{\"provider\":\"imdb\",\"scope\":\"series\",\"value\":\"tt1234567\"},{\"provider\":\"imdb\",\"scope\":\"episode\",\"value\":\"tt7654321\"}],",
        "");
    payload = replaceAll(
        std::move(payload),
        "\"origin\":\"primary-metadata\",",
        "");
    return payload;
}

EpgScraperMetadataResolution resolvePayload(const std::string& payload)
{
    MockTransport transport;
    transport.reply.transportSucceeded = true;
    transport.reply.replyCode = 250;
    transport.reply.payload = payload;
    SuiteBridgeEpgMetadataResolver resolver(transport);
    return resolver.resolve("default", event());
}

}

int main()
{
    {
        MockTransport transport;
        transport.reply.transportSucceeded = true;
        transport.reply.replyCode = 250;
        transport.reply.payload = completePayload();

        SuiteBridgeEpgMetadataResolver resolver(transport);
        const EpgScraperMetadataResolution resolution =
            resolver.resolve("default", event());

        assert(resolution.attempted);
        assert(resolution.found);
        assert(resolution.metadata.valid());
        assert(resolution.metadata.backendId == "default");
        assert(resolution.metadata.channelId == "S19.2E-1-1011-11100");
        assert(resolution.metadata.eventId == "12345");
        assert(resolution.metadata.mediaType == EpgScraperMediaType::Series);
        assert(resolution.metadata.providerId == 77);
        assert(resolution.metadata.seasonNumber == 3);
        assert(resolution.metadata.episodeNumber == 12);
        assert(resolution.metadata.absoluteEpisodeNumber == 44);
        assert(resolution.metadata.title == "Testserie");
        assert(resolution.metadata.originalTitle == "Test Series");
        assert(resolution.metadata.tagline == "Eine Zeile\nmit Umbruch");
        assert(resolution.metadata.overview == "Beschreibung mit \"Zitat\"");
        assert(resolution.metadata.imdbId == "tt7654321");
        assert(resolution.metadata.externalIds.size() == 2);
        assert(resolution.metadata.externalIds[0].provider ==
            EpgScraperExternalIdProvider::Imdb);
        assert(resolution.metadata.externalIds[0].scope ==
            EpgScraperExternalIdScope::Series);
        assert(resolution.metadata.externalIds[0].value == "tt1234567");
        assert(resolution.metadata.externalIds[1].scope ==
            EpgScraperExternalIdScope::Episode);
        assert(resolution.metadata.externalIds[1].value == "tt7654321");
        assert(resolution.metadata.genres.size() == 2);
        assert(resolution.metadata.genres[1] == "Mystery");
        assert(resolution.metadata.preferredArtwork.valid());
        assert(resolution.metadata.preferredArtwork.origin ==
            EpgScraperArtworkOrigin::PrimaryMetadata);
        assert(resolution.metadata.people.size() == 1);
        assert(resolution.metadata.people[0].role == EpgScraperPersonRole::Actor);
        assert(resolution.metadata.people[0].characterName == "Kommissarin Nord");
        assert(resolution.metadata.people[0].image.valid());
        assert(resolution.metadata.people[0].image.origin ==
            EpgScraperArtworkOrigin::PrimaryMetadata);
        assert(resolution.metadata.images.size() == 1);
        assert(resolution.metadata.images[0].orientation ==
            EpgScraperImageOrientation::Portrait);
        assert(resolution.metadata.images[0].artwork.valid());
        assert(resolution.metadata.images[0].artwork.origin ==
            EpgScraperArtworkOrigin::PrimaryMetadata);
        assert(transport.requestedChannelId == "S19.2E-1-1011-11100");
        assert(transport.requestedEventId == "12345");
    }

    {
        const EpgScraperMetadataResolution resolution =
            resolvePayload(legacyPayload());
        assert(resolution.attempted);
        assert(resolution.found);
        assert(resolution.metadata.externalIds.empty());
        assert(resolution.metadata.preferredArtwork.origin ==
            EpgScraperArtworkOrigin::PrimaryMetadata);
        assert(resolution.metadata.people[0].image.origin ==
            EpgScraperArtworkOrigin::PrimaryMetadata);
        assert(resolution.metadata.images[0].artwork.origin ==
            EpgScraperArtworkOrigin::PrimaryMetadata);
    }

    {
        MockTransport transport;
        transport.reply.transportSucceeded = true;
        transport.reply.replyCode = 250;
        transport.reply.payload =
            "{\"schema\":1,\"found\":false,\"provider\":\"none\"}";

        SuiteBridgeEpgMetadataResolver resolver(transport);
        const EpgScraperMetadataResolution resolution =
            resolver.resolve("default", event());
        assert(resolution.attempted);
        assert(!resolution.found);
    }

    {
        MockTransport transport;
        transport.reply.transportSucceeded = false;
        SuiteBridgeEpgMetadataResolver resolver(transport);
        const EpgScraperMetadataResolution resolution =
            resolver.resolve("default", event());
        assert(!resolution.attempted);
        assert(!resolution.found);
    }

    {
        MockTransport transport;
        transport.reply.transportSucceeded = true;
        transport.reply.replyCode = 451;
        SuiteBridgeEpgMetadataResolver resolver(transport);
        const EpgScraperMetadataResolution resolution =
            resolver.resolve("default", event());
        assert(resolution.attempted);
        assert(!resolution.found);
    }

    for (const std::string& malformed : {
            std::string("{}"),
            std::string("{\"schema\":2,\"found\":false,\"provider\":\"none\"}"),
            std::string("{\"schema\":1,\"found\":true,\"provider\":\"other\"}"),
            completePayload() + " trailing",
            std::string("{\"schema\":1,\"found\":false,\"provider\":\"none\",\"x\":\"\\uD800\"}"),
            replaceAll(
                completePayload(),
                "\"provider\":\"imdb\",\"scope\":\"series\"",
                "\"provider\":\"unknown\",\"scope\":\"series\""),
            replaceAll(
                completePayload(),
                "\"provider\":\"imdb\",\"scope\":\"series\"",
                "\"provider\":\"imdb\",\"scope\":\"unknown\""),
            replaceAll(
                completePayload(),
                "{\"provider\":\"imdb\",\"scope\":\"episode\",\"value\":\"tt7654321\"}",
                "{\"provider\":\"imdb\",\"scope\":\"series\",\"value\":\"tt1234567\"}"),
            replaceAll(
                completePayload(),
                "\"origin\":\"primary-metadata\"",
                "\"origin\":\"external-fallback\"")
        })
    {
        const EpgScraperMetadataResolution resolution = resolvePayload(malformed);
        assert(!resolution.attempted);
        assert(!resolution.found);
    }

    {
        MockTransport transport;
        transport.reply.transportSucceeded = true;
        transport.reply.replyCode = 250;
        transport.reply.payload = completePayload();
        SuiteBridgeEpgMetadataResolver resolver(transport);
        VdrEvent invalidEvent;
        const EpgScraperMetadataResolution resolution =
            resolver.resolve("default", invalidEvent);
        assert(!resolution.attempted);
        assert(transport.requestedChannelId.empty());
    }

    {
        EpgScraperArtwork fallback;
        fallback.available = true;
        fallback.provider = "tmdb";
        fallback.origin = EpgScraperArtworkOrigin::ExternalFallback;
        fallback.path = "/cache/fallback.jpg";
        fallback.width = 600;
        fallback.height = 900;
        assert(!fallback.valid());
    }

    return 0;
}

#include "SeriesArtworkFallbackMaterializingResolver.h"

#include <cassert>

namespace
{
class Delegate final : public IEpgScraperMetadataResolver
{
public:
    EpgScraperMetadataResolution value;

    EpgScraperMetadataResolution resolve(
        const std::string&,
        const VdrEvent&) override
    {
        return value;
    }
};

class Materializer final : public ISeriesArtworkFallbackMaterializer
{
public:
    int calls = 0;
    SeriesArtworkFallbackMaterializationRequest lastRequest;
    SeriesArtworkFallbackMaterializationResult value;

    SeriesArtworkFallbackMaterializationResult materialize(
        const SeriesArtworkFallbackMaterializationRequest& request) override
    {
        ++calls;
        lastRequest = request;
        return value;
    }
};

EpgScraperMetadataResolution seriesResolution()
{
    EpgScraperMetadataResolution resolution;
    resolution.attempted = true;
    resolution.found = true;
    resolution.metadata.backendId = "backend";
    resolution.metadata.channelId = "channel";
    resolution.metadata.eventId = "event";
    resolution.metadata.provider = "tvscraper";
    resolution.metadata.mediaType = EpgScraperMediaType::Series;
    resolution.metadata.seriesArtworkFallback.available = true;
    resolution.metadata.seriesArtworkFallback.provider = "provider";
    resolution.metadata.seriesArtworkFallback.origin =
        EpgScraperArtworkOrigin::ExternalFallback;
    resolution.metadata.seriesArtworkFallback.path = "/incoming/source.png";
    resolution.metadata.seriesArtworkFallback.width = 640;
    resolution.metadata.seriesArtworkFallback.height = 360;
    return resolution;
}

SeriesArtworkFallbackMaterializationResult materializedResult()
{
    SeriesArtworkFallbackMaterializationResult result;
    result.attempted = true;
    result.stored = true;
    result.artwork.available = true;
    result.artwork.provider = "provider";
    result.artwork.origin = EpgScraperArtworkOrigin::ExternalFallback;
    result.artwork.path = "/cache/series.png";
    result.artwork.width = 640;
    result.artwork.height = 360;
    return result;
}
}

int main()
{
    VdrEvent event;
    event.channelId = "event-channel";
    event.id = "event-id";

    {
        Delegate delegate;
        delegate.value = seriesResolution();
        Materializer materializer;
        materializer.value = materializedResult();
        SeriesArtworkFallbackMaterializingResolverConfig config;
        config.enabled = true;
        SeriesArtworkFallbackMaterializingResolver resolver(
            delegate,
            &materializer,
            config);

        const auto resolution = resolver.resolve("requested-backend", event);
        assert(materializer.calls == 1);
        assert(materializer.lastRequest.backendId == "backend");
        assert(materializer.lastRequest.channelId == "channel");
        assert(materializer.lastRequest.eventId == "event");
        assert(resolution.metadata.seriesArtworkFallback.path ==
               "/cache/series.png");
    }

    {
        Delegate delegate;
        delegate.value = seriesResolution();
        Materializer materializer;
        SeriesArtworkFallbackMaterializingResolver resolver(
            delegate,
            &materializer);
        const auto resolution = resolver.resolve("backend", event);
        assert(materializer.calls == 0);
        assert(!resolution.metadata.seriesArtworkFallback.available);
    }

    {
        Delegate delegate;
        delegate.value = seriesResolution();
        SeriesArtworkFallbackMaterializingResolverConfig config;
        config.enabled = true;
        SeriesArtworkFallbackMaterializingResolver resolver(
            delegate,
            nullptr,
            config);
        const auto resolution = resolver.resolve("backend", event);
        assert(!resolution.metadata.seriesArtworkFallback.available);
    }

    {
        Delegate delegate;
        delegate.value = seriesResolution();
        delegate.value.metadata.mediaType = EpgScraperMediaType::Movie;
        Materializer materializer;
        SeriesArtworkFallbackMaterializingResolverConfig config;
        config.enabled = true;
        SeriesArtworkFallbackMaterializingResolver resolver(
            delegate,
            &materializer,
            config);
        const auto resolution = resolver.resolve("backend", event);
        assert(materializer.calls == 0);
        assert(!resolution.metadata.seriesArtworkFallback.available);
    }

    {
        Delegate delegate;
        delegate.value = seriesResolution();
        delegate.value.found = false;
        Materializer materializer;
        SeriesArtworkFallbackMaterializingResolverConfig config;
        config.enabled = true;
        SeriesArtworkFallbackMaterializingResolver resolver(
            delegate,
            &materializer,
            config);
        const auto resolution = resolver.resolve("backend", event);
        assert(materializer.calls == 0);
        assert(!resolution.metadata.seriesArtworkFallback.available);
    }

    {
        Delegate delegate;
        delegate.value = seriesResolution();
        Materializer materializer;
        materializer.value.attempted = true;
        SeriesArtworkFallbackMaterializingResolverConfig config;
        config.enabled = true;
        SeriesArtworkFallbackMaterializingResolver resolver(
            delegate,
            &materializer,
            config);
        const auto resolution = resolver.resolve("backend", event);
        assert(materializer.calls == 1);
        assert(!resolution.metadata.seriesArtworkFallback.available);
    }

    return 0;
}

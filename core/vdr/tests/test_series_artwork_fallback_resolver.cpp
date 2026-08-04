#include "SeriesArtworkFallbackResolver.h"

#include <cassert>
#include <string>

namespace
{

class MockMetadataResolver final : public IEpgScraperMetadataResolver
{
public:
    EpgScraperMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override
    {
        ++calls;
        lastBackendId = backendId;
        lastEventId = event.id;
        return resolution;
    }

    EpgScraperMetadataResolution resolution;
    int calls = 0;
    std::string lastBackendId;
    std::string lastEventId;
};

class MockFallbackProvider final : public ISeriesArtworkFallbackProvider
{
public:
    SeriesArtworkFallbackResolution resolve(
        const std::string& backendId,
        const VdrEvent& event,
        const EpgScraperMetadata& metadata) override
    {
        ++calls;
        lastBackendId = backendId;
        lastEventId = event.id;
        lastMetadataTitle = metadata.title;
        return resolution;
    }

    SeriesArtworkFallbackResolution resolution;
    int calls = 0;
    std::string lastBackendId;
    std::string lastEventId;
    std::string lastMetadataTitle;
};

VdrEvent event()
{
    VdrEvent value;
    value.id = "event-1";
    value.channelId = "channel-1";
    value.title = "Testserie";
    return value;
}

EpgScraperMetadataResolution seriesResolution()
{
    EpgScraperMetadataResolution resolution;
    resolution.attempted = true;
    resolution.found = true;
    resolution.metadata.backendId = "default";
    resolution.metadata.channelId = "channel-1";
    resolution.metadata.eventId = "event-1";
    resolution.metadata.provider = "tvscraper";
    resolution.metadata.mediaType = EpgScraperMediaType::Series;
    resolution.metadata.title = "Testserie";
    return resolution;
}

EpgScraperArtwork primaryArtwork()
{
    EpgScraperArtwork artwork;
    artwork.available = true;
    artwork.provider = "tvscraper";
    artwork.origin = EpgScraperArtworkOrigin::PrimaryMetadata;
    artwork.path = "/cache/primary.jpg";
    artwork.width = 1280;
    artwork.height = 720;
    return artwork;
}

SeriesArtworkFallbackResolution fallbackResolution()
{
    SeriesArtworkFallbackResolution resolution;
    resolution.attempted = true;
    resolution.found = true;
    resolution.artwork.available = true;
    resolution.artwork.provider = "example-provider";
    resolution.artwork.origin = EpgScraperArtworkOrigin::ExternalFallback;
    resolution.artwork.path = "/cache/fallback.jpg";
    resolution.artwork.width = 600;
    resolution.artwork.height = 900;
    return resolution;
}

SeriesArtworkFallbackResolverConfig enabledConfig()
{
    SeriesArtworkFallbackResolverConfig config;
    config.enabled = true;
    return config;
}

}

int main()
{
    {
        MockMetadataResolver delegate;
        delegate.resolution = seriesResolution();
        MockFallbackProvider provider;
        provider.resolution = fallbackResolution();

        SeriesArtworkFallbackResolver resolver(delegate, &provider);
        const EpgScraperMetadataResolution resolution =
            resolver.resolve("default", event());

        assert(resolution.found);
        assert(!resolution.metadata.seriesArtworkFallback.available);
        assert(provider.calls == 0);
    }

    {
        MockMetadataResolver delegate;
        delegate.resolution = seriesResolution();

        SeriesArtworkFallbackResolver resolver(
            delegate,
            nullptr,
            enabledConfig());
        const EpgScraperMetadataResolution resolution =
            resolver.resolve("default", event());

        assert(resolution.found);
        assert(!resolution.metadata.seriesArtworkFallback.available);
    }

    {
        MockMetadataResolver delegate;
        delegate.resolution = seriesResolution();
        MockFallbackProvider provider;
        provider.resolution = fallbackResolution();

        SeriesArtworkFallbackResolver resolver(
            delegate,
            &provider,
            enabledConfig());
        const EpgScraperMetadataResolution resolution =
            resolver.resolve("default", event());

        assert(delegate.calls == 1);
        assert(provider.calls == 1);
        assert(provider.lastBackendId == "default");
        assert(provider.lastEventId == "event-1");
        assert(provider.lastMetadataTitle == "Testserie");
        assert(!resolution.metadata.preferredArtwork.available);
        assert(resolution.metadata.seriesArtworkFallback.available);
        assert(resolution.metadata.seriesArtworkFallback.provider ==
            "example-provider");
        assert(resolution.metadata.seriesArtworkFallback.origin ==
            EpgScraperArtworkOrigin::ExternalFallback);
        assert(resolution.metadata.seriesArtworkFallback.path ==
            "/cache/fallback.jpg");
    }

    {
        MockMetadataResolver delegate;
        delegate.resolution = seriesResolution();
        delegate.resolution.metadata.preferredArtwork = primaryArtwork();
        MockFallbackProvider provider;
        provider.resolution = fallbackResolution();

        SeriesArtworkFallbackResolver resolver(
            delegate,
            &provider,
            enabledConfig());
        const EpgScraperMetadataResolution resolution =
            resolver.resolve("default", event());

        assert(resolution.metadata.preferredArtwork.valid());
        assert(!resolution.metadata.seriesArtworkFallback.available);
        assert(provider.calls == 0);
    }

    {
        MockMetadataResolver delegate;
        delegate.resolution = seriesResolution();
        delegate.resolution.metadata.mediaType = EpgScraperMediaType::Movie;
        MockFallbackProvider provider;
        provider.resolution = fallbackResolution();

        SeriesArtworkFallbackResolver resolver(
            delegate,
            &provider,
            enabledConfig());
        resolver.resolve("default", event());
        assert(provider.calls == 0);
    }

    {
        MockMetadataResolver delegate;
        delegate.resolution = seriesResolution();
        delegate.resolution.found = false;
        MockFallbackProvider provider;
        provider.resolution = fallbackResolution();

        SeriesArtworkFallbackResolver resolver(
            delegate,
            &provider,
            enabledConfig());
        resolver.resolve("default", event());
        assert(provider.calls == 0);
    }

    {
        MockMetadataResolver delegate;
        delegate.resolution = seriesResolution();
        delegate.resolution.metadata.provider.clear();
        MockFallbackProvider provider;
        provider.resolution = fallbackResolution();

        SeriesArtworkFallbackResolver resolver(
            delegate,
            &provider,
            enabledConfig());
        resolver.resolve("default", event());
        assert(provider.calls == 0);
    }

    {
        MockMetadataResolver delegate;
        delegate.resolution = seriesResolution();
        MockFallbackProvider provider;
        provider.resolution = fallbackResolution();
        provider.resolution.artwork.provider = "tvscraper";
        provider.resolution.artwork.origin =
            EpgScraperArtworkOrigin::PrimaryMetadata;

        SeriesArtworkFallbackResolver resolver(
            delegate,
            &provider,
            enabledConfig());
        const EpgScraperMetadataResolution resolution =
            resolver.resolve("default", event());

        assert(provider.calls == 1);
        assert(!resolution.metadata.seriesArtworkFallback.available);
    }

    {
        MockMetadataResolver delegate;
        delegate.resolution = seriesResolution();
        MockFallbackProvider provider;
        provider.resolution = fallbackResolution();
        provider.resolution.found = false;

        SeriesArtworkFallbackResolver resolver(
            delegate,
            &provider,
            enabledConfig());
        const EpgScraperMetadataResolution resolution =
            resolver.resolve("default", event());

        assert(provider.calls == 1);
        assert(!resolution.metadata.seriesArtworkFallback.available);
    }

    {
        MockMetadataResolver delegate;
        delegate.resolution = seriesResolution();
        delegate.resolution.metadata.seriesArtworkFallback =
            fallbackResolution().artwork;
        MockFallbackProvider provider;
        provider.resolution = fallbackResolution();

        SeriesArtworkFallbackResolver resolver(
            delegate,
            &provider,
            enabledConfig());
        const EpgScraperMetadataResolution resolution =
            resolver.resolve("default", event());

        assert(provider.calls == 0);
        assert(resolution.metadata.seriesArtworkFallback.provider ==
            "example-provider");
    }

    return 0;
}

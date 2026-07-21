#pragma once

class IEpgScraperMetadataResolver;

class IEpgScraperMetadataResolverProvider
{
public:
    virtual ~IEpgScraperMetadataResolverProvider() = default;

    virtual IEpgScraperMetadataResolver* scraperMetadataResolver() = 0;
};

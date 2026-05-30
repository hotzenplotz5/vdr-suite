#include "MetadataService.h"
#include "MetadataRepository.h"

MetadataService::MetadataService(MetadataRepository& repository)
    : repository_(repository)
{
}

std::vector<Metadata> MetadataService::getAllMetadata()
{
    return repository_.getAllMetadata();
}

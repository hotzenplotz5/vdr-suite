#pragma once

#include "Metadata.h"

#include <vector>

class MetadataRepository;

class MetadataService
{
public:
    explicit MetadataService(MetadataRepository& repository);

    std::vector<Metadata> getAllMetadata();

private:
    MetadataRepository& repository_;
};

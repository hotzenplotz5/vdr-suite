#pragma once

#include "DashboardController.h"

class MetadataRepository;

class MetadataController
{
public:
    explicit MetadataController(
        MetadataRepository& metadataRepository);

    ApiResponse getMetadata();

private:
    MetadataRepository& metadataRepository_;
};

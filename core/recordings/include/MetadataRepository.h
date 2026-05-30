#pragma once

#include "Metadata.h"

#include <vector>

class Database;

class MetadataRepository
{
public:
    explicit MetadataRepository(Database& database);

    std::vector<Metadata> getAllMetadata();

private:
    Database& database_;
};

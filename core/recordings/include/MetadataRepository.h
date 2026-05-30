#pragma once

#include "Metadata.h"

#include <optional>
#include <vector>

class Database;

class MetadataRepository
{
public:
    explicit MetadataRepository(Database& database);

    std::vector<Metadata> getAllMetadata();
    std::optional<Metadata> getMetadataForRecording(int recordingId);

private:
    Database& database_;
};

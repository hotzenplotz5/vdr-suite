#pragma once

#include "Database.h"

#include <string>

class MediaTranscodeBackendSettingsRepository
{
public:
    explicit MediaTranscodeBackendSettingsRepository(Database& database)
        : database_(database)
    {
    }

    bool ensureSchema() const;

    bool readManagedMode(
        const std::string& backendId,
        std::string& mode) const;

    bool storeManagedMode(
        const std::string& backendId,
        const std::string& mode) const;

    bool clearManagedMode(const std::string& backendId) const;

private:
    Database& database_;
};

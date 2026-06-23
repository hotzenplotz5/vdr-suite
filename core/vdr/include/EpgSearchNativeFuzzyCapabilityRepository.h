#pragma once

#include "EpgSearchNativeFuzzyCapabilityDetector.h"

#include <optional>
#include <string>

class Database;

class EpgSearchNativeFuzzyCapabilityRepository
{
public:
    explicit EpgSearchNativeFuzzyCapabilityRepository(Database& database);

    bool ensureSchema();

    bool save(
        const std::string& backendId,
        const EpgSearchNativeFuzzyCapabilityProbeResult& result);

    std::optional<EpgSearchNativeFuzzyCapabilityProbeResult> load(
        const std::string& backendId) const;

private:
    Database& database_;
};

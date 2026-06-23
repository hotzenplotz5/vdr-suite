#pragma once

#include "EpgSearchNativeFuzzyCapabilityDetector.h"

#include <optional>
#include <string>

class Database;

struct EpgSearchNativeFuzzyPersistedCapabilityProbeResult
{
    EpgSearchNativeFuzzyCapabilityProbeResult probeResult;
    std::string updatedAt;
    long long ageSeconds = 0;
};

class EpgSearchNativeFuzzyCapabilityRepository
{
public:
    explicit EpgSearchNativeFuzzyCapabilityRepository(Database& database);

    bool ensureSchema();

    bool save(
        const std::string& backendId,
        const EpgSearchNativeFuzzyCapabilityProbeResult& result);

    std::optional<EpgSearchNativeFuzzyPersistedCapabilityProbeResult> loadPersistedProbeResult(
        const std::string& backendId) const;

    std::optional<EpgSearchNativeFuzzyCapabilityProbeResult> load(
        const std::string& backendId) const;

private:
    Database& database_;
};

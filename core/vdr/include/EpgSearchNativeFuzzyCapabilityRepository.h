#pragma once

#include "EpgSearchNativeFuzzyCapabilityDetector.h"

#include <optional>
#include <string>
#include <vector>

class Database;

struct EpgSearchNativeFuzzyPersistedCapabilityProbeResult
{
    std::string backendId;
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

    std::vector<EpgSearchNativeFuzzyPersistedCapabilityProbeResult> listPersistedProbeResults() const;

    bool deleteProbeResult(
        const std::string& backendId);

    std::optional<EpgSearchNativeFuzzyCapabilityProbeResult> load(
        const std::string& backendId) const;

private:
    Database& database_;
};

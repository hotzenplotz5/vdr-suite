#pragma once

#include "EpgSearchNativeFuzzyCapabilityFreshnessPolicy.h"
#include "EpgSearchNativeFuzzyCapabilityRepository.h"

#include <string>
#include <vector>

struct EpgSearchNativeFuzzyStaleProbeAdministrationRecord
{
    std::string backendId;
    std::string updatedAt;
    long long ageSeconds = 0;
    long long maxAgeSeconds = 0;
    std::string status;
    std::string reason;
};

struct EpgSearchNativeFuzzyStaleProbeAdministrationSummary
{
    bool schemaReady = false;
    int persistedResultsSeen = 0;
    int staleResultsFound = 0;
    int deletedResults = 0;
    int deleteFailures = 0;
};

class EpgSearchNativeFuzzyStaleProbeAdministrationService
{
public:
    EpgSearchNativeFuzzyStaleProbeAdministrationService(
        EpgSearchNativeFuzzyCapabilityRepository& repository,
        EpgSearchNativeFuzzyCapabilityFreshnessPolicy& freshnessPolicy);

    std::vector<EpgSearchNativeFuzzyStaleProbeAdministrationRecord> listStaleProbeResults();

    EpgSearchNativeFuzzyStaleProbeAdministrationSummary deleteStaleProbeResults();

private:
    EpgSearchNativeFuzzyCapabilityRepository& repository_;
    EpgSearchNativeFuzzyCapabilityFreshnessPolicy& freshnessPolicy_;
};

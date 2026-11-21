#include "EpgSearchNativeFuzzyStaleProbeAdministrationService.h"

EpgSearchNativeFuzzyStaleProbeAdministrationService::EpgSearchNativeFuzzyStaleProbeAdministrationService(
    EpgSearchNativeFuzzyCapabilityRepository& repository,
    EpgSearchNativeFuzzyCapabilityFreshnessPolicy& freshnessPolicy)
    : repository_(repository),
      freshnessPolicy_(freshnessPolicy)
{
}

std::vector<EpgSearchNativeFuzzyStaleProbeAdministrationRecord>
EpgSearchNativeFuzzyStaleProbeAdministrationService::listStaleProbeResults()
{
    std::vector<EpgSearchNativeFuzzyStaleProbeAdministrationRecord> staleResults;

    if (!repository_.ensureSchema())
    {
        return staleResults;
    }

    const auto persistedResults = repository_.listPersistedProbeResults();

    for (const auto& persisted : persistedResults)
    {
        const auto freshnessDecision = freshnessPolicy_.decide(
            persisted.ageSeconds);

        if (freshnessDecision.fresh)
        {
            continue;
        }

        EpgSearchNativeFuzzyStaleProbeAdministrationRecord record;
        record.backendId = persisted.backendId;
        record.updatedAt = persisted.updatedAt;
        record.ageSeconds = freshnessDecision.ageSeconds;
        record.maxAgeSeconds = freshnessDecision.maxAgeSeconds;
        record.status = freshnessDecision.status;
        record.reason = freshnessDecision.reason;
        staleResults.push_back(record);
    }

    return staleResults;
}

EpgSearchNativeFuzzyStaleProbeAdministrationSummary
EpgSearchNativeFuzzyStaleProbeAdministrationService::deleteStaleProbeResults()
{
    EpgSearchNativeFuzzyStaleProbeAdministrationSummary summary;

    if (!repository_.ensureSchema())
    {
        return summary;
    }

    summary.schemaReady = true;

    const auto persistedResults = repository_.listPersistedProbeResults();
    summary.persistedResultsSeen = static_cast<int>(persistedResults.size());

    for (const auto& persisted : persistedResults)
    {
        const auto freshnessDecision = freshnessPolicy_.decide(
            persisted.ageSeconds);

        if (freshnessDecision.fresh)
        {
            continue;
        }

        ++summary.staleResultsFound;

        if (repository_.deleteProbeResult(persisted.backendId))
        {
            ++summary.deletedResults;
        }
        else
        {
            ++summary.deleteFailures;
        }
    }

    return summary;
}

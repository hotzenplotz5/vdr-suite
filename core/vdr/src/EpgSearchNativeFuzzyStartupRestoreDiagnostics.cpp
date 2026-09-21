#include "EpgSearchNativeFuzzyStartupRestoreDiagnostics.h"

#include <algorithm>

std::string EpgSearchNativeFuzzyStartupRestoreDiagnostics::status() const
{
    if (!schemaReady)
    {
        return "schema-unavailable";
    }

    if (backendsSeen == 0)
    {
        return "no-backends";
    }

    if (persistedResultsFound == 0)
    {
        return "no-persisted-results";
    }

    if (nativeFuzzyAvailable > 0)
    {
        return "restored-native-available";
    }

    if (staleResultsIgnored > 0)
    {
        return "stale-results-ignored";
    }

    return "restored-native-unavailable";
}

std::string EpgSearchNativeFuzzyStartupRestoreDiagnostics::reason() const
{
    if (!schemaReady)
    {
        return "persistence schema was not available";
    }

    if (backendsSeen == 0)
    {
        return "no runtime backends were configured";
    }

    if (persistedResultsFound == 0)
    {
        return "no persisted native fuzzy probe results were found for configured backends";
    }

    if (nativeFuzzyAvailable > 0)
    {
        return "persisted native fuzzy probe results restored at least one native-capable backend";
    }

    if (staleResultsIgnored > 0)
    {
        return "all persisted native fuzzy probe results were stale and ignored for native enablement";
    }

    return "persisted native fuzzy probe results were found, but none restored native fuzzy availability";
}

RuntimeMeasurement EpgSearchNativeFuzzyStartupRestoreDiagnostics::toRuntimeMeasurement() const
{
    RuntimeMeasurement measurement;
    measurement.component = "epgsearch-native-fuzzy";
    measurement.operation = "startup-restore";
    measurement.durationMs = 0;
    measurement.statusCode = schemaReady ? 200 : 503;
    measurement.sizeBytes = 0;
    measurement.itemCount = static_cast<std::size_t>(
        persistedResultsFound);

    if (schemaReady && persistedResultsFound == 0)
    {
        measurement.statusCode = 204;
    }

    return measurement;
}

EpgSearchNativeFuzzyStartupRestoreDiagnostics
EpgSearchNativeFuzzyStartupRestoreDiagnostics::fromSummary(
    const EpgSearchNativeFuzzyStartupRestoreSummary& summary)
{
    EpgSearchNativeFuzzyStartupRestoreDiagnostics diagnostics;
    diagnostics.schemaReady = summary.schemaReady;
    diagnostics.backendsSeen = summary.backendsSeen;
    diagnostics.persistedResultsFound = summary.persistedResultsFound;
    diagnostics.backendsUpdated = summary.backendsUpdated;
    diagnostics.nativeFuzzyAvailable = summary.nativeFuzzyAvailable;
    diagnostics.nativeFuzzyUnavailable = std::max(
        0,
        summary.persistedResultsFound - summary.nativeFuzzyAvailable);
    diagnostics.staleResultsIgnored = summary.staleResultsIgnored;

    return diagnostics;
}

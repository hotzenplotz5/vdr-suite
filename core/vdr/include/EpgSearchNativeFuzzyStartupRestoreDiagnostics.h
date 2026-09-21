#pragma once

#include "EpgSearchNativeFuzzyStartupRestoreService.h"
#include "RuntimeMeasurement.h"

#include <string>

struct EpgSearchNativeFuzzyStartupRestoreDiagnostics
{
    bool schemaReady = false;
    int backendsSeen = 0;
    int persistedResultsFound = 0;
    int backendsUpdated = 0;
    int nativeFuzzyAvailable = 0;
    int nativeFuzzyUnavailable = 0;
    int staleResultsIgnored = 0;

    std::string status() const;
    std::string reason() const;
    RuntimeMeasurement toRuntimeMeasurement() const;

    static EpgSearchNativeFuzzyStartupRestoreDiagnostics fromSummary(
        const EpgSearchNativeFuzzyStartupRestoreSummary& summary);
};

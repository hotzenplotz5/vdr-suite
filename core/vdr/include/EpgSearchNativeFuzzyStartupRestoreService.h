#pragma once

class BackendRegistryService;
class EpgSearchNativeFuzzyCapabilityDetector;
class EpgSearchNativeFuzzyCapabilityFreshnessPolicy;
class EpgSearchNativeFuzzyCapabilityRepository;

struct EpgSearchNativeFuzzyStartupRestoreSummary
{
    bool schemaReady = false;
    int backendsSeen = 0;
    int persistedResultsFound = 0;
    int backendsUpdated = 0;
    int nativeFuzzyAvailable = 0;
    int staleResultsIgnored = 0;
};

class EpgSearchNativeFuzzyStartupRestoreService
{
public:
    EpgSearchNativeFuzzyStartupRestoreService(
        EpgSearchNativeFuzzyCapabilityRepository& repository,
        EpgSearchNativeFuzzyCapabilityDetector& detector,
        BackendRegistryService& backendRegistryService,
        EpgSearchNativeFuzzyCapabilityFreshnessPolicy& freshnessPolicy);

    EpgSearchNativeFuzzyStartupRestoreSummary restoreAllBackends();

private:
    EpgSearchNativeFuzzyCapabilityRepository& repository_;
    EpgSearchNativeFuzzyCapabilityDetector& detector_;
    BackendRegistryService& backendRegistryService_;
    EpgSearchNativeFuzzyCapabilityFreshnessPolicy& freshnessPolicy_;
};

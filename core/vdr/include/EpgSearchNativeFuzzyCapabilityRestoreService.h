#pragma once

#include <string>

class BackendRegistryService;
class EpgSearchNativeFuzzyCapabilityDetector;
class EpgSearchNativeFuzzyCapabilityRepository;

struct EpgSearchNativeFuzzyCapabilityRestoreResult
{
    bool persistedResultFound = false;
    bool backendFound = false;
    bool backendUpdated = false;
    bool nativeFuzzyAvailable = false;
};

class EpgSearchNativeFuzzyCapabilityRestoreService
{
public:
    EpgSearchNativeFuzzyCapabilityRestoreService(
        EpgSearchNativeFuzzyCapabilityRepository& repository,
        EpgSearchNativeFuzzyCapabilityDetector& detector,
        BackendRegistryService& backendRegistryService);

    EpgSearchNativeFuzzyCapabilityRestoreResult restoreBackend(
        const std::string& backendId);

private:
    EpgSearchNativeFuzzyCapabilityRepository& repository_;
    EpgSearchNativeFuzzyCapabilityDetector& detector_;
    BackendRegistryService& backendRegistryService_;
};

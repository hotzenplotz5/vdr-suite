#pragma once

#include "EpgSearchNativeFuzzyCapabilityDetector.h"

#include <string>
#include <vector>

class BackendRegistryService;
class EpgSearchNativeFuzzyCapabilityRepository;
class ISearchTimerCommandExecutor;
class ISearchTimerDataSource;

struct EpgSearchNativeFuzzyOperatorRefreshRequest
{
    std::string backendId = "default";
    std::string probeQuery = "VDR-Suite Native Fuzzy Operator Refresh";
    int tolerance = 2;
    bool keepProbeSearchTimer = false;
    bool updateBackendCapabilities = true;
};

struct EpgSearchNativeFuzzyOperatorRefreshSummary
{
    std::string backendId;
    std::string backendNativeId;
    std::string probeQuery;
    int tolerance = 0;
    bool backendKnown = false;
    bool createAttempted = false;
    bool createAccepted = false;
    bool readbackAvailable = false;
    bool modePreserved = false;
    bool tolerancePreserved = false;
    bool cleanupAttempted = false;
    bool cleanupSucceeded = false;
    bool persisted = false;
    bool backendCapabilitiesUpdated = false;
    bool nativeFuzzyAvailable = false;
    std::string status;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

class EpgSearchNativeFuzzyOperatorRefreshService
{
public:
    EpgSearchNativeFuzzyOperatorRefreshService(
        ISearchTimerCommandExecutor& commandExecutor,
        ISearchTimerDataSource& dataSource,
        EpgSearchNativeFuzzyCapabilityRepository& repository,
        EpgSearchNativeFuzzyCapabilityDetector& detector,
        BackendRegistryService& backendRegistryService);

    EpgSearchNativeFuzzyOperatorRefreshSummary refresh(
        const EpgSearchNativeFuzzyOperatorRefreshRequest& request);

private:
    ISearchTimerCommandExecutor& commandExecutor_;
    ISearchTimerDataSource& dataSource_;
    EpgSearchNativeFuzzyCapabilityRepository& repository_;
    EpgSearchNativeFuzzyCapabilityDetector& detector_;
    BackendRegistryService& backendRegistryService_;
};

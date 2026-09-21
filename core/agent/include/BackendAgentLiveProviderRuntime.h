#pragma once

#include "BackendAgentLiveProviderAuthority.h"
#include "SuiteBridgeLiveSourceTransport.h"

#include <string>

class BackendAgentCommandRepository;
class BackendAgentRepository;

namespace vdrsuite::agent
{

struct BackendAgentLiveProviderPreparation
{
    bool valid = false;
    std::string reasonCode;
    BackendAgentLiveProviderPin pin;
    BackendAgentLocalProviderFacts providerFacts;
};

struct BackendAgentLiveProviderOpenResult
{
    bool opened = false;
    std::string reasonCode;
    std::string unixSocketPath;
};

struct BackendAgentLiveProviderStatus
{
    bool current = false;
    bool receiverAttached = false;
    std::string state;
    std::string reasonCode;
};

class BackendAgentLiveProviderRuntime
{
public:
    BackendAgentLiveProviderRuntime(
        BackendAgentRepository& agentRepository,
        BackendAgentCommandRepository& commandRepository,
        ISuiteBridgeLiveSourceTransport& transport);

    BackendAgentLiveProviderPreparation prepare(
        const std::string& backendId,
        const std::string& channelId) const;

    bool current(
        const BackendAgentLiveProviderPreparation& preparation,
        std::string& reasonCode) const;

    BackendAgentLiveProviderOpenResult open(
        const BackendAgentLiveProviderPreparation& preparation,
        const std::string& leaseId) const;

    BackendAgentLiveProviderStatus status(
        const BackendAgentLiveProviderPreparation& preparation,
        const std::string& leaseId) const;

    bool close(
        const BackendAgentLiveProviderPreparation& preparation,
        const std::string& leaseId,
        std::string& reasonCode) const;

private:
    bool discoverFacts(
        BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) const;

    BackendAgentRepository& agentRepository_;
    BackendAgentCommandRepository& commandRepository_;
    ISuiteBridgeLiveSourceTransport& transport_;
    BackendAgentLiveProviderAuthority authority_;
};

}

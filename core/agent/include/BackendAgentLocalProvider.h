#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::agent
{

struct BackendAgentLocalProviderFacts
{
    std::string providerId;
    std::string providerKind;
    std::string providerInstanceEpoch;
    std::uint64_t providerGeneration = 0;
    std::uint64_t capabilityRevision = 0;
    bool available = false;
    std::vector<std::string> capabilities;
};

struct BackendAgentLocalProviderOwnership
{
    std::string backendId;
    std::string authorityDomain;
    std::string providerId;
    std::string providerKind;
    std::uint64_t ownershipGeneration = 0;
    std::vector<std::string> allowedCapabilities;
};

struct BackendAgentLocalProviderSelection
{
    std::string backendId;
    std::string authorityDomain;
    std::string providerId;
    std::string providerKind;
    std::uint64_t ownershipGeneration = 0;
    std::string providerInstanceEpoch;
    std::uint64_t providerGeneration = 0;
    std::uint64_t capabilityRevision = 0;
    std::string requiredCapability;
};

bool backendAgentLocalProviderKnownKind(const std::string& providerKind);
bool backendAgentLocalProviderValidFacts(
    const BackendAgentLocalProviderFacts& facts);
bool backendAgentLocalProviderValidOwnership(
    const BackendAgentLocalProviderOwnership& ownership);
bool backendAgentLocalProviderValidSelection(
    const BackendAgentLocalProviderSelection& selection);

BackendAgentLocalProviderSelection backendAgentLocalProviderSelect(
    const BackendAgentLocalProviderOwnership& ownership,
    const BackendAgentLocalProviderFacts& facts,
    const std::string& requiredCapability,
    std::string& reasonCode);

bool backendAgentLocalProviderSelectionUsable(
    const BackendAgentLocalProviderSelection& selection,
    const BackendAgentLocalProviderOwnership& ownership,
    const BackendAgentLocalProviderFacts& facts,
    std::string& reasonCode);

bool backendAgentLocalProviderSameFence(
    const BackendAgentLocalProviderSelection& left,
    const BackendAgentLocalProviderSelection& right);

std::string backendAgentLocalProviderSelectionIdentity(
    const BackendAgentLocalProviderSelection& selection);

}

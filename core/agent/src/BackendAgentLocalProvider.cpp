#include "BackendAgentLocalProvider.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace vdrsuite::agent
{
namespace
{
bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '-' ||
            character == '_' || character == '.' || character == ':';
    });
}

bool boundedUniqueIdentifiers(const std::vector<std::string>& values)
{
    if (values.empty() || values.size() > 64) return false;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (!safeIdentifier(values[index])) return false;
        if (std::find(values.begin(), values.begin() + index, values[index]) !=
            values.begin() + index) return false;
    }
    return true;
}

bool contains(
    const std::vector<std::string>& values,
    const std::string& expected)
{
    return std::find(values.begin(), values.end(), expected) != values.end();
}

void appendIdentityField(std::ostringstream& output, const std::string& value)
{
    output << value.size() << ':' << value << '|';
}
}

bool backendAgentLocalProviderKnownKind(const std::string& providerKind)
{
    return providerKind == "suitebridge" ||
        providerKind == "restfulapi" ||
        providerKind == "external";
}

bool backendAgentLocalProviderValidFacts(
    const BackendAgentLocalProviderFacts& facts)
{
    return safeIdentifier(facts.providerId) &&
        backendAgentLocalProviderKnownKind(facts.providerKind) &&
        safeIdentifier(facts.providerInstanceEpoch) &&
        facts.providerGeneration > 0 && facts.capabilityRevision > 0 &&
        boundedUniqueIdentifiers(facts.capabilities);
}

bool backendAgentLocalProviderValidOwnership(
    const BackendAgentLocalProviderOwnership& ownership)
{
    return safeIdentifier(ownership.backendId) &&
        safeIdentifier(ownership.authorityDomain) &&
        safeIdentifier(ownership.providerId) &&
        backendAgentLocalProviderKnownKind(ownership.providerKind) &&
        ownership.ownershipGeneration > 0 &&
        boundedUniqueIdentifiers(ownership.allowedCapabilities);
}

bool backendAgentLocalProviderValidSelection(
    const BackendAgentLocalProviderSelection& selection)
{
    return safeIdentifier(selection.backendId) &&
        safeIdentifier(selection.authorityDomain) &&
        safeIdentifier(selection.providerId) &&
        backendAgentLocalProviderKnownKind(selection.providerKind) &&
        selection.ownershipGeneration > 0 &&
        safeIdentifier(selection.providerInstanceEpoch) &&
        selection.providerGeneration > 0 &&
        selection.capabilityRevision > 0 &&
        safeIdentifier(selection.requiredCapability);
}

BackendAgentLocalProviderSelection backendAgentLocalProviderSelect(
    const BackendAgentLocalProviderOwnership& ownership,
    const BackendAgentLocalProviderFacts& facts,
    const std::string& requiredCapability,
    std::string& reasonCode)
{
    BackendAgentLocalProviderSelection selection;
    if (!backendAgentLocalProviderValidOwnership(ownership) ||
        !backendAgentLocalProviderValidFacts(facts) ||
        !safeIdentifier(requiredCapability))
    {
        reasonCode = "invalid_local_provider_contract";
        return selection;
    }
    if (ownership.providerId != facts.providerId ||
        ownership.providerKind != facts.providerKind)
    {
        reasonCode = "local_provider_not_owner";
        return selection;
    }
    if (!contains(ownership.allowedCapabilities, requiredCapability))
    {
        reasonCode = "local_provider_capability_not_authorized";
        return selection;
    }
    if (!contains(facts.capabilities, requiredCapability))
    {
        reasonCode = "local_provider_capability_not_observed";
        return selection;
    }
    if (!facts.available)
    {
        reasonCode = "local_provider_unavailable";
        return selection;
    }

    selection.backendId = ownership.backendId;
    selection.authorityDomain = ownership.authorityDomain;
    selection.providerId = ownership.providerId;
    selection.providerKind = ownership.providerKind;
    selection.ownershipGeneration = ownership.ownershipGeneration;
    selection.providerInstanceEpoch = facts.providerInstanceEpoch;
    selection.providerGeneration = facts.providerGeneration;
    selection.capabilityRevision = facts.capabilityRevision;
    selection.requiredCapability = requiredCapability;
    reasonCode = "local_provider_selected";
    return selection;
}

bool backendAgentLocalProviderSelectionUsable(
    const BackendAgentLocalProviderSelection& selection,
    const BackendAgentLocalProviderOwnership& ownership,
    const BackendAgentLocalProviderFacts& facts,
    std::string& reasonCode)
{
    if (!backendAgentLocalProviderValidSelection(selection) ||
        !backendAgentLocalProviderValidOwnership(ownership) ||
        !backendAgentLocalProviderValidFacts(facts))
    {
        reasonCode = "invalid_local_provider_fence";
        return false;
    }
    if (selection.backendId != ownership.backendId ||
        selection.authorityDomain != ownership.authorityDomain ||
        selection.providerId != ownership.providerId ||
        selection.providerKind != ownership.providerKind ||
        selection.ownershipGeneration != ownership.ownershipGeneration)
    {
        reasonCode = "local_provider_ownership_changed";
        return false;
    }
    if (!contains(ownership.allowedCapabilities, selection.requiredCapability))
    {
        reasonCode = "local_provider_capability_not_authorized";
        return false;
    }
    if (selection.providerId != facts.providerId ||
        selection.providerKind != facts.providerKind)
    {
        reasonCode = "local_provider_identity_changed";
        return false;
    }
    if (selection.providerInstanceEpoch != facts.providerInstanceEpoch)
    {
        reasonCode = "local_provider_instance_epoch_changed";
        return false;
    }
    if (selection.providerGeneration != facts.providerGeneration)
    {
        reasonCode = "local_provider_generation_changed";
        return false;
    }
    if (selection.capabilityRevision != facts.capabilityRevision)
    {
        reasonCode = "local_provider_capability_revision_changed";
        return false;
    }
    if (!contains(facts.capabilities, selection.requiredCapability))
    {
        reasonCode = "local_provider_capability_not_observed";
        return false;
    }
    if (!facts.available)
    {
        reasonCode = "local_provider_unavailable";
        return false;
    }
    reasonCode = "local_provider_fence_current";
    return true;
}

bool backendAgentLocalProviderSameFence(
    const BackendAgentLocalProviderSelection& left,
    const BackendAgentLocalProviderSelection& right)
{
    return backendAgentLocalProviderValidSelection(left) &&
        backendAgentLocalProviderValidSelection(right) &&
        left.backendId == right.backendId &&
        left.authorityDomain == right.authorityDomain &&
        left.providerId == right.providerId &&
        left.providerKind == right.providerKind &&
        left.ownershipGeneration == right.ownershipGeneration &&
        left.providerInstanceEpoch == right.providerInstanceEpoch &&
        left.providerGeneration == right.providerGeneration &&
        left.capabilityRevision == right.capabilityRevision &&
        left.requiredCapability == right.requiredCapability;
}

std::string backendAgentLocalProviderSelectionIdentity(
    const BackendAgentLocalProviderSelection& selection)
{
    if (!backendAgentLocalProviderValidSelection(selection)) return {};
    std::ostringstream output;
    output << "local-provider-selection/1|";
    appendIdentityField(output, selection.backendId);
    appendIdentityField(output, selection.authorityDomain);
    appendIdentityField(output, selection.providerKind);
    appendIdentityField(output, selection.providerId);
    output << "ownershipGeneration=" << selection.ownershipGeneration << '|';
    appendIdentityField(output, selection.providerInstanceEpoch);
    output << "providerGeneration=" << selection.providerGeneration << '|'
           << "capabilityRevision=" << selection.capabilityRevision << '|';
    appendIdentityField(output, selection.requiredCapability);
    return output.str();
}

}

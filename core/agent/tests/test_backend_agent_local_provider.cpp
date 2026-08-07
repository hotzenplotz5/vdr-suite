#include "BackendAgentLocalProvider.h"

#include <cassert>
#include <string>

using namespace vdrsuite::agent;

namespace
{
BackendAgentLocalProviderFacts suiteBridgeFacts()
{
    BackendAgentLocalProviderFacts facts;
    facts.providerId = "suitebridge:local";
    facts.providerKind = "suitebridge";
    facts.providerInstanceEpoch = "pie_7";
    facts.providerGeneration = 7;
    facts.capabilityRevision = 11;
    facts.available = true;
    facts.capabilities = {"vdr.native.probe", "vdr.channels.read"};
    return facts;
}

BackendAgentLocalProviderOwnership nativeOwnership()
{
    BackendAgentLocalProviderOwnership ownership;
    ownership.backendId = "default";
    ownership.authorityDomain = "vdr.native";
    ownership.providerId = "suitebridge:local";
    ownership.providerKind = "suitebridge";
    ownership.ownershipGeneration = 3;
    ownership.allowedCapabilities = {"vdr.native.probe"};
    return ownership;
}
}

int main()
{
    std::string reason;
    auto facts = suiteBridgeFacts();
    auto ownership = nativeOwnership();

    const auto selection = backendAgentLocalProviderSelect(
        ownership, facts, "vdr.native.probe", reason);
    assert(reason == "local_provider_selected");
    assert(backendAgentLocalProviderValidSelection(selection));
    assert(backendAgentLocalProviderSelectionUsable(
        selection, ownership, facts, reason));
    assert(reason == "local_provider_fence_current");
    assert(!backendAgentLocalProviderSelectionIdentity(selection).empty());

    auto unauthorizedOwnership = ownership;
    unauthorizedOwnership.allowedCapabilities = {"vdr.channels.read"};
    assert(!backendAgentLocalProviderValidSelection(
        backendAgentLocalProviderSelect(
            unauthorizedOwnership, facts, "vdr.native.probe", reason)));
    assert(reason == "local_provider_capability_not_authorized");

    auto unavailableFacts = facts;
    unavailableFacts.available = false;
    assert(!backendAgentLocalProviderValidSelection(
        backendAgentLocalProviderSelect(
            ownership, unavailableFacts, "vdr.native.probe", reason)));
    assert(reason == "local_provider_unavailable");

    auto otherAvailableProvider = facts;
    otherAvailableProvider.providerId = "restfulapi:local";
    otherAvailableProvider.providerKind = "restfulapi";
    assert(!backendAgentLocalProviderValidSelection(
        backendAgentLocalProviderSelect(
            ownership, otherAvailableProvider, "vdr.native.probe", reason)));
    assert(reason == "local_provider_not_owner");

    auto newOwnership = ownership;
    ++newOwnership.ownershipGeneration;
    assert(!backendAgentLocalProviderSelectionUsable(
        selection, newOwnership, facts, reason));
    assert(reason == "local_provider_ownership_changed");

    auto restartedFacts = facts;
    restartedFacts.providerInstanceEpoch = "pie_8";
    assert(!backendAgentLocalProviderSelectionUsable(
        selection, ownership, restartedFacts, reason));
    assert(reason == "local_provider_instance_epoch_changed");

    auto replacementFacts = facts;
    ++replacementFacts.providerGeneration;
    assert(!backendAgentLocalProviderSelectionUsable(
        selection, ownership, replacementFacts, reason));
    assert(reason == "local_provider_generation_changed");

    auto changedCapabilities = facts;
    ++changedCapabilities.capabilityRevision;
    assert(!backendAgentLocalProviderSelectionUsable(
        selection, ownership, changedCapabilities, reason));
    assert(reason == "local_provider_capability_revision_changed");

    auto removedCapability = facts;
    removedCapability.capabilities = {"vdr.channels.read"};
    assert(!backendAgentLocalProviderSelectionUsable(
        selection, ownership, removedCapability, reason));
    assert(reason == "local_provider_capability_not_observed");

    auto replaySelection = selection;
    assert(backendAgentLocalProviderSameFence(selection, replaySelection));
    ++replaySelection.providerGeneration;
    assert(!backendAgentLocalProviderSameFence(selection, replaySelection));

    BackendAgentLocalProviderFacts unknown = facts;
    unknown.providerKind = "automatic";
    assert(!backendAgentLocalProviderValidFacts(unknown));

    return 0;
}

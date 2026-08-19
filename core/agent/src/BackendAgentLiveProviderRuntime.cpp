#include "BackendAgentLiveProviderRuntime.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace vdrsuite::agent
{
namespace
{

bool jsonString(const std::string& input, const char* key, std::string& value)
{
    if (key == nullptr) return false;
    const std::string marker = std::string("\"") + key + "\":\"";
    const std::size_t start = input.find(marker);
    if (start == std::string::npos) return false;
    std::size_t position = start + marker.size();
    value.clear();
    while (position < input.size()) {
        const char character = input[position++];
        if (character == '"') return !value.empty() && value.size() <= 256;
        if (character == '\\' || static_cast<unsigned char>(character) < 0x20U)
            return false;
        value.push_back(character);
        if (value.size() > 256) return false;
    }
    return false;
}

bool jsonUnsigned(const std::string& input, const char* key, std::uint64_t& value)
{
    if (key == nullptr) return false;
    const std::string marker = std::string("\"") + key + "\":";
    const std::size_t start = input.find(marker);
    if (start == std::string::npos) return false;
    std::size_t position = start + marker.size();
    if (position >= input.size() || !std::isdigit(static_cast<unsigned char>(input[position])))
        return false;
    std::string digits;
    while (position < input.size() &&
           std::isdigit(static_cast<unsigned char>(input[position]))) {
        digits.push_back(input[position++]);
        if (digits.size() > 19) return false;
    }
    if (digits.empty()) return false;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(digits.c_str(), &end, 10);
    if (end == nullptr || *end != '\0') return false;
    value = static_cast<std::uint64_t>(parsed);
    return true;
}

bool jsonBool(const std::string& input, const char* key, bool& value)
{
    if (key == nullptr) return false;
    const std::string marker = std::string("\"") + key + "\":";
    const std::size_t start = input.find(marker);
    if (start == std::string::npos) return false;
    const std::size_t position = start + marker.size();
    if (input.compare(position, 4, "true") == 0) { value = true; return true; }
    if (input.compare(position, 5, "false") == 0) { value = false; return true; }
    return false;
}

bool safeSocketPath(const std::string& value)
{
    if (value.empty() || value.front() != '/' || value.size() > 100 ||
        value.find("..") != std::string::npos || value.find('?') != std::string::npos ||
        value.find('#') != std::string::npos) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '/' ||
            character == '-' || character == '_' || character == '.' ||
            character == ':';
    });
}

bool keyValue(const std::string& payload, const char* key, std::string& value)
{
    const std::string marker = std::string(key) + '=';
    const std::size_t found = payload.find(marker);
    if (found == std::string::npos) return false;
    const std::size_t start = found + marker.size();
    const std::size_t end = payload.find(' ', start);
    value = payload.substr(start, end == std::string::npos ? std::string::npos : end - start);
    return !value.empty() && value.size() <= 256;
}

bool parseBooleanToken(const std::string& value, bool& parsed)
{
    if (value == "true") { parsed = true; return true; }
    if (value == "false") { parsed = false; return true; }
    return false;
}

}

BackendAgentLiveProviderRuntime::BackendAgentLiveProviderRuntime(
    BackendAgentRepository& agentRepository,
    BackendAgentCommandRepository& commandRepository,
    ISuiteBridgeLiveSourceTransport& transport)
    : agentRepository_(agentRepository),
      commandRepository_(commandRepository),
      transport_(transport)
{
}

bool BackendAgentLiveProviderRuntime::discoverFacts(
    BackendAgentLocalProviderFacts& facts,
    std::string& reasonCode) const
{
    facts = {};
    const SuiteBridgeCommandReply reply = transport_.discoverLiveSource();
    if (!reply.transportSucceeded() || reply.replyCode != 250 ||
        reply.payload.empty() || reply.payload.size() > 2048) {
        reasonCode = "live_provider_capability_unavailable";
        return false;
    }
    std::string capability;
    bool available = false;
    if (!jsonString(reply.payload, "providerId", facts.providerId) ||
        !jsonString(reply.payload, "providerKind", facts.providerKind) ||
        !jsonString(reply.payload, "pluginInstanceEpoch", facts.providerInstanceEpoch) ||
        !jsonUnsigned(reply.payload, "providerGeneration", facts.providerGeneration) ||
        !jsonUnsigned(reply.payload, "capabilityRevision", facts.capabilityRevision) ||
        !jsonString(reply.payload, "capability", capability) ||
        !jsonBool(reply.payload, "available", available) ||
        facts.providerId != "suitebridge:local" ||
        facts.providerKind != "suitebridge" ||
        capability != BackendAgentLiveProviderAuthority::RequiredCapability ||
        facts.providerGeneration == 0 || facts.capabilityRevision == 0) {
        reasonCode = "live_provider_capability_invalid";
        return false;
    }
    facts.available = available;
    facts.capabilities = {capability};
    if (!backendAgentLocalProviderValidFacts(facts)) {
        reasonCode = "live_provider_facts_invalid";
        return false;
    }
    reasonCode = available
        ? "live_provider_capability_current"
        : "live_provider_unavailable";
    return available;
}

BackendAgentLiveProviderPreparation BackendAgentLiveProviderRuntime::prepare(
    const std::string& backendId,
    const std::string& channelId) const
{
    BackendAgentLiveProviderPreparation result;
    const auto agent = agentRepository_.findAgentForBackend(backendId);
    if (!agent.has_value()) {
        result.reasonCode = "live_backend_authority_required";
        return result;
    }
    const auto cursor = agentRepository_.observationCursorForBackend(
        backendId, BackendAgentLiveProviderAuthority::ChannelObservationDomain);
    const auto channels = agentRepository_.channelFactsForBackend(backendId);

    if (!discoverFacts(result.providerFacts, result.reasonCode)) return result;

    const auto ownership = commandRepository_.localProviderOwnershipStatus(
        backendId, BackendAgentLiveProviderAuthority::AuthorityDomain);
    if (!ownership.present || !ownership.active) {
        result.reasonCode = "live_provider_ownership_required";
        return result;
    }
    std::string selectionReason;
    const auto selection = backendAgentLocalProviderSelect(
        ownership.ownership,
        result.providerFacts,
        BackendAgentLiveProviderAuthority::RequiredCapability,
        selectionReason);
    if (!backendAgentLocalProviderValidSelection(selection)) {
        result.reasonCode = selectionReason.empty()
            ? "live_provider_selection_required"
            : selectionReason;
        return result;
    }

    result.pin = authority_.pin(
        backendId, channelId, *agent, cursor, channels, selection,
        ownership.ownership, result.providerFacts);
    result.valid = result.pin.valid;
    result.reasonCode = result.pin.reasonCode;
    return result;
}

bool BackendAgentLiveProviderRuntime::current(
    const BackendAgentLiveProviderPreparation& preparation,
    std::string& reasonCode) const
{
    if (!preparation.valid || !preparation.pin.valid) {
        reasonCode = "live_provider_pin_required";
        return false;
    }
    const auto& fence = preparation.pin.channelFence;
    const auto agent = agentRepository_.findAgentForBackend(fence.backendId);
    if (!agent.has_value()) {
        reasonCode = "live_backend_generation_stale";
        return false;
    }
    const auto cursor = agentRepository_.observationCursorForBackend(
        fence.backendId, BackendAgentLiveProviderAuthority::ChannelObservationDomain);
    const auto channels = agentRepository_.channelFactsForBackend(fence.backendId);
    BackendAgentLocalProviderFacts currentFacts;
    if (!discoverFacts(currentFacts, reasonCode)) return false;
    const auto ownership = commandRepository_.localProviderOwnershipStatus(
        fence.backendId, BackendAgentLiveProviderAuthority::AuthorityDomain);
    if (!ownership.present || !ownership.active) {
        reasonCode = "live_provider_ownership_required";
        return false;
    }
    return authority_.usable(
        preparation.pin, *agent, cursor, channels,
        ownership.ownership, currentFacts, reasonCode);
}

BackendAgentLiveProviderOpenResult BackendAgentLiveProviderRuntime::open(
    const BackendAgentLiveProviderPreparation& preparation,
    const std::string& leaseId) const
{
    BackendAgentLiveProviderOpenResult result;
    if (!current(preparation, result.reasonCode)) return result;
    SuiteBridgeLiveSourceOpenRequest request;
    request.leaseId = leaseId;
    request.channelId = preparation.pin.channelFence.channelId;
    request.pluginInstanceEpoch =
        preparation.pin.providerSelection.providerInstanceEpoch;
    const auto reply = transport_.openLiveSource(request);
    if (!reply.transportSucceeded() || reply.replyCode != 250) {
        result.reasonCode = "live_provider_open_failed";
        return result;
    }
    std::string state;
    std::string receiver;
    std::string channel;
    std::string socket;
    std::string epoch;
    bool receiverAttached = false;
    if (!keyValue(reply.payload, "state", state) || state != "active" ||
        !keyValue(reply.payload, "receiverAttached", receiver) ||
        !parseBooleanToken(receiver, receiverAttached) || !receiverAttached ||
        !keyValue(reply.payload, "channelId", channel) ||
        channel != request.channelId ||
        !keyValue(reply.payload, "socket", socket) || !safeSocketPath(socket) ||
        !keyValue(reply.payload, "pluginInstanceEpoch", epoch) ||
        epoch != request.pluginInstanceEpoch) {
        std::string ignored;
        close(preparation, leaseId, ignored);
        result.reasonCode = "live_provider_open_evidence_invalid";
        return result;
    }
    result.opened = true;
    result.unixSocketPath = socket;
    result.reasonCode = "live_provider_opened";
    return result;
}

BackendAgentLiveProviderStatus BackendAgentLiveProviderRuntime::status(
    const BackendAgentLiveProviderPreparation& preparation,
    const std::string& leaseId) const
{
    BackendAgentLiveProviderStatus result;
    std::string currentReason;
    if (!current(preparation, currentReason)) {
        result.reasonCode = currentReason;
        return result;
    }
    SuiteBridgeLiveSourceLeaseRequest request{
        leaseId, preparation.pin.providerSelection.providerInstanceEpoch};
    const auto reply = transport_.statusLiveSource(request);
    if (!reply.transportSucceeded() || reply.replyCode != 250) {
        result.reasonCode = "live_provider_status_failed";
        return result;
    }
    std::string receiver;
    std::string channel;
    if (!keyValue(reply.payload, "state", result.state) ||
        !keyValue(reply.payload, "reason", result.reasonCode) ||
        !keyValue(reply.payload, "receiverAttached", receiver) ||
        !parseBooleanToken(receiver, result.receiverAttached) ||
        !keyValue(reply.payload, "channelId", channel) ||
        channel != preparation.pin.channelFence.channelId) {
        result.current = false;
        result.reasonCode = "live_provider_status_invalid";
        return result;
    }
    result.current = result.state == "active" && result.receiverAttached;
    if (result.current) result.reasonCode = "live_provider_active";
    return result;
}

bool BackendAgentLiveProviderRuntime::close(
    const BackendAgentLiveProviderPreparation& preparation,
    const std::string& leaseId,
    std::string& reasonCode) const
{
    if (!preparation.pin.valid || leaseId.empty()) {
        reasonCode = "live_provider_close_invalid";
        return false;
    }
    SuiteBridgeLiveSourceLeaseRequest request{
        leaseId, preparation.pin.providerSelection.providerInstanceEpoch};
    const auto reply = transport_.closeLiveSource(request);
    if (!reply.transportSucceeded()) {
        reasonCode = "live_provider_close_transport_failed";
        return false;
    }
    // A restarted plugin necessarily has no receiver from the fenced epoch.
    // Treat stale-epoch close as terminal cleanup evidence, not permission to
    // open on the replacement epoch.
    if (reply.replyCode == 250 || reply.replyCode == 555) {
        reasonCode = reply.replyCode == 250
            ? "live_provider_closed"
            : "live_provider_epoch_replaced";
        return true;
    }
    reasonCode = "live_provider_close_failed";
    return false;
}

}

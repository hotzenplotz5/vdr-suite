#include "SuiteBridgeSvdrpTransport.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace vdrsuite::agent
{
namespace
{
constexpr const char* CapabilityProtocol = "vdr-suite-ntdel-cap/1";
constexpr const char* ResultProtocol = "vdr-suite-ntdel-result/1";
constexpr const char* SideEffectClass = "timer-delete";
constexpr const char* DisabledState = "disabled";
constexpr std::uint64_t ProviderGeneration = 1;
constexpr std::uint64_t CapabilityRevision = 1;
constexpr int CapabilityReplyCode = 900;
constexpr int DisabledReplyCode = 556;
constexpr int StaleReplyCode = 555;

std::vector<std::string> split(const std::string& input, std::size_t maximum)
{
    std::vector<std::string> values;
    std::size_t position = 0;
    while (position < input.size())
    {
        while (position < input.size() && input[position] == ' ') ++position;
        if (position == input.size()) break;
        const std::size_t end = input.find(' ', position);
        values.push_back(input.substr(
            position,
            end == std::string::npos ? std::string::npos : end - position));
        if (values.size() > maximum) return {};
        if (end == std::string::npos) break;
        position = end + 1;
    }
    return values;
}

bool unsignedValue(const std::string& value, std::uint64_t& parsed)
{
    if (value.empty() || value.size() > 19) return false;
    parsed = 0;
    for (unsigned char character : value)
    {
        if (character < '0' || character > '9') return false;
        const unsigned digit = static_cast<unsigned>(character - '0');
        if (parsed >
            (static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) - digit) / 10U)
        {
            return false;
        }
        parsed = parsed * 10U + digit;
    }
    return parsed > 0;
}

bool safeWireToken(const std::string& value, std::size_t maximum = 512)
{
    return !value.empty() && value.size() <= maximum &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}

BackendAgentNativeTimerDeleteTransportReply rejected(std::string evidence)
{
    BackendAgentNativeTimerDeleteTransportReply reply;
    reply.disposition =
        BackendAgentNativeTimerDeleteTransportDisposition::rejectedWithoutEffect;
    reply.evidenceReference = std::move(evidence);
    return reply;
}

BackendAgentNativeTimerDeleteTransportReply unknown(std::string evidence)
{
    BackendAgentNativeTimerDeleteTransportReply reply;
    reply.disposition =
        BackendAgentNativeTimerDeleteTransportDisposition::outcomeUnknown;
    reply.evidenceReference = std::move(evidence);
    return reply;
}

bool safeRequest(const BackendAgentNativeTimerDeleteTransportRequest& request)
{
    std::string reason;
    const auto& command = request.command;
    const auto& selection = command.localProviderSelection;
    return backendAgentNativeTimerDeleteValidCommand(command, reason) &&
        request.localStartingPersistedAt >= command.controlPlaneClaimedAt &&
        safeWireToken(command.commandId, 192) &&
        safeWireToken(command.requestFingerprint) &&
        safeWireToken(command.operationId, 192) &&
        safeWireToken(command.operationRevision, 192) &&
        safeWireToken(command.nativeTimerBindingId, 192) &&
        safeWireToken(command.expectedBindingRevision, 192) &&
        safeWireToken(command.timerAssignmentId, 192) &&
        safeWireToken(command.backendNativeTimerId, 192) &&
        safeWireToken(command.jobId, 192) &&
        safeWireToken(command.attemptId, 192) &&
        safeWireToken(command.backendId, 192) &&
        safeWireToken(command.agentId, 192) &&
        safeWireToken(command.agentInstanceId, 192) &&
        safeWireToken(selection.authorityDomain, 192) &&
        safeWireToken(selection.providerId, 192) &&
        safeWireToken(selection.providerKind, 192) &&
        safeWireToken(selection.providerInstanceEpoch, 192) &&
        safeWireToken(selection.requiredCapability, 192);
}
}

bool SuiteBridgeSvdrpTransport::discoverProvider(
    BackendAgentLocalProviderFacts& facts,
    std::string& reasonCode)
{
    facts = {};
    const SuiteBridgeCommandReply reply = executeRequest(
        "PLUG suitebridge NTDEL CAP 1\r\n");
    if (!reply.transportSucceeded() || reply.replyCode != CapabilityReplyCode)
    {
        reasonCode = "native_timer_delete_suitebridge_capability_unavailable";
        return false;
    }

    const std::vector<std::string> values = split(reply.payload, 10);
    std::uint64_t providerGeneration = 0;
    std::uint64_t capabilityRevision = 0;
    if (values.size() != 10 || values[0] != CapabilityProtocol ||
        values[1] != kBackendAgentNativeTimerDeleteCapability ||
        values[2] != "1" || values[3] != SideEffectClass ||
        values[4] != DisabledState ||
        values[5] != kBackendAgentNativeTimerDeleteProviderKind ||
        !safeWireToken(values[6], 192) ||
        !unsignedValue(values[7], providerGeneration) ||
        !unsignedValue(values[8], capabilityRevision) ||
        values[9] != DisabledState ||
        providerGeneration != ProviderGeneration ||
        capabilityRevision != CapabilityRevision)
    {
        reasonCode = "native_timer_delete_suitebridge_capability_invalid";
        return false;
    }

    facts.providerId = kBackendAgentNativeTimerDeleteProviderId;
    facts.providerKind = kBackendAgentNativeTimerDeleteProviderKind;
    facts.providerInstanceEpoch = values[6];
    facts.providerGeneration = providerGeneration;
    facts.capabilityRevision = capabilityRevision;
    facts.available = true;
    facts.capabilities = {kBackendAgentNativeTimerDeleteCapability};
    if (!backendAgentLocalProviderValidFacts(facts))
    {
        facts = {};
        reasonCode = "native_timer_delete_suitebridge_provider_facts_invalid";
        return false;
    }

    reasonCode = "native_timer_delete_suitebridge_provider_discovered_disabled";
    return true;
}

BackendAgentNativeTimerDeleteTransportReply SuiteBridgeSvdrpTransport::deleteTimer(
    const BackendAgentNativeTimerDeleteTransportRequest& request)
{
    if (!safeRequest(request))
    {
        return rejected("suitebridge:ntdel:local-request-invalid");
    }

    const auto& command = request.command;
    const auto& selection = command.localProviderSelection;
    std::ostringstream wire;
    wire << "PLUG suitebridge NTDEL EXEC vdr-suite-native/1 "
         << kBackendAgentNativeTimerDeleteCapability << " 1 "
         << command.commandId << ' ' << command.requestFingerprint << ' '
         << command.operationId << ' ' << command.operationRevision << ' '
         << command.nativeTimerBindingId << ' '
         << command.expectedBindingRevision << ' '
         << command.timerAssignmentId << ' ' << command.backendNativeTimerId << ' '
         << command.jobId << ' ' << command.attemptId << ' '
         << command.claimEpoch << ' ' << command.backendId << ' '
         << command.agentId << ' ' << command.agentInstanceId << ' '
         << command.backendGeneration << ' ' << command.controlPlaneClaimedAt << ' '
         << selection.authorityDomain << ' ' << selection.providerId << ' '
         << selection.providerKind << ' ' << selection.ownershipGeneration << ' '
         << selection.providerInstanceEpoch << ' ' << selection.providerGeneration << ' '
         << selection.capabilityRevision << ' ' << selection.requiredCapability << ' '
         << request.localStartingPersistedAt << "\r\n";

    const SuiteBridgeCommandReply reply = executeRequest(wire.str());
    if (!reply.transportSucceeded())
    {
        return unknown("suitebridge:ntdel:transport-outcome-unknown");
    }

    const std::vector<std::string> values = split(reply.payload, 11);
    std::uint64_t providerGeneration = 0;
    std::uint64_t capabilityRevision = 0;
    const bool typedRejection =
        values.size() == 11 && values[0] == ResultProtocol &&
        values[1] == command.commandId &&
        values[2] == command.requestFingerprint &&
        values[3] == kBackendAgentNativeTimerDeleteCapability &&
        values[4] == "1" && safeWireToken(values[5], 192) &&
        unsignedValue(values[6], providerGeneration) &&
        unsignedValue(values[7], capabilityRevision) &&
        values[8] == "rejected_without_effect" &&
        values[9] == DisabledState && safeWireToken(values[10]);
    if (!typedRejection ||
        (reply.replyCode != DisabledReplyCode && reply.replyCode != StaleReplyCode))
    {
        return unknown("suitebridge:ntdel:reply-outcome-unknown");
    }

    if (reply.replyCode == DisabledReplyCode &&
        (values[5] != selection.providerInstanceEpoch ||
         providerGeneration != selection.providerGeneration ||
         capabilityRevision != selection.capabilityRevision))
    {
        return unknown("suitebridge:ntdel:reply-fence-mismatch");
    }

    return rejected(values[10]);
}

}

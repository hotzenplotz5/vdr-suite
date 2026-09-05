#include "SuiteBridgeRecordingMarksModifyTransport.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace vdrsuite::agent
{
namespace
{
constexpr const char* CapabilityProtocol = "vdr-suite-nmarks-cap/2";
constexpr const char* ResultProtocol = "vdr-suite-nmarks-result/2";
constexpr const char* SideEffectClass = "recording-marks-modify";
constexpr const char* DisabledState = "disabled";
constexpr const char* EnabledState = "enabled";
constexpr std::uint64_t ProviderGeneration = 1;
constexpr std::uint64_t CapabilityRevision = 2;
constexpr int CapabilityReplyCode = 900;
constexpr int StaleReplyCode = 555;
constexpr int RejectedReplyCode = 556;
constexpr int AcceptedUnverifiedReplyCode = 557;
constexpr int OutcomeUnknownReplyCode = 558;
constexpr int ReplayConflictReplyCode = 559;
constexpr int ReplayLedgerFullReplyCode = 560;

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
    for (const unsigned char character : value)
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

bool safeFingerprint(const std::string& value)
{
    return value.size() == 71 && value.compare(0, 7, "sha256:") == 0 &&
        std::all_of(value.begin() + 7, value.end(), [](unsigned char character) {
            return (character >= '0' && character <= '9') ||
                (character >= 'a' && character <= 'f');
        });
}

std::string frameToken(int frame)
{
    return frame < 0 ? "-" : std::to_string(frame);
}

std::string replacementFramesToken(const std::vector<int>& frames)
{
    if (frames.empty()) return "-";
    std::ostringstream out;
    for (std::size_t index = 0; index < frames.size(); ++index)
    {
        if (index != 0) out << ',';
        out << frames[index];
    }
    return out.str();
}

BackendAgentRecordingMarksModifyTransportReply rejected(std::string evidence)
{
    BackendAgentRecordingMarksModifyTransportReply reply;
    reply.disposition =
        BackendAgentRecordingMarksModifyTransportDisposition::rejectedWithoutEffect;
    reply.evidenceReference = std::move(evidence);
    return reply;
}

BackendAgentRecordingMarksModifyTransportReply accepted(std::string evidence)
{
    BackendAgentRecordingMarksModifyTransportReply reply;
    reply.disposition =
        BackendAgentRecordingMarksModifyTransportDisposition::acceptedUnverified;
    reply.evidenceReference = std::move(evidence);
    return reply;
}

BackendAgentRecordingMarksModifyTransportReply unknown(std::string evidence)
{
    BackendAgentRecordingMarksModifyTransportReply reply;
    reply.disposition =
        BackendAgentRecordingMarksModifyTransportDisposition::outcomeUnknown;
    reply.evidenceReference = std::move(evidence);
    return reply;
}

bool safeRequest(const BackendAgentRecordingMarksModifyTransportRequest& request)
{
    std::string reason;
    const auto& command = request.command;
    const auto& selection = command.localProviderSelection;
    return backendAgentRecordingMarksModifyValidCommand(command, reason) &&
        request.localStartingPersistedAt > 0 &&
        request.localStartingPersistedAt >= command.controlPlaneClaimedAt &&
        safeWireToken(command.commandId, 192) &&
        safeFingerprint(command.requestFingerprint) &&
        safeWireToken(command.operationId, 192) &&
        safeWireToken(command.operationRevision, 192) &&
        safeWireToken(command.recordingKey, 32) &&
        safeWireToken(command.expectedMarksRevision, 32) &&
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

SuiteBridgeCommandReply SuiteBridgeSvdrpTransport::discoverRecordingMarksModifyContract()
{
    return executeRequest("PLUG suitebridge NMARKS CAP 2 modify\r\n");
}

SuiteBridgeCommandReply SuiteBridgeSvdrpTransport::executeRecordingMarksModifyContract(
    const BackendAgentRecordingMarksModifyTransportRequest& request)
{
    const auto& command = request.command;
    const auto& selection = command.localProviderSelection;
    std::ostringstream wire;
    wire << "PLUG suitebridge NMARKS EXEC vdr-suite-native/1 "
         << kBackendAgentRecordingMarksModifyCapability << " 2 "
         << command.commandId << ' ' << command.requestFingerprint << ' '
         << command.operationId << ' ' << command.operationRevision << ' '
         << command.recordingKey << ' ' << command.expectedMarksRevision << ' '
         << backendAgentRecordingMarksModifyKindName(command.kind) << ' '
         << frameToken(command.sourceFrame) << ' '
         << frameToken(command.targetFrame) << ' '
         << replacementFramesToken(command.replacementFrames) << ' '
         << command.jobId << ' ' << command.attemptId << ' '
         << command.claimEpoch << ' ' << command.backendId << ' '
         << command.agentId << ' ' << command.agentInstanceId << ' '
         << command.backendGeneration << ' ' << command.controlPlaneClaimedAt << ' '
         << selection.authorityDomain << ' ' << selection.providerId << ' '
         << selection.providerKind << ' ' << selection.ownershipGeneration << ' '
         << selection.providerInstanceEpoch << ' ' << selection.providerGeneration << ' '
         << selection.capabilityRevision << ' ' << selection.requiredCapability << ' '
         << request.localStartingPersistedAt << "\r\n";
    return executeRequest(wire.str());
}

SuiteBridgeRecordingMarksModifyTransport::SuiteBridgeRecordingMarksModifyTransport(
    SuiteBridgeSvdrpTransportConfig config)
    : transport_(std::move(config))
{
}

bool SuiteBridgeRecordingMarksModifyTransport::discoverProvider(
    BackendAgentLocalProviderFacts& facts,
    std::string& reasonCode)
{
    facts = {};
    const SuiteBridgeCommandReply reply =
        transport_.discoverRecordingMarksModifyContract();
    if (!reply.transportSucceeded() || reply.replyCode != CapabilityReplyCode)
    {
        reasonCode = "recording_marks_modify_suitebridge_capability_unavailable";
        return false;
    }

    const std::vector<std::string> values = split(reply.payload, 10);
    std::uint64_t providerGeneration = 0;
    std::uint64_t capabilityRevision = 0;
    const bool stateValid = values.size() == 10 &&
        (values[4] == DisabledState || values[4] == EnabledState) &&
        values[9] == values[4];
    if (!stateValid || values[0] != CapabilityProtocol ||
        values[1] != kBackendAgentRecordingMarksModifyCapability ||
        values[2] != "2" || values[3] != SideEffectClass ||
        values[5] != kBackendAgentRecordingMarksModifyProviderKind ||
        !safeWireToken(values[6], 192) ||
        !unsignedValue(values[7], providerGeneration) ||
        !unsignedValue(values[8], capabilityRevision) ||
        providerGeneration != ProviderGeneration ||
        capabilityRevision != CapabilityRevision)
    {
        reasonCode = "recording_marks_modify_suitebridge_capability_invalid";
        return false;
    }

    facts.providerId = kBackendAgentRecordingMarksModifyProviderId;
    facts.providerKind = kBackendAgentRecordingMarksModifyProviderKind;
    facts.providerInstanceEpoch = values[6];
    facts.providerGeneration = providerGeneration;
    facts.capabilityRevision = capabilityRevision;
    facts.available = values[4] == EnabledState;
    facts.capabilities = {kBackendAgentRecordingMarksModifyCapability};
    if (!backendAgentLocalProviderValidFacts(facts))
    {
        facts = {};
        reasonCode = "recording_marks_modify_suitebridge_provider_facts_invalid";
        return false;
    }

    reasonCode = values[4] == EnabledState
        ? "recording_marks_modify_suitebridge_provider_discovered_enabled"
        : "recording_marks_modify_suitebridge_provider_discovered_disabled";
    return true;
}

BackendAgentRecordingMarksModifyTransportReply
SuiteBridgeRecordingMarksModifyTransport::modifyMarks(
    const BackendAgentRecordingMarksModifyTransportRequest& request)
{
    if (!safeRequest(request))
        return rejected("suitebridge:nmarks:local-request-invalid");

    const auto& command = request.command;
    const auto& selection = command.localProviderSelection;
    const SuiteBridgeCommandReply reply =
        transport_.executeRecordingMarksModifyContract(request);
    if (!reply.transportSucceeded())
        return unknown("suitebridge:nmarks:transport-outcome-unknown");

    const std::vector<std::string> values = split(reply.payload, 11);
    std::uint64_t providerGeneration = 0;
    std::uint64_t capabilityRevision = 0;
    const bool typedResult =
        values.size() == 11 && values[0] == ResultProtocol &&
        values[1] == command.commandId &&
        values[2] == command.requestFingerprint &&
        values[3] == kBackendAgentRecordingMarksModifyCapability &&
        values[4] == "2" && safeWireToken(values[5], 192) &&
        unsignedValue(values[6], providerGeneration) &&
        unsignedValue(values[7], capabilityRevision) &&
        safeWireToken(values[8], 64) && safeWireToken(values[9], 64) &&
        safeWireToken(values[10]);
    if (!typedResult)
        return unknown("suitebridge:nmarks:reply-outcome-unknown");

    const bool staleReply = reply.replyCode == StaleReplyCode;
    if (!staleReply &&
        (values[5] != selection.providerInstanceEpoch ||
         providerGeneration != selection.providerGeneration ||
         capabilityRevision != selection.capabilityRevision))
    {
        return unknown("suitebridge:nmarks:reply-fence-mismatch");
    }

    if (reply.replyCode == AcceptedUnverifiedReplyCode &&
        values[8] == "accepted_unverified")
    {
        return accepted(values[10]);
    }

    if (reply.replyCode == OutcomeUnknownReplyCode &&
        values[8] == "outcome_unknown")
    {
        return unknown(values[10]);
    }

    if ((reply.replyCode == StaleReplyCode ||
         reply.replyCode == RejectedReplyCode ||
         reply.replyCode == ReplayConflictReplyCode ||
         reply.replyCode == ReplayLedgerFullReplyCode) &&
        values[8] == "rejected_without_effect")
    {
        return rejected(values[10]);
    }

    return unknown("suitebridge:nmarks:reply-outcome-unknown");
}

}

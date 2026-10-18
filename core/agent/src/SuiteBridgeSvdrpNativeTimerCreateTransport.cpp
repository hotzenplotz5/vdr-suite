#include "SuiteBridgeNativeTimerCreateTransport.h"

#include "BackendAgentCommand.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace vdrsuite::agent
{
namespace
{

constexpr const char* CapabilityProtocol = "vdr-suite-ntcreate-cap/1";
constexpr const char* ResultProtocol = "vdr-suite-ntcreate-result/1";
constexpr const char* SideEffectClass = "timer-create";
constexpr const char* DisabledState = "disabled";
constexpr const char* EnabledState = "enabled";

constexpr std::uint64_t ProviderGeneration = 1;
constexpr std::uint64_t CapabilityRevision = 1;

constexpr int CapabilityReplyCode = 900;
constexpr int StaleReplyCode = 555;
constexpr int RejectedReplyCode = 556;
constexpr int AcceptedUnverifiedReplyCode = 557;
constexpr int OutcomeUnknownReplyCode = 558;
constexpr int ReplayConflictReplyCode = 559;
constexpr int ReplayLedgerFullReplyCode = 560;

std::vector<std::string> split(
    const std::string& input,
    std::size_t maximum)
{
    std::vector<std::string> values;
    std::size_t position = 0;

    while (position < input.size())
    {
        while (position < input.size() && input[position] == ' ')
            ++position;

        if (position == input.size())
            break;

        const std::size_t end = input.find(' ', position);

        values.push_back(input.substr(
            position,
            end == std::string::npos
                ? std::string::npos
                : end - position));

        if (values.size() > maximum)
            return {};

        if (end == std::string::npos)
            break;

        position = end + 1;
    }

    return values;
}

bool unsignedValue(
    const std::string& value,
    std::uint64_t& parsed)
{
    if (value.empty() || value.size() > 19)
        return false;

    parsed = 0;

    for (unsigned char character : value)
    {
        if (character < '0' || character > '9')
            return false;

        const unsigned digit =
            static_cast<unsigned>(character - '0');

        if (parsed >
            (static_cast<std::uint64_t>(
                 std::numeric_limits<std::int64_t>::max()) - digit) /
                10U)
        {
            return false;
        }

        parsed = parsed * 10U + digit;
    }

    return parsed > 0;
}

bool signedOffset(const std::string& value, std::int32_t& parsed)
{
    if (value.empty() || value.size() > 6) return false;
    try
    {
        std::size_t used = 0;
        const long offset = std::stol(value, &used, 10);
        if (used != value.size() || offset < -14 * 3600 ||
            offset > 14 * 3600 || offset % 60 != 0)
            return false;
        parsed = static_cast<std::int32_t>(offset);
        return true;
    }
    catch (...) { return false; }
}

bool matchesLocalProjection(
    std::int64_t unixSeconds, std::int32_t offset,
    const std::string& day, const std::string& hhmm)
{
    if (day.size() != 10 || hhmm.size() != 4 ||
        unixSeconds <= 0 || unixSeconds >
            std::numeric_limits<std::int64_t>::max() - 14 * 3600)
        return false;
    const std::int64_t local = unixSeconds + offset;
    const auto timestamp = static_cast<std::time_t>(local);
    if (static_cast<std::int64_t>(timestamp) != local) return false;
    std::tm parts{};
    if (!gmtime_r(&timestamp, &parts)) return false;
    char expectedDay[16]{};
    char expectedTime[8]{};
    return std::snprintf(expectedDay, sizeof(expectedDay), "%04d-%02d-%02d",
               parts.tm_year + 1900, parts.tm_mon + 1, parts.tm_mday) == 10 &&
        std::snprintf(expectedTime, sizeof(expectedTime), "%02d%02d",
               parts.tm_hour, parts.tm_min) == 4 &&
        day == expectedDay && hhmm == expectedTime;
}

bool safeWireToken(
    const std::string& value,
    std::size_t maximum = 512)
{
    return !value.empty() &&
        value.size() <= maximum &&
        std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character) {
                return std::isalnum(character) != 0 ||
                    character == '-' ||
                    character == '_' ||
                    character == '.' ||
                    character == ':';
            });
}

std::string hexToken(const std::string& input)
{
    if (input.empty())
        return "-";

    static constexpr char Digits[] = "0123456789abcdef";

    std::string output;
    output.reserve(input.size() * 2U);

    for (unsigned char character : input)
    {
        output.push_back(Digits[(character >> 4U) & 0x0fU]);
        output.push_back(Digits[character & 0x0fU]);
    }

    return output;
}

BackendAgentNativeTimerCreateTransportReply rejected(
    std::string evidence)
{
    BackendAgentNativeTimerCreateTransportReply reply;
    reply.disposition =
        BackendAgentNativeTimerCreateTransportDisposition::
            rejectedWithoutEffect;
    reply.evidenceReference = std::move(evidence);
    return reply;
}

BackendAgentNativeTimerCreateTransportReply accepted(
    std::string evidence)
{
    BackendAgentNativeTimerCreateTransportReply reply;
    reply.disposition =
        BackendAgentNativeTimerCreateTransportDisposition::
            acceptedUnverified;
    reply.evidenceReference = std::move(evidence);
    return reply;
}

BackendAgentNativeTimerCreateTransportReply unknown(
    std::string evidence)
{
    BackendAgentNativeTimerCreateTransportReply reply;
    reply.disposition =
        BackendAgentNativeTimerCreateTransportDisposition::
            outcomeUnknown;
    reply.evidenceReference = std::move(evidence);
    return reply;
}

bool safeRequest(
    const BackendAgentNativeTimerCreateTransportRequest& request)
{
    std::string reason;
    const auto& command = request.command;
    const auto& selection = command.localProviderSelection;

    return
        backendAgentNativeTimerCreateValidCommand(command, reason) &&
        request.localStartingPersistedAt >=
            command.controlPlaneClaimedAt &&
        safeWireToken(command.commandId, 192) &&
        safeWireToken(command.requestFingerprint, 192) &&
        safeWireToken(command.operationId, 192) &&
        safeWireToken(command.operationRevision, 192) &&
        safeWireToken(command.timerAssignmentId, 192) &&
        safeWireToken(command.expectedAssignmentRevision, 192) &&
        safeWireToken(command.expectedIntentRevision, 192) &&
        safeWireToken(command.nativeTimerBindingId, 192) &&
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

} // namespace

SuiteBridgeCommandReply
SuiteBridgeSvdrpTransport::discoverNativeTimerCreateContract()
{
    return executeRequest(
        "PLUG suitebridge NTCREATE CAP 1\r\n");
}

SuiteBridgeCommandReply
SuiteBridgeSvdrpTransport::inspectNativeTimerClockContract(
    std::int64_t start, std::int64_t stop)
{
    if (start <= 0 || stop <= start || stop - start > 48 * 3600)
        return {SuiteBridgeTransportStatus::Failed, 0, {},
            "invalid native timer clock request"};
    std::ostringstream wire;
    wire << "PLUG suitebridge NTCREATE CLOCK 1 "
         << start << ' ' << stop << "\r\n";
    return executeRequest(wire.str());
}

SuiteBridgeCommandReply
SuiteBridgeSvdrpTransport::executeNativeTimerCreateContract(
    const BackendAgentNativeTimerCreateTransportRequest& request)
{
    const auto& command = request.command;
    const auto& selection = command.localProviderSelection;
    const auto& specification = command.specification;

    std::ostringstream wire;

    wire
        << "PLUG suitebridge NTCREATE EXEC vdr-suite-native/1 "
        << kBackendAgentNativeTimerCreateCapability << " 1 "
        << command.commandId << ' '
        << command.requestFingerprint << ' '
        << command.operationId << ' '
        << command.operationRevision << ' '
        << command.timerAssignmentId << ' '
        << command.expectedAssignmentRevision << ' '
        << command.expectedIntentRevision << ' '
        << command.assignmentEpoch << ' '
        << command.nativeTimerBindingId << ' '
        << hexToken(command.expectedSpecificationFingerprint) << ' '
        << command.jobId << ' '
        << command.attemptId << ' '
        << command.claimEpoch << ' '
        << command.backendId << ' '
        << command.agentId << ' '
        << command.agentInstanceId << ' '
        << command.backendGeneration << ' '
        << command.controlPlaneClaimedAt << ' '
        << selection.authorityDomain << ' '
        << selection.providerId << ' '
        << selection.providerKind << ' '
        << selection.ownershipGeneration << ' '
        << selection.providerInstanceEpoch << ' '
        << selection.providerGeneration << ' '
        << selection.capabilityRevision << ' '
        << selection.requiredCapability << ' '
        << request.localStartingPersistedAt << ' '
        << hexToken(specification.channelId) << ' '
        << hexToken(specification.title) << ' '
        << hexToken(specification.directory) << ' '
        << hexToken(specification.day) << ' '
        << hexToken(specification.weekdays) << ' '
        << hexToken(specification.startTime) << ' '
        << hexToken(specification.endTime) << ' '
        << specification.priority << ' '
        << specification.lifetime << ' '
        << (specification.enabled ? 1 : 0) << ' '
        << (specification.vps ? 1 : 0)
        << "\r\n";

    return executeRequest(wire.str());
}

SuiteBridgeNativeTimerCreateTransport::
SuiteBridgeNativeTimerCreateTransport(
    SuiteBridgeSvdrpTransportConfig config)
    : transport_(std::move(config))
{
}

bool SuiteBridgeNativeTimerCreateTransport::discoverProvider(
    BackendAgentLocalProviderFacts& facts,
    std::string& reasonCode)
{
    facts = {};

    const SuiteBridgeCommandReply reply =
        transport_.discoverNativeTimerCreateContract();

    if (!reply.transportSucceeded() ||
        reply.replyCode != CapabilityReplyCode)
    {
        reasonCode =
            "native_timer_create_suitebridge_capability_unavailable";
        return false;
    }

    const std::vector<std::string> values =
        split(reply.payload, 10);

    std::uint64_t providerGeneration = 0;
    std::uint64_t capabilityRevision = 0;

    const bool stateValid =
        values.size() == 10 &&
        (values[4] == DisabledState ||
         values[4] == EnabledState) &&
        values[9] == values[4];

    if (!stateValid ||
        values[0] != CapabilityProtocol ||
        values[1] != kBackendAgentNativeTimerCreateCapability ||
        values[2] != "1" ||
        values[3] != SideEffectClass ||
        values[5] != kBackendAgentNativeTimerCreateProviderKind ||
        !safeWireToken(values[6], 192) ||
        !unsignedValue(values[7], providerGeneration) ||
        !unsignedValue(values[8], capabilityRevision) ||
        providerGeneration != ProviderGeneration ||
        capabilityRevision != CapabilityRevision)
    {
        reasonCode =
            "native_timer_create_suitebridge_capability_invalid";
        return false;
    }

    facts.providerId =
        kBackendAgentNativeTimerCreateProviderId;
    facts.providerKind =
        kBackendAgentNativeTimerCreateProviderKind;
    facts.providerInstanceEpoch = values[6];
    facts.providerGeneration = providerGeneration;
    facts.capabilityRevision = capabilityRevision;
    facts.available = values[4] == EnabledState;
    facts.capabilities = {
        kBackendAgentNativeTimerCreateCapability
    };

    if (!backendAgentLocalProviderValidFacts(facts))
    {
        facts = {};
        reasonCode =
            "native_timer_create_suitebridge_provider_facts_invalid";
        return false;
    }

    reasonCode =
        values[4] == EnabledState
            ? "native_timer_create_suitebridge_provider_discovered_enabled"
            : "native_timer_create_suitebridge_provider_discovered_disabled";

    return true;
}

bool SuiteBridgeNativeTimerCreateTransport::inspectClock(
    std::int64_t start, std::int64_t stop,
    const BackendAgentLocalProviderSelection& authorizedSelection,
    SuiteBridgeNativeTimerClockProjection& projection,
    std::string& reasonCode)
{
    if (start <= 0 || stop <= start || stop - start > 48 * 3600)
    {
        projection = {};
        reasonCode = "native_timer_clock_request_invalid";
        return false;
    }
    return verifyNativeClockReply(
        transport_.inspectNativeTimerClockContract(start, stop), start, stop,
        authorizedSelection, projection, reasonCode);
}

bool SuiteBridgeNativeTimerCreateTransport::verifyNativeClockReply(
    const SuiteBridgeCommandReply& reply,
    std::int64_t start, std::int64_t stop,
    const BackendAgentLocalProviderSelection& authorizedSelection,
    SuiteBridgeNativeTimerClockProjection& projection,
    std::string& reasonCode)
{
    projection = {};
    if (start <= 0 || stop <= start || stop - start > 48 * 3600 ||
        !backendAgentLocalProviderValidSelection(authorizedSelection) ||
        authorizedSelection.authorityDomain !=
            kBackendAgentNativeTimerCreateAuthorityDomain ||
        authorizedSelection.providerId !=
            kBackendAgentNativeTimerCreateProviderId ||
        authorizedSelection.providerKind !=
            kBackendAgentNativeTimerCreateProviderKind ||
        authorizedSelection.requiredCapability !=
            kBackendAgentNativeTimerCreateCapability)
    {
        reasonCode = "native_timer_clock_request_invalid";
        return false;
    }
    if (!reply.transportSucceeded() || reply.replyCode != CapabilityReplyCode)
    {
        reasonCode = "native_timer_clock_unavailable";
        return false;
    }
    const auto values = split(reply.payload, 15);
    std::uint64_t providerGeneration = 0, capabilityRevision = 0;
    std::uint64_t echoedStart = 0, echoedStop = 0;
    std::int32_t startOffset = 0, stopOffset = 0;
    if (values.size() != 15 ||
        values[0] != "vdr-suite-ntcreate-clock/1" ||
        values[1] != kBackendAgentNativeTimerCreateCapability ||
        values[2] != "1" ||
        values[3] != kBackendAgentNativeTimerCreateProviderKind ||
        values[4] != authorizedSelection.providerInstanceEpoch ||
        !unsignedValue(values[5], providerGeneration) ||
        !unsignedValue(values[6], capabilityRevision) ||
        providerGeneration != authorizedSelection.providerGeneration ||
        capabilityRevision != authorizedSelection.capabilityRevision ||
        !unsignedValue(values[7], echoedStart) ||
        !unsignedValue(values[8], echoedStop) ||
        echoedStart != static_cast<std::uint64_t>(start) ||
        echoedStop != static_cast<std::uint64_t>(stop) ||
        !signedOffset(values[9], startOffset) ||
        !signedOffset(values[10], stopOffset) ||
        !matchesLocalProjection(start, startOffset, values[11], values[12]) ||
        !matchesLocalProjection(stop, stopOffset, values[13], values[14]))
    {
        reasonCode = "native_timer_clock_reply_invalid_or_stale";
        return false;
    }
    projection.start = start;
    projection.stop = stop;
    projection.startUtcOffsetSeconds = startOffset;
    projection.stopUtcOffsetSeconds = stopOffset;
    projection.startDay = values[11];
    projection.startHhmm = values[12];
    projection.stopDay = values[13];
    projection.stopHhmm = values[14];
    projection.providerInstanceEpoch = values[4];
    reasonCode = "native_timer_clock_verified";
    return true;
}

BackendAgentNativeTimerCreateTransportReply
SuiteBridgeNativeTimerCreateTransport::createTimer(
    const BackendAgentNativeTimerCreateTransportRequest& request)
{
    if (!safeRequest(request))
    {
        return rejected(
            "suitebridge:ntcreate:local-request-invalid");
    }

    const auto& command = request.command;
    const auto& selection = command.localProviderSelection;

    const SuiteBridgeCommandReply reply =
        transport_.executeNativeTimerCreateContract(request);

    if (!reply.transportSucceeded())
    {
        return unknown(
            "suitebridge:ntcreate:transport-outcome-unknown");
    }

    const std::vector<std::string> values =
        split(reply.payload, 11);

    std::uint64_t providerGeneration = 0;
    std::uint64_t capabilityRevision = 0;

    const bool typedResult =
        values.size() == 11 &&
        values[0] == ResultProtocol &&
        values[1] == command.commandId &&
        values[2] == command.requestFingerprint &&
        values[3] == kBackendAgentNativeTimerCreateCapability &&
        values[4] == "1" &&
        safeWireToken(values[5], 192) &&
        unsignedValue(values[6], providerGeneration) &&
        unsignedValue(values[7], capabilityRevision) &&
        safeWireToken(values[8], 64) &&
        safeWireToken(values[9], 64) &&
        safeWireToken(values[10], 1024);

    if (!typedResult)
    {
        return unknown(
            "suitebridge:ntcreate:reply-outcome-unknown");
    }

    const bool staleReply =
        reply.replyCode == StaleReplyCode;

    if (!staleReply &&
        (values[5] != selection.providerInstanceEpoch ||
         providerGeneration != selection.providerGeneration ||
         capabilityRevision != selection.capabilityRevision))
    {
        return unknown(
            "suitebridge:ntcreate:reply-fence-mismatch");
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

    return unknown(
        "suitebridge:ntcreate:reply-outcome-unknown");
}

} // namespace vdrsuite::agent

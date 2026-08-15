#include "BackendAgentCommandDelivery.h"
#include "BackendAgentCommandJson.h"
#include "BackendAgentNativeTimerDelete.h"
#include "BackendAgentNativeTimerDeleteAdvertisement.h"
#include "BackendAgentNativeTimerDeletePayload.h"
#include "Database.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
using vdrsuite::agent::BackendAgentLocalProviderFacts;
using vdrsuite::agent::BackendAgentLocalProviderSelection;

BackendAgentLocalProviderFacts providerFacts(
    const std::string& epoch,
    std::uint64_t generation,
    std::uint64_t capabilityRevision,
    bool available = true,
    bool includeDelete = true,
    const std::string& providerId = "suitebridge:local",
    const std::string& providerKind = "suitebridge")
{
    BackendAgentLocalProviderFacts facts;
    facts.providerId = providerId;
    facts.providerKind = providerKind;
    facts.providerInstanceEpoch = epoch;
    facts.providerGeneration = generation;
    facts.capabilityRevision = capabilityRevision;
    facts.available = available;
    facts.capabilities = includeDelete
        ? std::vector<std::string>{"vdr.timer.delete"}
        : std::vector<std::string>{"vdr.native.probe"};
    return facts;
}

BackendAgentCommandPollRequest pollRequest(
    const std::vector<std::string>& commandTypes,
    const std::vector<BackendAgentLocalProviderFacts>& providers = {})
{
    BackendAgentCommandPollRequest poll;
    poll.backendId = "default";
    poll.agentInstanceId = "inst_timer_delivery";
    poll.backendGeneration = 7;
    poll.supportedCommandTypes = commandTypes;
    poll.localProviders = providers;
    return poll;
}

BackendAgentCommandAssignment timerDeleteAssignment(
    const std::string& suffix,
    const BackendAgentLocalProviderSelection& selection,
    std::int64_t assignedAt)
{
    using namespace vdrsuite::agent;
    BackendAgentNativeTimerDeletePayload payload;
    payload.operationRevision = "3";
    payload.nativeTimerBindingId = "ntb_timer_" + suffix;
    payload.expectedBindingRevision = "12";
    payload.timerAssignmentId = "ta_timer_" + suffix;
    payload.backendNativeTimerId = "native_timer_" + suffix;
    payload.controlPlaneClaimedAt = assignedAt - 1;
    payload.localProviderSelection = selection;

    BackendAgentCommandAssignment assignment;
    assignment.present = true;
    assignment.requestId = "req_timer_delivery_" + suffix;
    assignment.correlationId = "corr_timer_delivery_" + suffix;
    assignment.operationId = "op_timer_delivery_" + suffix;
    assignment.jobId = "job_timer_delivery_" + suffix;
    assignment.attemptId = "attempt_timer_delivery_" + suffix;
    assignment.claimEpoch = 1;
    assignment.commandId = "cmd_timer_delivery_" + suffix;
    assignment.backendId = "default";
    assignment.agentId = "agt_timer_delivery";
    assignment.agentInstanceId = "inst_timer_delivery";
    assignment.backendGeneration = 7;
    assignment.commandType = kBackendAgentNativeTimerDeleteCommandType;
    assignment.payloadVersion = kBackendAgentNativeTimerDeletePayloadVersion;
    assignment.payload = backendAgentNativeTimerDeletePayload(payload);
    assignment.verificationPolicy = "readback_required";
    assignment.assignedAt = assignedAt;
    assignment.deadline = assignedAt + 300;
    assignment.requestFingerprint = backendAgentCommandFingerprint(assignment);
    assert(backendAgentCommandValidAssignment(assignment));
    return assignment;
}

BackendAgentCommandReceipt receiptFor(
    const BackendAgentCommandAssignment& assignment,
    std::int64_t receivedAt)
{
    BackendAgentCommandReceipt receipt;
    receipt.commandId = assignment.commandId;
    receipt.requestFingerprint = assignment.requestFingerprint;
    receipt.jobId = assignment.jobId;
    receipt.attemptId = assignment.attemptId;
    receipt.claimEpoch = assignment.claimEpoch;
    receipt.backendId = assignment.backendId;
    receipt.agentId = assignment.agentId;
    receipt.agentInstanceId = assignment.agentInstanceId;
    receipt.backendGeneration = assignment.backendGeneration;
    receipt.receiptCategory = "accepted";
    receipt.receivedAt = receivedAt;
    receipt.reasonCode = "durably_recorded_without_execution";
    assert(backendAgentCommandValidReceipt(receipt));
    return receipt;
}

BackendAgentCommandResult nonDispatchResult(
    const BackendAgentCommandAssignment& assignment,
    std::int64_t completedAt)
{
    BackendAgentCommandResult result;
    result.commandId = assignment.commandId;
    result.requestFingerprint = assignment.requestFingerprint;
    result.jobId = assignment.jobId;
    result.attemptId = assignment.attemptId;
    result.claimEpoch = assignment.claimEpoch;
    result.backendId = assignment.backendId;
    result.agentId = assignment.agentId;
    result.agentInstanceId = assignment.agentInstanceId;
    result.backendGeneration = assignment.backendGeneration;
    result.dispatchState = "not_started";
    result.verificationState = "outcome_unknown";
    result.resultCategory = "rejected";
    result.errorCategory = "unsupported";
    result.retryClassification = "none";
    result.boundedDiagnostics =
        "Slice 26 has no local vdr.timer.delete executor";
    result.completedAt = completedAt;
    assert(backendAgentCommandValidResult(result));
    return result;
}

BackendAgentCommandAssignment noopAssignment(std::int64_t assignedAt)
{
    BackendAgentCommandAssignment assignment;
    assignment.present = true;
    assignment.requestId = "req_probe_delivery_regression";
    assignment.correlationId = "corr_probe_delivery_regression";
    assignment.operationId = "op_probe_delivery_regression";
    assignment.jobId = "job_probe_delivery_regression";
    assignment.attemptId = "attempt_probe_delivery_regression";
    assignment.claimEpoch = 1;
    assignment.commandId = "cmd_probe_delivery_regression";
    assignment.backendId = "default";
    assignment.agentId = "agt_timer_delivery";
    assignment.agentInstanceId = "inst_timer_delivery";
    assignment.backendGeneration = 7;
    assignment.commandType = "probe.noop";
    assignment.payloadVersion = 1;
    assignment.payload = "{}";
    assignment.verificationPolicy = "none";
    assignment.assignedAt = assignedAt;
    assignment.deadline = assignedAt + 300;
    assignment.requestFingerprint = backendAgentCommandFingerprint(assignment);
    assert(backendAgentCommandValidAssignment(assignment));
    return assignment;
}
}

int main()
{
    using namespace vdrsuite::agent;

    Database database;
    assert(database.open(":memory:"));
    BackendAgentCommandRepository commands(database);
    assert(commands.ensureSchema());

    BackendAgentLocalProviderOwnership ownership;
    std::string reason;
    assert(commands.setLocalProviderOwnership(
        "default", kBackendAgentNativeTimerDeleteAuthorityDomain,
        kBackendAgentNativeTimerDeleteProviderId,
        kBackendAgentNativeTimerDeleteProviderKind,
        {kBackendAgentNativeTimerDeleteCapability},
        100, ownership, reason));

    const auto firstFacts = providerFacts("pie_timer_delivery_1", 3, 4);
    const auto observe = commands.poll(
        pollRequest({"probe.noop"}, {firstFacts}),
        "agt_timer_delivery", 101);
    assert(observe.accepted);
    assert(!observe.assignment.present);

    const auto firstSelection = commands.selectLocalProvider(
        "default", "agt_timer_delivery", "inst_timer_delivery", 7,
        kBackendAgentNativeTimerDeleteAuthorityDomain,
        kBackendAgentNativeTimerDeleteCapability, reason);
    assert(firstSelection.has_value());

    const auto first = timerDeleteAssignment("1", *firstSelection, 110);
    assert(commands.insertAssignment(first, &*firstSelection));

    // JSON transport accepts the bounded command type and preserves the exact
    // provider facts. This does not grant provider ownership.
    const auto validPoll = pollRequest(
        {kBackendAgentNativeTimerDeleteCommandType}, {firstFacts});
    const std::string encoded = serializeBackendAgentCommandPollRequestJson(validPoll);
    BackendAgentCommandPollRequest parsed;
    assert(parseBackendAgentCommandPollRequestJson(encoded, parsed, reason));
    assert(parsed.supportedCommandTypes == validPoll.supportedCommandTypes);
    assert(parsed.localProviders.size() == 1);
    assert(parsed.localProviders.front().providerInstanceEpoch ==
           firstFacts.providerInstanceEpoch);

    auto duplicateType = validPoll;
    duplicateType.supportedCommandTypes.push_back(
        kBackendAgentNativeTimerDeleteCommandType);
    assert(!backendAgentNativeTimerDeleteAdvertisementValid(duplicateType, reason));
    assert(reason == "invalid_native_timer_delete_advertisement");

    const auto noProvider = commands.poll(
        pollRequest({kBackendAgentNativeTimerDeleteCommandType}),
        "agt_timer_delivery", 111);
    assert(!noProvider.accepted);
    assert(noProvider.reasonCode ==
           "native_timer_delete_provider_advertisement_required");

    const auto unavailableProvider = commands.poll(
        pollRequest(
            {kBackendAgentNativeTimerDeleteCommandType},
            {providerFacts("pie_timer_delivery_unavailable", 3, 4, false)}),
        "agt_timer_delivery", 112);
    assert(!unavailableProvider.accepted);

    const auto missingCapability = commands.poll(
        pollRequest(
            {kBackendAgentNativeTimerDeleteCommandType},
            {providerFacts("pie_timer_delivery_no_delete", 3, 4, true, false)}),
        "agt_timer_delivery", 113);
    assert(!missingCapability.accepted);

    const auto wrongProvider = commands.poll(
        pollRequest(
            {kBackendAgentNativeTimerDeleteCommandType},
            {providerFacts(
                "pie_timer_delivery_wrong", 3, 4, true, true,
                "restfulapi:local", "restfulapi")}),
        "agt_timer_delivery", 114);
    assert(!wrongProvider.accepted);

    // Recreate the Slice-25 dormant trigger to prove the delivery boundary
    // removes it immediately before accepting the current Agent advertisement.
    assert(database.execute(
        "CREATE TRIGGER trg_backend_agent_timer_delete_dormant_capability "
        "BEFORE INSERT ON backend_agent_command_capabilities "
        "WHEN NEW.command_type='vdr.timer.delete' "
        "BEGIN SELECT RAISE(IGNORE); END;"));

    const auto delivered = commands.poll(
        validPoll, "agt_timer_delivery", 120);
    assert(delivered.accepted);
    assert(delivered.assignment.present);
    assert(delivered.assignment.commandId == first.commandId);
    assert(commands.hasCapability(
        "default", "agt_timer_delivery", "inst_timer_delivery", 7,
        kBackendAgentNativeTimerDeleteCommandType));

    // Provider replacement after delivery fences redelivery and the first
    // receipt. The command has not crossed a durable Agent receipt boundary.
    const auto secondFacts = providerFacts("pie_timer_delivery_2", 4, 5);
    const auto staleRedelivery = commands.poll(
        pollRequest({kBackendAgentNativeTimerDeleteCommandType}, {secondFacts}),
        "agt_timer_delivery", 130);
    assert(staleRedelivery.accepted);
    assert(!staleRedelivery.assignment.present);
    assert(staleRedelivery.reasonCode == "local_provider_selection_stale");

    const auto staleReceipt = commands.acceptReceipt(receiptFor(first, 131));
    assert(!staleReceipt.accepted);
    assert(staleReceipt.reasonCode == "local_provider_selection_stale");

    // Retire the deliberately stale fixture so the next operation can bind to
    // the replacement provider fence.
    assert(database.execute(
        "UPDATE backend_agent_commands SET state='expired' "
        "WHERE command_id='cmd_timer_delivery_1';"));

    const auto secondSelection = commands.selectLocalProvider(
        "default", "agt_timer_delivery", "inst_timer_delivery", 7,
        kBackendAgentNativeTimerDeleteAuthorityDomain,
        kBackendAgentNativeTimerDeleteCapability, reason);
    assert(secondSelection.has_value());
    const auto second = timerDeleteAssignment("2", *secondSelection, 132);
    assert(commands.insertAssignment(second, &*secondSelection));

    const auto secondDelivery = commands.poll(
        pollRequest({kBackendAgentNativeTimerDeleteCommandType}, {secondFacts}),
        "agt_timer_delivery", 133);
    assert(secondDelivery.accepted);
    assert(secondDelivery.assignment.present);
    assert(secondDelivery.assignment.commandId == second.commandId);

    const auto receipt = receiptFor(second, 134);
    const auto acceptedReceipt = commands.acceptReceipt(receipt);
    assert(acceptedReceipt.accepted);
    assert(!acceptedReceipt.replayed);

    // After a durable receipt, provider drift must not invalidate an exact
    // lost-response receipt replay or a bounded result for the already accepted
    // command. Result validation stays bound to the persisted selection identity.
    const auto thirdFacts = providerFacts("pie_timer_delivery_3", 5, 6);
    const auto observeDrift = commands.poll(
        pollRequest({kBackendAgentNativeTimerDeleteCommandType}, {thirdFacts}),
        "agt_timer_delivery", 140);
    assert(observeDrift.accepted);
    assert(!observeDrift.assignment.present);

    const auto replayedReceipt = commands.acceptReceipt(receipt);
    assert(replayedReceipt.accepted);
    assert(replayedReceipt.replayed);

    const auto result = nonDispatchResult(second, 141);
    const auto acceptedResult = commands.acceptResult(result);
    assert(acceptedResult.accepted);
    assert(!acceptedResult.replayed);
    const auto replayedResult = commands.acceptResult(result);
    assert(replayedResult.accepted);
    assert(replayedResult.replayed);

    // Existing non-mutating command delivery remains unchanged.
    const auto probe = noopAssignment(150);
    assert(commands.insertAssignment(probe));
    const auto probeDelivery = commands.poll(
        pollRequest({"probe.noop"}), "agt_timer_delivery", 151);
    assert(probeDelivery.accepted);
    assert(probeDelivery.assignment.present);
    assert(probeDelivery.assignment.commandId == probe.commandId);

    return 0;
}

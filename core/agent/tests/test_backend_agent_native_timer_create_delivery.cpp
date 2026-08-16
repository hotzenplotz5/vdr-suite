#include "BackendAgentCommandDelivery.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerCreatePayload.h"
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
    bool includeCreate = true)
{
    BackendAgentLocalProviderFacts facts;
    facts.providerId = "suitebridge:local";
    facts.providerKind = "suitebridge";
    facts.providerInstanceEpoch = epoch;
    facts.providerGeneration = generation;
    facts.capabilityRevision = capabilityRevision;
    facts.available = true;
    facts.capabilities = includeCreate
        ? std::vector<std::string>{"vdr.timer.create"}
        : std::vector<std::string>{"vdr.native.probe"};
    return facts;
}

BackendAgentCommandPollRequest pollRequest(
    const std::vector<std::string>& commandTypes,
    const std::vector<BackendAgentLocalProviderFacts>& providers)
{
    BackendAgentCommandPollRequest poll;
    poll.backendId = "default";
    poll.agentInstanceId = "inst_timer_create_delivery";
    poll.backendGeneration = 7;
    poll.supportedCommandTypes = commandTypes;
    poll.localProviders = providers;
    return poll;
}

vdrsuite::agent::BackendAgentNativeTimerCreateSpecification specification(
    const std::string& suffix)
{
    vdrsuite::agent::BackendAgentNativeTimerCreateSpecification value;
    value.channelId = "C-1-2-3";
    value.title = "Phase 64 CREATE " + suffix;
    value.directory = "Tests";
    value.day = "2026-08-17";
    value.weekdays = "-------";
    value.startTime = "0930";
    value.endTime = "1030";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

BackendAgentCommandAssignment assignment(
    const std::string& suffix,
    const BackendAgentLocalProviderSelection& selection,
    std::int64_t assignedAt)
{
    using namespace vdrsuite::agent;
    BackendAgentNativeTimerCreatePayload payload;
    payload.operationRevision = "2";
    payload.timerAssignmentId = "ta_create_delivery_" + suffix;
    payload.expectedAssignmentRevision = "4";
    payload.expectedIntentRevision = "9";
    payload.assignmentEpoch = 3;
    payload.nativeTimerBindingId = "ntb_create_delivery_" + suffix;
    payload.controlPlaneClaimedAt = assignedAt - 1;
    payload.specification = specification(suffix);
    payload.expectedSpecificationFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(payload.specification);
    payload.localProviderSelection = selection;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.requestId = "req_create_delivery_" + suffix;
    value.correlationId = value.requestId;
    value.operationId = "op_create_delivery_" + suffix;
    value.jobId = "job_create_delivery_" + suffix;
    value.attemptId = "attempt_create_delivery_" + suffix;
    value.claimEpoch = 1;
    value.commandId = "cmd_create_delivery_" + suffix;
    value.backendId = "default";
    value.agentId = "agt_create_delivery";
    value.agentInstanceId = "inst_timer_create_delivery";
    value.backendGeneration = 7;
    value.commandType = kBackendAgentNativeTimerCreateCommandType;
    value.payloadVersion = kBackendAgentNativeTimerCreatePayloadVersion;
    value.payload = backendAgentNativeTimerCreatePayload(payload);
    value.verificationPolicy = "readback_required";
    value.assignedAt = assignedAt;
    value.deadline = assignedAt + 300;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

BackendAgentCommandReceipt receiptFor(
    const BackendAgentCommandAssignment& value,
    std::int64_t receivedAt)
{
    BackendAgentCommandReceipt receipt;
    receipt.commandId = value.commandId;
    receipt.requestFingerprint = value.requestFingerprint;
    receipt.jobId = value.jobId;
    receipt.attemptId = value.attemptId;
    receipt.claimEpoch = value.claimEpoch;
    receipt.backendId = value.backendId;
    receipt.agentId = value.agentId;
    receipt.agentInstanceId = value.agentInstanceId;
    receipt.backendGeneration = value.backendGeneration;
    receipt.receiptCategory = "accepted";
    receipt.receivedAt = receivedAt;
    receipt.reasonCode = "durably_recorded_before_native_create";
    assert(backendAgentCommandValidReceipt(receipt));
    return receipt;
}

BackendAgentCommandResult boundedResult(
    const BackendAgentCommandAssignment& value,
    std::int64_t completedAt)
{
    BackendAgentCommandResult result;
    result.commandId = value.commandId;
    result.requestFingerprint = value.requestFingerprint;
    result.jobId = value.jobId;
    result.attemptId = value.attemptId;
    result.claimEpoch = value.claimEpoch;
    result.backendId = value.backendId;
    result.agentId = value.agentId;
    result.agentInstanceId = value.agentInstanceId;
    result.backendGeneration = value.backendGeneration;
    result.dispatchState = "accepted_by_executor";
    result.verificationState = "outcome_unknown";
    result.resultCategory = "outcome_unknown";
    result.errorCategory = "executor_unknown";
    result.retryClassification = "reconcile_only";
    result.boundedDiagnostics = "CREATE dispatched; authoritative readback required";
    result.completedAt = completedAt;
    assert(backendAgentCommandValidResult(result));
    return result;
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
        "default", kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateProviderId,
        kBackendAgentNativeTimerCreateProviderKind,
        {kBackendAgentNativeTimerCreateCapability},
        100, ownership, reason));

    const auto firstFacts = providerFacts("pie_create_delivery_1", 3, 4);
    const auto observed = commands.poll(
        pollRequest({"probe.noop"}, {firstFacts}),
        "agt_create_delivery", 101);
    assert(observed.accepted);
    assert(!observed.assignment.present);

    const auto firstSelection = commands.selectLocalProvider(
        "default", "agt_create_delivery", "inst_timer_create_delivery", 7,
        kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateCapability, reason);
    assert(firstSelection.has_value());

    const auto first = assignment("1", *firstSelection, 110);
    assert(commands.insertAssignment(first, &*firstSelection));

    // Provider reachability/facts alone never makes the command executable.
    const auto advertisementClosed = commands.poll(
        pollRequest({"probe.noop"}, {firstFacts}),
        "agt_create_delivery", 111);
    assert(advertisementClosed.accepted);
    assert(!advertisementClosed.assignment.present);

    // Repository-level fixture can explicitly advertise CREATE to exercise the
    // delivery boundary. Production Agent advertisement remains closed elsewhere.
    const auto delivered = commands.poll(
        pollRequest({kBackendAgentNativeTimerCreateCommandType}, {firstFacts}),
        "agt_create_delivery", 120);
    assert(delivered.accepted);
    assert(delivered.assignment.present);
    assert(delivered.assignment.commandId == first.commandId);

    // Provider generation/epoch drift before the first durable receipt fences
    // both redelivery and receipt acceptance.
    const auto secondFacts = providerFacts("pie_create_delivery_2", 4, 5);
    const auto staleRedelivery = commands.poll(
        pollRequest({kBackendAgentNativeTimerCreateCommandType}, {secondFacts}),
        "agt_create_delivery", 130);
    assert(staleRedelivery.accepted);
    assert(!staleRedelivery.assignment.present);
    assert(staleRedelivery.reasonCode == "local_provider_selection_stale");

    const auto staleReceipt = commands.acceptReceipt(receiptFor(first, 131));
    assert(!staleReceipt.accepted);
    assert(staleReceipt.reasonCode == "local_provider_selection_stale");

    assert(database.execute(
        "UPDATE backend_agent_commands SET state='expired' "
        "WHERE command_id='cmd_create_delivery_1';"));

    const auto secondSelection = commands.selectLocalProvider(
        "default", "agt_create_delivery", "inst_timer_create_delivery", 7,
        kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateCapability, reason);
    assert(secondSelection.has_value());
    const auto second = assignment("2", *secondSelection, 132);
    assert(commands.insertAssignment(second, &*secondSelection));

    const auto secondDelivery = commands.poll(
        pollRequest({kBackendAgentNativeTimerCreateCommandType}, {secondFacts}),
        "agt_create_delivery", 133);
    assert(secondDelivery.accepted);
    assert(secondDelivery.assignment.present);
    assert(secondDelivery.assignment.commandId == second.commandId);

    const auto receipt = receiptFor(second, 134);
    const auto acceptedReceipt = commands.acceptReceipt(receipt);
    assert(acceptedReceipt.accepted);
    assert(!acceptedReceipt.replayed);

    // Once the receipt is durable, later provider drift cannot cause a blind
    // redispatch. Exact lost-response receipt/result replay remains tied to the
    // persisted provider selection and must flow into reconciliation/readback.
    const auto thirdFacts = providerFacts("pie_create_delivery_3", 5, 6);
    const auto observeDrift = commands.poll(
        pollRequest({kBackendAgentNativeTimerCreateCommandType}, {thirdFacts}),
        "agt_create_delivery", 140);
    assert(observeDrift.accepted);
    assert(!observeDrift.assignment.present);

    const auto replayedReceipt = commands.acceptReceipt(receipt);
    assert(replayedReceipt.accepted);
    assert(replayedReceipt.replayed);

    const auto result = boundedResult(second, 141);
    const auto acceptedResult = commands.acceptResult(result);
    assert(acceptedResult.accepted);
    assert(!acceptedResult.replayed);
    const auto replayedResult = commands.acceptResult(result);
    assert(replayedResult.accepted);
    assert(replayedResult.replayed);

    // A CREATE assignment without its durable provider-selection sidecar is
    // never deliverable, even when the currently observed provider is healthy.
    const auto currentSelection = commands.selectLocalProvider(
        "default", "agt_create_delivery", "inst_timer_create_delivery", 7,
        kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateCapability, reason);
    assert(currentSelection.has_value());
    const auto unbound = assignment("3", *currentSelection, 150);
    assert(commands.insertAssignment(unbound));
    const auto missingSelection = commands.poll(
        pollRequest({kBackendAgentNativeTimerCreateCommandType}, {thirdFacts}),
        "agt_create_delivery", 151);
    assert(missingSelection.accepted);
    assert(!missingSelection.assignment.present);
    assert(missingSelection.reasonCode == "local_provider_selection_required");

    return 0;
}

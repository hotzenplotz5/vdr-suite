#include "BackendAgentNativeTimerDeleteLocalState.h"

#include "BackendAgentNativeTimerDeletePayload.h"

#include <algorithm>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

namespace vdrsuite::agent
{
namespace
{

constexpr std::size_t kMaximumLocalStateBytes = 16U * 1024U;

bool evidenceEmpty(const BackendAgentNativeTimerDeleteEvidence& evidence)
{
    return evidence.commandId.empty() &&
        evidence.requestFingerprint.empty() &&
        evidence.operationId.empty() &&
        evidence.operationRevision.empty() &&
        evidence.jobId.empty() &&
        evidence.attemptId.empty() &&
        evidence.claimEpoch == 0 &&
        evidence.backendId.empty() &&
        evidence.agentId.empty() &&
        evidence.agentInstanceId.empty() &&
        evidence.backendGeneration == 0 &&
        evidence.providerInstanceEpoch.empty() &&
        evidence.localStartingPersistedAt == 0 &&
        evidence.outcome ==
            BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown &&
        evidence.dispatchStartedAt == 0 &&
        evidence.completedAt == 0 &&
        evidence.evidenceReference.empty();
}

BackendAgentNativeTimerDeleteEvidence evidenceFor(
    const BackendAgentNativeTimerDeleteCommand& command,
    std::int64_t localStartingPersistedAt,
    BackendAgentNativeTimerDeleteOutcomeCategory outcome,
    std::int64_t dispatchStartedAt,
    std::int64_t completedAt,
    const std::string& evidenceReference)
{
    BackendAgentNativeTimerDeleteEvidence evidence;
    evidence.commandId = command.commandId;
    evidence.requestFingerprint = command.requestFingerprint;
    evidence.operationId = command.operationId;
    evidence.operationRevision = command.operationRevision;
    evidence.jobId = command.jobId;
    evidence.attemptId = command.attemptId;
    evidence.claimEpoch = command.claimEpoch;
    evidence.backendId = command.backendId;
    evidence.agentId = command.agentId;
    evidence.agentInstanceId = command.agentInstanceId;
    evidence.backendGeneration = command.backendGeneration;
    evidence.providerInstanceEpoch =
        command.localProviderSelection.providerInstanceEpoch;
    evidence.localStartingPersistedAt = localStartingPersistedAt;
    evidence.outcome = outcome;
    evidence.dispatchStartedAt = dispatchStartedAt;
    evidence.completedAt = completedAt;
    evidence.evidenceReference = evidenceReference;
    return evidence;
}

std::string phaseText(BackendAgentNativeTimerDeleteLocalPhase phase)
{
    switch (phase)
    {
        case BackendAgentNativeTimerDeleteLocalPhase::starting:
            return "starting";
        case BackendAgentNativeTimerDeleteLocalPhase::completed:
            return "completed";
    }
    return {};
}

bool parsePhase(
    const std::string& value,
    BackendAgentNativeTimerDeleteLocalPhase& phase)
{
    if (value == "starting")
    {
        phase = BackendAgentNativeTimerDeleteLocalPhase::starting;
        return true;
    }
    if (value == "completed")
    {
        phase = BackendAgentNativeTimerDeleteLocalPhase::completed;
        return true;
    }
    return false;
}

std::string outcomeText(BackendAgentNativeTimerDeleteOutcomeCategory outcome)
{
    switch (outcome)
    {
        case BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect:
            return "rejected_without_effect";
        case BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified:
            return "accepted_unverified";
        case BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown:
            return "outcome_unknown";
    }
    return {};
}

bool parseOutcome(
    const std::string& value,
    BackendAgentNativeTimerDeleteOutcomeCategory& outcome)
{
    if (value == "rejected_without_effect")
    {
        outcome = BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect;
        return true;
    }
    if (value == "accepted_unverified")
    {
        outcome = BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified;
        return true;
    }
    if (value == "outcome_unknown")
    {
        outcome = BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown;
        return true;
    }
    return false;
}

bool parseNumber(const std::string& value, std::uint64_t& number)
{
    if (value.empty() || value.size() > 19 ||
        (value.size() > 1 && value.front() == '0')) return false;
    number = 0;
    for (unsigned char character : value)
    {
        if (character < '0' || character > '9') return false;
        const unsigned digit = static_cast<unsigned>(character - '0');
        if (number >
            (static_cast<std::uint64_t>(
                 std::numeric_limits<std::int64_t>::max()) -
             digit) /
                10U)
            return false;
        number = number * 10U + digit;
    }
    return true;
}

bool exactKeys(
    const std::map<std::string, std::string>& values,
    const std::vector<std::string>& expected)
{
    if (values.size() != expected.size()) return false;
    return std::all_of(expected.begin(), expected.end(), [&](const std::string& key) {
        return values.count(key) == 1;
    });
}

const std::vector<std::string>& localStateKeys()
{
    static const std::vector<std::string> keys = {
        "schema",
        "phase",
        "command_id",
        "request_fingerprint",
        "operation_id",
        "operation_revision",
        "native_timer_binding_id",
        "expected_binding_revision",
        "timer_assignment_id",
        "backend_native_timer_id",
        "job_id",
        "attempt_id",
        "claim_epoch",
        "backend_id",
        "agent_id",
        "agent_instance_id",
        "backend_generation",
        "control_plane_claimed_at",
        "provider_backend_id",
        "provider_authority_domain",
        "provider_id",
        "provider_kind",
        "ownership_generation",
        "provider_instance_epoch",
        "provider_generation",
        "capability_revision",
        "required_capability",
        "local_starting_persisted_at",
        "outcome",
        "dispatch_started_at",
        "completed_at",
        "evidence_reference",
    };
    return keys;
}

bool sameContext(
    const BackendAgentNativeTimerDeleteCommand& command,
    const std::string& backendId,
    const std::string& agentId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration)
{
    return command.backendId == backendId &&
        command.agentId == agentId &&
        command.agentInstanceId == agentInstanceId &&
        command.backendGeneration == backendGeneration;
}

} // namespace

bool backendAgentNativeTimerDeleteCommandFromAssignment(
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerDeleteCommand& command,
    std::string& reasonCode)
{
    if (!::backendAgentCommandValidAssignment(assignment) ||
        assignment.commandType != kBackendAgentNativeTimerDeleteCommandType ||
        assignment.payloadVersion != kBackendAgentNativeTimerDeletePayloadVersion ||
        assignment.verificationPolicy != "readback_required")
    {
        reasonCode = "invalid_native_timer_delete_assignment";
        return false;
    }

    BackendAgentNativeTimerDeletePayload payload;
    if (!backendAgentNativeTimerDeleteParsePayload(
            assignment.payload, payload, reasonCode))
    {
        reasonCode = "invalid_native_timer_delete_assignment_payload";
        return false;
    }

    BackendAgentNativeTimerDeleteCommand candidate;
    candidate.commandId = assignment.commandId;
    candidate.requestFingerprint = assignment.requestFingerprint;
    candidate.operationId = assignment.operationId;
    candidate.operationRevision = payload.operationRevision;
    candidate.nativeTimerBindingId = payload.nativeTimerBindingId;
    candidate.expectedBindingRevision = payload.expectedBindingRevision;
    candidate.timerAssignmentId = payload.timerAssignmentId;
    candidate.backendNativeTimerId = payload.backendNativeTimerId;
    candidate.jobId = assignment.jobId;
    candidate.attemptId = assignment.attemptId;
    candidate.claimEpoch = assignment.claimEpoch;
    candidate.backendId = assignment.backendId;
    candidate.agentId = assignment.agentId;
    candidate.agentInstanceId = assignment.agentInstanceId;
    candidate.backendGeneration = assignment.backendGeneration;
    candidate.controlPlaneClaimedAt = payload.controlPlaneClaimedAt;
    candidate.localProviderSelection = payload.localProviderSelection;

    if (!backendAgentNativeTimerDeleteValidCommand(candidate, reasonCode))
    {
        reasonCode = "invalid_native_timer_delete_command_envelope";
        return false;
    }

    command = candidate;
    reasonCode.clear();
    return true;
}

bool backendAgentNativeTimerDeletePrepareLocalStarting(
    const BackendAgentCommandAssignment& assignment,
    std::int64_t now,
    BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode)
{
    BackendAgentNativeTimerDeleteCommand command;
    if (!backendAgentNativeTimerDeleteCommandFromAssignment(
            assignment, command, reasonCode))
        return false;

    if (now <= 0 || now < command.controlPlaneClaimedAt ||
        now > assignment.deadline)
    {
        reasonCode = "native_timer_delete_starting_time_fenced";
        return false;
    }

    BackendAgentNativeTimerDeleteLocalState candidate;
    candidate.schemaVersion = 1;
    candidate.phase = BackendAgentNativeTimerDeleteLocalPhase::starting;
    candidate.command = command;
    candidate.localStartingPersistedAt = now;

    if (!backendAgentNativeTimerDeleteLocalStateValid(candidate, reasonCode))
        return false;

    state = candidate;
    reasonCode = "native_timer_delete_starting_ready_for_durable_persist";
    return true;
}

bool backendAgentNativeTimerDeleteLocalStateValid(
    const BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode)
{
    if (state.schemaVersion != 1 ||
        !backendAgentNativeTimerDeleteValidCommand(state.command, reasonCode) ||
        state.localStartingPersistedAt < state.command.controlPlaneClaimedAt)
    {
        reasonCode = "invalid_native_timer_delete_local_state";
        return false;
    }

    if (state.phase == BackendAgentNativeTimerDeleteLocalPhase::starting)
    {
        if (!evidenceEmpty(state.evidence))
        {
            reasonCode = "starting_native_timer_delete_has_outcome_evidence";
            return false;
        }
        reasonCode.clear();
        return true;
    }

    if (state.phase != BackendAgentNativeTimerDeleteLocalPhase::completed ||
        !::backendAgentCommandSafeText(state.evidence.evidenceReference, 512) ||
        state.evidence.localStartingPersistedAt !=
            state.localStartingPersistedAt ||
        !backendAgentNativeTimerDeleteEvidenceMatches(
            state.evidence, state.command, reasonCode))
    {
        reasonCode = "invalid_native_timer_delete_completed_state";
        return false;
    }

    reasonCode.clear();
    return true;
}

bool backendAgentNativeTimerDeleteCompleteLocalState(
    BackendAgentNativeTimerDeleteLocalState& state,
    const BackendAgentNativeTimerDeleteEvidence& evidence,
    std::string& reasonCode)
{
    if (!backendAgentNativeTimerDeleteLocalStateValid(state, reasonCode) ||
        state.phase != BackendAgentNativeTimerDeleteLocalPhase::starting)
    {
        reasonCode = "native_timer_delete_local_state_not_starting";
        return false;
    }

    if (evidence.localStartingPersistedAt != state.localStartingPersistedAt ||
        !::backendAgentCommandSafeText(evidence.evidenceReference, 512) ||
        !backendAgentNativeTimerDeleteEvidenceMatches(
            evidence, state.command, reasonCode))
    {
        reasonCode = "native_timer_delete_completion_evidence_mismatch";
        return false;
    }

    BackendAgentNativeTimerDeleteLocalState candidate = state;
    candidate.phase = BackendAgentNativeTimerDeleteLocalPhase::completed;
    candidate.evidence = evidence;
    if (!backendAgentNativeTimerDeleteLocalStateValid(candidate, reasonCode))
        return false;

    state = candidate;
    reasonCode = "native_timer_delete_completion_ready_for_durable_persist";
    return true;
}

BackendAgentNativeTimerDeleteRecoveryResult
backendAgentNativeTimerDeleteRecoverLocalState(
    const BackendAgentNativeTimerDeleteLocalState& state,
    const std::string& backendId,
    const std::string& agentId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration,
    std::int64_t now)
{
    BackendAgentNativeTimerDeleteRecoveryResult result;
    std::string reason;
    if (!backendAgentNativeTimerDeleteLocalStateValid(state, reason))
    {
        result.reasonCode = "native_timer_delete_recovery_state_invalid";
        return result;
    }
    if (!::backendAgentCommandSafeIdentifier(backendId) ||
        !::backendAgentCommandSafeIdentifier(agentId) ||
        !::backendAgentCommandSafeIdentifier(agentInstanceId) ||
        backendGeneration == 0 || now <= 0)
    {
        result.reasonCode = "native_timer_delete_recovery_context_invalid";
        return result;
    }

    const bool contextCurrent = sameContext(
        state.command, backendId, agentId, agentInstanceId, backendGeneration);

    if (state.phase == BackendAgentNativeTimerDeleteLocalPhase::completed)
    {
        result.decision =
            BackendAgentNativeTimerDeleteRecoveryDecision::returnPersistedEvidence;
        result.evidence = state.evidence;
        result.reasonCode = contextCurrent
            ? "native_timer_delete_completed_evidence_replay"
            : "native_timer_delete_completed_evidence_survives_context_drift";
        return result;
    }

    // A durable starting record opens the no-blind-retry hazard window. A crash
    // can happen immediately before or after the future side-effectful call, so
    // recovery conservatively treats the earliest possible dispatch boundary as
    // the durable starting timestamp and emits outcome-unknown evidence.
    const std::int64_t completedAt =
        std::max(now, state.localStartingPersistedAt);
    result.evidence = evidenceFor(
        state.command,
        state.localStartingPersistedAt,
        BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown,
        state.localStartingPersistedAt,
        completedAt,
        "local-recovery:" + state.command.commandId);

    if (!backendAgentNativeTimerDeleteEvidenceMatches(
            result.evidence, state.command, reason))
    {
        result.reasonCode = "native_timer_delete_recovery_evidence_invalid";
        return result;
    }

    result.decision =
        BackendAgentNativeTimerDeleteRecoveryDecision::reconcileOnly;
    result.reasonCode = contextCurrent
        ? "native_timer_delete_starting_recovery_reconcile_only"
        : "native_timer_delete_starting_context_fenced_reconcile_only";
    return result;
}

std::string backendAgentNativeTimerDeleteSerializeLocalState(
    const BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode)
{
    if (!backendAgentNativeTimerDeleteLocalStateValid(state, reasonCode))
        return {};

    const auto& command = state.command;
    const auto& selection = command.localProviderSelection;
    const bool completed =
        state.phase == BackendAgentNativeTimerDeleteLocalPhase::completed;
    const auto& evidence = state.evidence;

    std::ostringstream output;
    output
        << "schema=1\n"
        << "phase=" << phaseText(state.phase) << "\n"
        << "command_id=" << command.commandId << "\n"
        << "request_fingerprint=" << command.requestFingerprint << "\n"
        << "operation_id=" << command.operationId << "\n"
        << "operation_revision=" << command.operationRevision << "\n"
        << "native_timer_binding_id=" << command.nativeTimerBindingId << "\n"
        << "expected_binding_revision=" << command.expectedBindingRevision << "\n"
        << "timer_assignment_id=" << command.timerAssignmentId << "\n"
        << "backend_native_timer_id=" << command.backendNativeTimerId << "\n"
        << "job_id=" << command.jobId << "\n"
        << "attempt_id=" << command.attemptId << "\n"
        << "claim_epoch=" << command.claimEpoch << "\n"
        << "backend_id=" << command.backendId << "\n"
        << "agent_id=" << command.agentId << "\n"
        << "agent_instance_id=" << command.agentInstanceId << "\n"
        << "backend_generation=" << command.backendGeneration << "\n"
        << "control_plane_claimed_at=" << command.controlPlaneClaimedAt << "\n"
        << "provider_backend_id=" << selection.backendId << "\n"
        << "provider_authority_domain=" << selection.authorityDomain << "\n"
        << "provider_id=" << selection.providerId << "\n"
        << "provider_kind=" << selection.providerKind << "\n"
        << "ownership_generation=" << selection.ownershipGeneration << "\n"
        << "provider_instance_epoch=" << selection.providerInstanceEpoch << "\n"
        << "provider_generation=" << selection.providerGeneration << "\n"
        << "capability_revision=" << selection.capabilityRevision << "\n"
        << "required_capability=" << selection.requiredCapability << "\n"
        << "local_starting_persisted_at=" << state.localStartingPersistedAt << "\n"
        << "outcome="
        << (completed ? outcomeText(evidence.outcome) : "none") << "\n"
        << "dispatch_started_at="
        << (completed ? evidence.dispatchStartedAt : 0) << "\n"
        << "completed_at=" << (completed ? evidence.completedAt : 0) << "\n"
        << "evidence_reference="
        << (completed ? evidence.evidenceReference : "") << "\n";

    const std::string encoded = output.str();
    if (encoded.size() > kMaximumLocalStateBytes)
    {
        reasonCode = "native_timer_delete_local_state_too_large";
        return {};
    }
    reasonCode.clear();
    return encoded;
}

bool backendAgentNativeTimerDeleteParseLocalState(
    const std::string& encoded,
    BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode)
{
    if (encoded.empty() || encoded.size() > kMaximumLocalStateBytes)
    {
        reasonCode = "invalid_native_timer_delete_local_state_encoding";
        return false;
    }

    std::map<std::string, std::string> values;
    std::istringstream input(encoded);
    std::string line;
    while (std::getline(input, line))
    {
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos || separator == 0 ||
            !values.emplace(
                line.substr(0, separator),
                line.substr(separator + 1)).second)
        {
            reasonCode = "invalid_native_timer_delete_local_state_encoding";
            return false;
        }
    }

    if (!exactKeys(values, localStateKeys()) || values["schema"] != "1")
    {
        reasonCode = "invalid_native_timer_delete_local_state_schema";
        return false;
    }

    BackendAgentNativeTimerDeleteLocalState candidate;
    candidate.schemaVersion = 1;
    if (!parsePhase(values["phase"], candidate.phase))
    {
        reasonCode = "invalid_native_timer_delete_local_state_phase";
        return false;
    }

    auto& command = candidate.command;
    command.commandId = values["command_id"];
    command.requestFingerprint = values["request_fingerprint"];
    command.operationId = values["operation_id"];
    command.operationRevision = values["operation_revision"];
    command.nativeTimerBindingId = values["native_timer_binding_id"];
    command.expectedBindingRevision = values["expected_binding_revision"];
    command.timerAssignmentId = values["timer_assignment_id"];
    command.backendNativeTimerId = values["backend_native_timer_id"];
    command.jobId = values["job_id"];
    command.attemptId = values["attempt_id"];
    command.backendId = values["backend_id"];
    command.agentId = values["agent_id"];
    command.agentInstanceId = values["agent_instance_id"];
    command.localProviderSelection.backendId = values["provider_backend_id"];
    command.localProviderSelection.authorityDomain =
        values["provider_authority_domain"];
    command.localProviderSelection.providerId = values["provider_id"];
    command.localProviderSelection.providerKind = values["provider_kind"];
    command.localProviderSelection.providerInstanceEpoch =
        values["provider_instance_epoch"];
    command.localProviderSelection.requiredCapability =
        values["required_capability"];

    std::uint64_t claimEpoch = 0;
    std::uint64_t backendGeneration = 0;
    std::uint64_t controlPlaneClaimedAt = 0;
    std::uint64_t ownershipGeneration = 0;
    std::uint64_t providerGeneration = 0;
    std::uint64_t capabilityRevision = 0;
    std::uint64_t localStartingPersistedAt = 0;
    std::uint64_t dispatchStartedAt = 0;
    std::uint64_t completedAt = 0;
    if (!parseNumber(values["claim_epoch"], claimEpoch) ||
        !parseNumber(values["backend_generation"], backendGeneration) ||
        !parseNumber(values["control_plane_claimed_at"], controlPlaneClaimedAt) ||
        !parseNumber(values["ownership_generation"], ownershipGeneration) ||
        !parseNumber(values["provider_generation"], providerGeneration) ||
        !parseNumber(values["capability_revision"], capabilityRevision) ||
        !parseNumber(
            values["local_starting_persisted_at"], localStartingPersistedAt) ||
        !parseNumber(values["dispatch_started_at"], dispatchStartedAt) ||
        !parseNumber(values["completed_at"], completedAt))
    {
        reasonCode = "invalid_native_timer_delete_local_state_number";
        return false;
    }

    command.claimEpoch = claimEpoch;
    command.backendGeneration = backendGeneration;
    command.controlPlaneClaimedAt =
        static_cast<std::int64_t>(controlPlaneClaimedAt);
    command.localProviderSelection.ownershipGeneration = ownershipGeneration;
    command.localProviderSelection.providerGeneration = providerGeneration;
    command.localProviderSelection.capabilityRevision = capabilityRevision;
    candidate.localStartingPersistedAt =
        static_cast<std::int64_t>(localStartingPersistedAt);

    if (candidate.phase == BackendAgentNativeTimerDeleteLocalPhase::starting)
    {
        if (values["outcome"] != "none" || dispatchStartedAt != 0 ||
            completedAt != 0 || !values["evidence_reference"].empty())
        {
            reasonCode = "invalid_native_timer_delete_starting_encoding";
            return false;
        }
    }
    else
    {
        BackendAgentNativeTimerDeleteOutcomeCategory outcome;
        if (!parseOutcome(values["outcome"], outcome))
        {
            reasonCode = "invalid_native_timer_delete_local_state_outcome";
            return false;
        }
        candidate.evidence = evidenceFor(
            command,
            candidate.localStartingPersistedAt,
            outcome,
            static_cast<std::int64_t>(dispatchStartedAt),
            static_cast<std::int64_t>(completedAt),
            values["evidence_reference"]);
    }

    if (!backendAgentNativeTimerDeleteLocalStateValid(candidate, reasonCode))
        return false;

    state = candidate;
    reasonCode.clear();
    return true;
}

} // namespace vdrsuite::agent

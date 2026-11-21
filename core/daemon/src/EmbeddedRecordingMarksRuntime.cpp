#include "EmbeddedRecordingMarksRuntime.h"

#include "BackendAgentLifecycle.h"
#include "BackendAgentRecordingMarksModifyPayload.h"

#include <chrono>
#include <sstream>
#include <utility>

namespace
{
using namespace vdrsuite::agent;
std::string identity(const RecordingMarksMutationRequest& r)
{
    // Length-prefixed fields prevent delimiter collisions; compare full requests.
    std::ostringstream out;
    for (const auto& field : {r.backendId, r.recordingKey, r.operationId,
            r.operationRevision, r.expectedMarksRevision})
        out << field.size() << ':' << field;
    out << ':' << static_cast<int>(r.kind) << ':' << r.sourceFrame << ':' << r.targetFrame;
    for (int frame : r.replacementFrames) out << ':' << frame;
    return out.str();
}
BackendAgentRecordingMarksModifyKind kind(RecordingMarksMutationKind value)
{
    switch (value)
    {
    case RecordingMarksMutationKind::Add: return BackendAgentRecordingMarksModifyKind::add;
    case RecordingMarksMutationKind::Delete: return BackendAgentRecordingMarksModifyKind::deleteMark;
    case RecordingMarksMutationKind::Move: return BackendAgentRecordingMarksModifyKind::move;
    case RecordingMarksMutationKind::Reset: return BackendAgentRecordingMarksModifyKind::reset;
    case RecordingMarksMutationKind::Replace: return BackendAgentRecordingMarksModifyKind::replace;
    }
    return BackendAgentRecordingMarksModifyKind::add;
}
RecordingMarksMutationDispatchResult failure(const std::string& reason)
{
    RecordingMarksMutationDispatchResult result;
    result.reasonCode = reason;
    return result;
}
}

EmbeddedRecordingMarksRuntime::EmbeddedRecordingMarksRuntime(Database& database,
    std::string backendId, IBackendAgentRecordingMarksModifyTransport& transport,
    IVdrRecordingNativeMarksResolver& resolver)
    : repository_(database), backendId_(std::move(backendId)), transport_(transport), resolver_(resolver)
{
}

bool EmbeddedRecordingMarksRuntime::ensureSchema()
{
    return repository_.ensureSchema();
}

RecordingMarksMutationDispatchResult EmbeddedRecordingMarksRuntime::dispatch(
    const RecordingMarksMutationRequest& request)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (request.backendId != backendId_)
        return failure("recording_marks_modify_backend_conflict");
    const auto requestIdentity = identity(request);
    BackendAgentRecordingMarksModifyPayload payload;
    BackendAgentRecordingMarksModifyCommand command;
    std::string state, evidence, canonical, encoded, reason;
    EmbeddedRecordingMarksRecord record;
    if (!repository_.find(backendId_, request.operationId, record))
        return failure("recording_marks_modify_journal_unavailable");
    const bool replayed = record.found;
    if (replayed)
    {
        if (record.requestIdentity != requestIdentity)
            return failure("recording_marks_modify_assignment_conflict");
        command.commandId = record.commandId;
        command.requestFingerprint = record.fingerprint;
        encoded = record.payload;
        command.agentInstanceId = record.instanceId;
        state = record.state; evidence = record.evidence; canonical = record.canonicalRevision;
        if (!backendAgentRecordingMarksModifyParsePayload(encoded, payload, reason))
            return failure("recording_marks_modify_journal_invalid");
    }
    if (!replayed)
    {
        if (request.replayOnly)
            return failure("recording_marks_modify_assignment_not_found");
        BackendAgentLocalProviderFacts facts;
        if (!transport_.discoverProvider(facts, reason) || !facts.available)
            return failure("recording_marks_modify_suitebridge_capability_unavailable");
        BackendAgentLocalProviderOwnership ownership;
        ownership.backendId = backendId_;
        ownership.authorityDomain = kBackendAgentRecordingMarksModifyAuthorityDomain;
        ownership.providerId = kBackendAgentRecordingMarksModifyProviderId;
        ownership.providerKind = kBackendAgentRecordingMarksModifyProviderKind;
        ownership.ownershipGeneration = 1;
        ownership.allowedCapabilities = {kBackendAgentRecordingMarksModifyCapability};
        payload.localProviderSelection = backendAgentLocalProviderSelect(ownership, facts,
            kBackendAgentRecordingMarksModifyCapability, reason);
        payload.kind = kind(request.kind);
        payload.operationRevision = request.operationRevision;
        payload.recordingKey = request.recordingKey;
        payload.expectedMarksRevision = request.expectedMarksRevision;
        payload.sourceFrame = request.sourceFrame;
        payload.targetFrame = request.targetFrame;
        payload.replacementFrames = request.replacementFrames;
        payload.backendId = backendId_;
        payload.backendGeneration = 1;
        payload.controlPlaneClaimedAt = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        encoded = backendAgentRecordingMarksModifyPayload(payload);
        if (encoded.empty()) return failure("invalid_recording_marks_modify_assignment_request");
        command.commandId = backendAgentGenerateOpaqueId("embedded_marks_", 12);
        command.agentInstanceId = backendAgentGenerateOpaqueId("embedded_", 12);
        BackendAgentCommandAssignment fingerprintInput;
        fingerprintInput.commandId = command.commandId;
        fingerprintInput.operationId = request.operationId;
        fingerprintInput.backendId = backendId_;
        fingerprintInput.payload = encoded;
        command.requestFingerprint = backendAgentCommandFingerprint(fingerprintInput);
        state = "starting";
    }
    command.kind = payload.kind;
    command.operationId = request.operationId;
    command.operationRevision = payload.operationRevision;
    command.recordingKey = payload.recordingKey;
    command.expectedMarksRevision = payload.expectedMarksRevision;
    command.sourceFrame = payload.sourceFrame;
    command.targetFrame = payload.targetFrame;
    command.replacementFrames = payload.replacementFrames;
    command.jobId = command.commandId;
    command.attemptId = command.commandId;
    command.claimEpoch = 1;
    command.backendId = backendId_;
    // Protocol identity of this in-process executor, not an external agent lease.
    command.agentId = "embedded-suitebridge";
    command.backendGeneration = payload.backendGeneration;
    command.controlPlaneClaimedAt = payload.controlPlaneClaimedAt;
    command.localProviderSelection = payload.localProviderSelection;
    if (!backendAgentRecordingMarksModifyValidCommand(command, reason))
        return failure("invalid_recording_marks_modify_assignment_request");
    if (!replayed)
    {
        record.requestIdentity = requestIdentity; record.commandId = command.commandId;
        record.fingerprint = command.requestFingerprint; record.payload = encoded;
        record.instanceId = command.agentInstanceId; record.state = state;
        if (!repository_.insert(record))
            return failure("recording_marks_modify_journal_unavailable");
    }
    auto persist = [&]() {
        record.state = state; record.evidence = evidence; record.canonicalRevision = canonical;
        return repository_.update(record);
    };
    if (state == "starting" || state == "unknown")
    {
        // Reuse the exact persisted command. The native replay ledger prevents
        // duplicate effects; the original provider epoch fences VDR restarts.
        BackendAgentRecordingMarksModifyTransportRequest wire;
        wire.command = command;
        wire.localStartingPersistedAt = payload.controlPlaneClaimedAt;
        const auto reply = transport_.modifyMarks(wire);
        evidence = reply.evidenceReference;
        // A rejection of a replay after an unknown outcome (e.g. old provider
        // epoch after VDR restart) cannot prove the original command had no effect.
        if (replayed && reply.disposition ==
                BackendAgentRecordingMarksModifyTransportDisposition::rejectedWithoutEffect)
        {
            state = "uncertain";
        }
        else state = reply.disposition == BackendAgentRecordingMarksModifyTransportDisposition::rejectedWithoutEffect
            ? "rejected" : reply.disposition == BackendAgentRecordingMarksModifyTransportDisposition::acceptedUnverified
            ? "accepted" : "unknown";
        if (!persist()) return failure("recording_marks_modify_journal_unavailable");
    }
    if (state == "rejected") return failure("recording_marks_modify_rejected");
    if (state == "accepted")
    {
        const std::string prefix = "nmarks:vdr:postrev:", suffix = ":" + command.commandId;
        if (evidence.size() == prefix.size() + 32 + suffix.size() &&
            evidence.compare(0, prefix.size(), prefix) == 0 &&
            evidence.compare(evidence.size() - suffix.size(), suffix.size(), suffix) == 0)
        {
            const auto revision = evidence.substr(prefix.size(), 32);
            const auto native = resolver_.resolve(request.recordingKey);
            if (backendAgentRecordingMarksModifyRevisionTokenValid(revision) &&
                native.availability == VdrRecordingNativeMarksAvailability::Available &&
                native.found && native.recordingKey == request.recordingKey &&
                native.marksRevision == revision)
            {
                canonical = revision;
                state = "verified";
                if (!persist()) return failure("recording_marks_modify_journal_unavailable");
            }
        }
    }
    RecordingMarksMutationDispatchResult result;
    result.accepted = true;
    result.replayed = replayed;
    result.commandId = command.commandId;
    result.requestFingerprint = command.requestFingerprint;
    result.verified = state == "verified";
    result.canonicalMarksRevision = canonical;
    result.reasonCode = result.verified ? "recording_marks_modify_verified" : "recording_marks_modify_pending";
    return result;
}

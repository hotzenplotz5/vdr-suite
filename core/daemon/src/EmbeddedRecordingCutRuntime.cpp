#include "EmbeddedRecordingCutRuntime.h"

#include "BackendAgentLifecycle.h"
#include "BackendAgentRecordingCutPayload.h"
#include "DaemonRecordingCutReconciliation.h"
#include "VdrRecordingNativeIdentity.h"

#include <chrono>
#include <sstream>
#include <utility>

namespace
{
using namespace vdrsuite::agent;

std::string identity(const RecordingCutStartRequest& request)
{
    std::ostringstream value;

    for (const auto& field : {
            request.backendId,
            request.recordingKey,
            request.operationId,
            request.operationRevision,
            request.expectedMarksRevision})
    {
        value << field.size() << ':' << field;
    }

    return value.str();
}

RecordingCutDispatchResult failure(
    const std::string& reason)
{
    RecordingCutDispatchResult result;
    result.reasonCode = reason;
    return result;
}

bool editedKeyFromEvidence(
    const std::string& evidence,
    const std::string& commandId,
    std::string& editedRecordingKey)
{
    const std::string prefix = "ncut:vdr:queued:";
    const std::string suffix = ":" + commandId;

    editedRecordingKey.clear();

    if (evidence.size() !=
            prefix.size() + 32U + suffix.size() ||
        evidence.compare(
            0,
            prefix.size(),
            prefix) != 0 ||
        evidence.compare(
            evidence.size() - suffix.size(),
            suffix.size(),
            suffix) != 0)
    {
        return false;
    }

    editedRecordingKey =
        evidence.substr(prefix.size(), 32U);

    return
        backendAgentRecordingCutRevisionTokenValid(
            editedRecordingKey);
}
}

EmbeddedRecordingCutRuntime::EmbeddedRecordingCutRuntime(
    Database& database,
    std::string backendId,
    IBackendAgentRecordingCutTransport& transport,
    IVdrRecordingNativeCutStateResolver& resolver)
    : repository_(database),
      backendId_(std::move(backendId)),
      transport_(transport),
      resolver_(resolver)
{
}

bool EmbeddedRecordingCutRuntime::ensureSchema()
{
    return repository_.ensureSchema();
}

RecordingCutDispatchResult EmbeddedRecordingCutRuntime::dispatch(
    const RecordingCutStartRequest& request)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (request.backendId != backendId_)
        return failure("recording_cut_backend_conflict");

    const std::string requestIdentity =
        identity(request);

    BackendAgentRecordingCutPayload payload;
    BackendAgentRecordingCutCommand command;
    EmbeddedRecordingCutRecord record;

    std::string encoded;
    std::string state;
    std::string evidence;
    std::string editedRecordingKey;
    std::string reason;

    if (!repository_.find(
            backendId_,
            request.operationId,
            record))
    {
        return failure(
            "recording_cut_journal_unavailable");
    }

    const bool replayed = record.found;

    if (replayed)
    {
        if (record.requestIdentity != requestIdentity)
            return failure(
                "recording_cut_assignment_conflict");

        command.commandId = record.commandId;
        command.requestFingerprint =
            record.fingerprint;
        command.agentInstanceId =
            record.instanceId;

        encoded = record.payload;
        state = record.state;
        evidence = record.evidence;
        editedRecordingKey =
            record.editedRecordingKey;

        if (!backendAgentRecordingCutParsePayload(
                encoded,
                payload,
                reason))
        {
            return failure(
                "recording_cut_journal_invalid");
        }
    }
    else
    {
        if (request.replayOnly)
            return failure(
                "recording_cut_assignment_not_found");

        BackendAgentLocalProviderFacts facts;

        if (!transport_.discoverProvider(
                facts,
                reason) ||
            !facts.available)
        {
            return failure(
                "recording_cut_suitebridge_capability_unavailable");
        }

        BackendAgentLocalProviderOwnership ownership;
        ownership.backendId = backendId_;
        ownership.authorityDomain =
            kBackendAgentRecordingCutAuthorityDomain;
        ownership.providerId =
            kBackendAgentRecordingCutProviderId;
        ownership.providerKind =
            kBackendAgentRecordingCutProviderKind;
        ownership.ownershipGeneration = 1;
        ownership.allowedCapabilities = {
            kBackendAgentRecordingCutCapability};

        payload.localProviderSelection =
            backendAgentLocalProviderSelect(
                ownership,
                facts,
                kBackendAgentRecordingCutCapability,
                reason);

        payload.operationRevision =
            request.operationRevision;
        payload.recordingKey =
            request.recordingKey;
        payload.expectedMarksRevision =
            request.expectedMarksRevision;
        payload.backendId = backendId_;
        payload.backendGeneration = 1;
        payload.controlPlaneClaimedAt =
            std::chrono::duration_cast<
                std::chrono::seconds>(
                std::chrono::system_clock::now()
                    .time_since_epoch())
                .count();

        encoded =
            backendAgentRecordingCutPayload(payload);

        if (encoded.empty())
            return failure(
                "invalid_recording_cut_assignment_request");

        command.commandId =
            backendAgentGenerateOpaqueId(
                "embedded_cut_",
                12);

        command.agentInstanceId =
            backendAgentGenerateOpaqueId(
                "embedded_",
                12);

        BackendAgentCommandAssignment fingerprintInput;
        fingerprintInput.commandId =
            command.commandId;
        fingerprintInput.operationId =
            request.operationId;
        fingerprintInput.backendId =
            backendId_;
        fingerprintInput.payload = encoded;

        command.requestFingerprint =
            backendAgentCommandFingerprint(
                fingerprintInput);

        state = "starting";
    }

    command.operationId =
        request.operationId;
    command.operationRevision =
        payload.operationRevision;
    command.recordingKey =
        payload.recordingKey;
    command.expectedMarksRevision =
        payload.expectedMarksRevision;
    command.jobId =
        command.commandId;
    command.attemptId =
        command.commandId;
    command.claimEpoch = 1;
    command.backendId =
        backendId_;

    // Protocol identity of the daemon-local executor.
    // This is not an external Backend-Agent lease.
    command.agentId =
        "embedded-suitebridge";

    command.backendGeneration =
        payload.backendGeneration;
    command.controlPlaneClaimedAt =
        payload.controlPlaneClaimedAt;
    command.localProviderSelection =
        payload.localProviderSelection;

    if (!backendAgentRecordingCutValidCommand(
            command,
            reason))
    {
        return failure(
            "invalid_recording_cut_assignment_request");
    }

    if (!replayed)
    {
        record.requestIdentity =
            requestIdentity;
        record.commandId =
            command.commandId;
        record.fingerprint =
            command.requestFingerprint;
        record.payload =
            encoded;
        record.instanceId =
            command.agentInstanceId;
        record.state =
            state;

        if (!repository_.insert(record))
            return failure(
                "recording_cut_journal_unavailable");
    }

    auto persist = [&]() {
        record.state = state;
        record.evidence = evidence;
        record.editedRecordingKey =
            editedRecordingKey;

        return repository_.update(record);
    };

    if (state == "starting" ||
        state == "unknown")
    {
        BackendAgentRecordingCutTransportRequest wire;
        wire.command = command;
        wire.localStartingPersistedAt =
            payload.controlPlaneClaimedAt;

        const auto reply =
            transport_.startCut(wire);

        evidence =
            reply.evidenceReference;

        if (replayed &&
            reply.disposition ==
                BackendAgentRecordingCutTransportDisposition::
                    rejectedWithoutEffect)
        {
            /*
             * After an unknown outcome a rejection cannot prove
             * that the original native command had no effect.
             */
            state = "uncertain";
        }
        else if (
            reply.disposition ==
            BackendAgentRecordingCutTransportDisposition::
                rejectedWithoutEffect)
        {
            state = "rejected";
        }
        else if (
            reply.disposition ==
            BackendAgentRecordingCutTransportDisposition::
                acceptedUnverified)
        {
            if (editedKeyFromEvidence(
                    evidence,
                    command.commandId,
                    editedRecordingKey) &&
                editedRecordingKey !=
                    request.recordingKey)
            {
                state = "accepted";
            }
            else
            {
                editedRecordingKey.clear();
                state = "unknown";
            }
        }
        else
        {
            state = "unknown";
        }

        if (!persist())
            return failure(
                "recording_cut_journal_unavailable");
    }

    if (state == "rejected")
        return failure(
            "recording_cut_rejected");

    if (state == "accepted")
    {
        const auto native =
            resolver_.resolve(
                request.recordingKey);

        if (daemonRecordingCutResultMatches(
                request.recordingKey,
                editedRecordingKey,
                native))
        {
            state = "verified";

            if (!persist())
                return failure(
                    "recording_cut_journal_unavailable");
        }
    }

    if (state != "accepted" &&
        state != "unknown" &&
        state != "uncertain" &&
        state != "verified")
    {
        return failure(
            "recording_cut_journal_invalid");
    }

    RecordingCutDispatchResult result;
    result.accepted = true;
    result.replayed = replayed;
    result.verified =
        state == "verified";
    result.commandId =
        command.commandId;
    result.requestFingerprint =
        command.requestFingerprint;

    if (result.verified)
        result.editedRecordingKey =
            editedRecordingKey;

    result.reasonCode =
        result.verified
            ? "recording_cut_verified_replayed"
            : "recording_cut_pending";

    return result;
}

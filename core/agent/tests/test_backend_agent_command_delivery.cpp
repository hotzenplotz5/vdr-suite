#include "AccountabilityEventRepository.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "MediaImportOperationFence.h"
#include "AddonActivationIntentRepository.h"
#include "MediaImportRecordingEvidence.h"
#include "BackendAgentMediaImportAssignment.h"
#include "MutationOperationRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"
#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "BackendAccessPolicy.h"
#include "Database.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <sqlite3.h>
#include <thread>

namespace
{
int sqlCount(sqlite3* connection, const char* statement)
{
    sqlite3_stmt* query = nullptr;
    assert(sqlite3_prepare_v2(connection, statement, -1, &query, nullptr)
        == SQLITE_OK);
    assert(sqlite3_step(query) == SQLITE_ROW);
    const int result = sqlite3_column_int(query, 0);
    sqlite3_finalize(query);
    return result;
}

RequestSecurityContext context(ActorType type,const std::string& actor)
{
    RequestSecurityContext c;c.requestId="req_test";c.correlationId="corr_test";c.authenticationState=AuthenticationState::Authenticated;c.actor=ActorIdentity{actor,type,"test",true};c.device=DeviceIdentity{"dev_test",true};c.credential=CredentialIdentity{"cred_test",true,false,false};c.permissionGrantResolution=PermissionGrantResolutionState::Resolved;return c;
}
}
int main()
{
    Database db;assert(db.open(":memory:"));AccountabilityEventRepository accountability(db);BackendAgentRepository agents(db);BackendAgentCommandRepository commands(db);assert(accountability.ensureSchema());assert(agents.ensureSchema());assert(commands.ensureSchema());
    assert(db.execute("INSERT INTO backend_agents(agent_id,backend_id,actor_id,device_id,credential_id,credential_generation,agent_instance_id,backend_generation,protocol_version,software_version,heartbeat_sequence,capability_revision,last_connected_at,last_heartbeat_at,lease_expires_at,created_at,updated_at) VALUES('agt_test','default','actor_agent','dev_agent','cred_agent',1,'inst_test',7,'vdr-suite-agent/1','test',2,1,100,100,1000,1,1);"));
    BackendAgentCommandDeliveryService service(commands,agents,accountability);std::string reason;
    auto unsupported=service.assignProbe(context(ActorType::System,"system_admin"),"default",100,500,reason);assert(!unsupported.has_value()&&reason=="command_capability_required");
    BackendAgentCommandPollRequest poll;poll.backendId="default";poll.agentInstanceId="inst_test";poll.backendGeneration=7;poll.supportedCommandTypes={"probe.noop"};auto capabilityPoll=service.poll(context(ActorType::Agent,"actor_agent"),poll,101);assert(capabilityPoll.accepted&&!capabilityPoll.assignment.present);
    auto assignment=service.assignProbe(context(ActorType::System,"system_admin"),"default",102,500,reason);assert(assignment.has_value());assert(assignment->requestFingerprint==backendAgentCommandFingerprint(*assignment));
    auto polled=service.poll(context(ActorType::Agent,"actor_agent"),poll,103);assert(polled.accepted&&polled.assignment.present&&polled.assignment.commandId==assignment->commandId);
    auto duplicatePoll=service.poll(context(ActorType::Agent,"actor_agent"),poll,102);assert(duplicatePoll.assignment.present&&duplicatePoll.assignment.commandId==assignment->commandId);
    BackendAgentCommandReceipt receipt;receipt.commandId=assignment->commandId;receipt.requestFingerprint=assignment->requestFingerprint;receipt.jobId=assignment->jobId;receipt.attemptId=assignment->attemptId;receipt.claimEpoch=1;receipt.backendId="default";receipt.agentId="agt_test";receipt.agentInstanceId="inst_test";receipt.backendGeneration=7;receipt.receiptCategory="accepted";receipt.receivedAt=103;receipt.reasonCode="durably_recorded";auto receiptResult=service.receipt(context(ActorType::Agent,"actor_agent"),receipt,103);assert(receiptResult.accepted&&!receiptResult.replayed);auto receiptReplay=service.receipt(context(ActorType::Agent,"actor_agent"),receipt,104);assert(receiptReplay.accepted&&receiptReplay.replayed);
    BackendAgentCommandResult result;result.commandId=assignment->commandId;result.requestFingerprint=assignment->requestFingerprint;result.jobId=assignment->jobId;result.attemptId=assignment->attemptId;result.claimEpoch=1;result.backendId="default";result.agentId="agt_test";result.agentInstanceId="inst_test";result.backendGeneration=7;result.dispatchState="effect_reported";result.verificationState="not_required";result.resultCategory="succeeded";result.errorCategory="none";result.retryClassification="none";result.boundedDiagnostics="probe.noop completed without native side effect";result.completedAt=105;auto resultAck=service.result(context(ActorType::Agent,"actor_agent"),result,105);assert(resultAck.accepted&&!resultAck.replayed);auto resultReplay=service.result(context(ActorType::Agent,"actor_agent"),result,106);assert(resultReplay.accepted&&resultReplay.replayed);
    auto summary=service.summaryForBackend("default");assert(summary.present&&summary.state=="completed"&&summary.resultCategory=="succeeded"&&summary.deliveryCount==2&&summary.receiptReplayCount==1&&summary.resultReplayCount==1);
    assert(service.requestReplay(context(ActorType::System,"system_admin"),"default",assignment->commandId,107,reason));auto replayPoll=service.poll(context(ActorType::Agent,"actor_agent"),poll,108);assert(replayPoll.assignment.present&&replayPoll.assignment.commandId==assignment->commandId);summary=service.summaryForBackend("default");assert(summary.deliveryCount==3);
    std::atomic<bool> pollStarted{false};
    std::atomic<bool> pollFinished{false};
    BackendAgentCommandPollResult concurrentPoll;
    auto transactionLease=db.acquireTransactionLease();
    assert(db.execute("BEGIN IMMEDIATE;"));
    std::thread pollThread([&] {
        pollStarted.store(true);
        concurrentPoll=service.poll(
            context(ActorType::Agent,"actor_agent"),poll,109);
        pollFinished.store(true);
    });
    while(!pollStarted.load())std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const bool pollBlocked=!pollFinished.load();
    assert(db.execute("COMMIT;"));
    transactionLease.unlock();
    pollThread.join();
    assert(
        pollBlocked&&pollFinished.load()&&
        concurrentPoll.accepted);
    BackendAgentCommandPollRequest stale=poll;stale.backendGeneration=6;assert(!service.poll(context(ActorType::Agent,"actor_agent"),stale,109).accepted);
    assert(service.armFault(context(ActorType::System,"system_admin"),"default","receipt",110,reason));
    // P7 reconstructed pre-result-evidence schema (private in-memory DB).
    // The real command/result roundtrip above persisted this older-style row.
    assert(sqlCount(db.handle(),
        "SELECT count(*) FROM backend_agent_command_results "
        "WHERE dispatch_state='effect_reported' "
        "AND result_category='succeeded' "
        "AND bounded_diagnostics='probe.noop completed without native side effect';") == 1);
    assert(db.execute(
        "ALTER TABLE backend_agent_command_results "
        "DROP COLUMN result_evidence;"
        // Historical dormant-capability trigger name: initializer must clear it.
        "CREATE TRIGGER trg_backend_agent_timer_delete_dormant_capability "
        "AFTER INSERT ON backend_agent_command_results "
        "BEGIN SELECT 1; END;"));
    assert(sqlCount(db.handle(),
        "SELECT count(*) FROM pragma_table_info("
        "'backend_agent_command_results') WHERE name='result_evidence';") == 0);
    assert(sqlCount(db.handle(),
        "SELECT count(*) FROM sqlite_master WHERE name="
        "'trg_backend_agent_timer_delete_dormant_capability';") == 1);
    assert(commands.ensureSchema());  // actual C++ ALTER + retired trigger cleanup
    assert(commands.ensureSchema());  // idempotence
    assert(sqlCount(db.handle(),
        "SELECT count(*) FROM pragma_table_info("
        "'backend_agent_command_results') WHERE name='result_evidence';") == 1);
    assert(sqlCount(db.handle(),
        "SELECT count(*) FROM backend_agent_command_results "
        "WHERE dispatch_state='effect_reported' "
        "AND result_category='succeeded' "
        "AND bounded_diagnostics='probe.noop completed without native side effect' "
        "AND result_evidence='';") == 1);
    assert(sqlCount(db.handle(),
        "SELECT count(*) FROM sqlite_master WHERE name IN ("
        "'trg_backend_agent_timer_delete_dormant_capability',"
        "'trg_backend_agent_recording_marks_modify_dormant_capability',"
        "'trg_backend_agent_recording_cut_dormant_capability');") == 0);
    assert(sqlCount(db.handle(),
        "SELECT count(*) FROM pragma_foreign_key_check;") == 0);

    // Exercise SQLite online backup API only between isolated memory databases.
    Database snapshot;
    assert(snapshot.open(":memory:"));
    sqlite3_backup* backup =
        sqlite3_backup_init(snapshot.handle(), "main", db.handle(), "main");
    assert(backup != nullptr);
    assert(sqlite3_backup_step(backup, -1) == SQLITE_DONE);
    assert(sqlite3_backup_finish(backup) == SQLITE_OK);
    BackendAgentCommandRepository restored(snapshot);
    assert(restored.ensureSchema());
    assert(sqlCount(snapshot.handle(),
        "SELECT count(*) FROM backend_agent_command_results "
        "WHERE result_category='succeeded' AND result_evidence='';") == 1);
    assert(sqlCount(snapshot.handle(),
        "SELECT count(*) FROM pragma_foreign_key_check;") == 0);

    // A6: even a valid reserved add-on command and Agent receipt cannot
    // bypass the server-side operation/grant authority. In this fixture the
    // sole positive authorization is explicitly injected by trusted code.
    auto media = *assignment;
    media.requestId = "req_media";
    media.correlationId = "corr_media";
    media.operationId = "op_media";
    media.jobId = "job_media";
    media.attemptId = "att_media";
    media.claimEpoch = 3;
    media.commandId = "cmd_media";
    media.commandType = "addon.media.import";
    media.payloadVersion = 1;
    media.payload = "{\"sourceId\":\"local_token\"}";
    media.verificationPolicy = "reconciliation_required";
    media.assignedAt = 111;
    media.deadline = 500;
    media.requestFingerprint = backendAgentCommandFingerprint(media);
    assert(backendAgentCommandValidAssignment(media));
    assert(commands.insertAssignment(media));
    auto mediaReceipt = receipt;
    mediaReceipt.commandId = media.commandId;
    mediaReceipt.requestFingerprint = media.requestFingerprint;
    mediaReceipt.jobId = media.jobId;
    mediaReceipt.attemptId = media.attemptId;
    mediaReceipt.claimEpoch = media.claimEpoch;
    mediaReceipt.receivedAt = 112;

    const auto defaultDeny = service.receipt(
        context(ActorType::Agent, "actor_agent"), mediaReceipt, 112);
    assert(!defaultDeny.accepted &&
           defaultDeny.reasonCode == "media_import_server_fence_unavailable");
    assert(sqlCount(db.handle(),
        "SELECT COUNT(*) FROM backend_agent_command_receipts "
        "WHERE command_id='cmd_media';") == 0);

    int grantChecks = 0;
    bool grantAvailable = false;
    service.setMediaImportReceiptFenceCheck(
        [&](const BackendAgentCommandAssignment& reserved,
            const BackendAgentCommandReceipt& actual, std::string& gateReason) {
            ++grantChecks;
            assert(reserved.commandId == media.commandId);
            assert(reserved.operationId == media.operationId);
            assert(reserved.backendGeneration == media.backendGeneration);
            assert(reserved.claimEpoch == actual.claimEpoch);
            assert(reserved.requestFingerprint == actual.requestFingerprint);
            gateReason = grantAvailable ? "media_import_operation_validated" :
                "media_import_effective_grant_denied";
            return grantAvailable;
        });
    const auto denied = service.receipt(
        context(ActorType::Agent, "actor_agent"), mediaReceipt, 113);
    assert(!denied.accepted &&
           denied.reasonCode == "media_import_effective_grant_denied");
    assert(sqlCount(db.handle(),
        "SELECT COUNT(*) FROM backend_agent_command_receipts "
        "WHERE command_id='cmd_media';") == 0);
    grantAvailable = true;
    const auto acceptedMedia = service.receipt(
        context(ActorType::Agent, "actor_agent"), mediaReceipt, 114);
    assert(acceptedMedia.accepted && !acceptedMedia.replayed);
    const auto replayedMedia = service.receipt(
        context(ActorType::Agent, "actor_agent"), mediaReceipt, 115);
    assert(replayedMedia.accepted && replayedMedia.replayed);
    grantAvailable = false;
    const auto revokedMedia = service.receipt(
        context(ActorType::Agent, "actor_agent"), mediaReceipt, 116);
    assert(!revokedMedia.accepted);
    assert(grantChecks == 4);
    assert(sqlCount(db.handle(),
        "SELECT COUNT(*) FROM backend_agent_command_receipts "
        "WHERE command_id='cmd_media';") == 1);

    // A6: use the REAL persisted operation, immutable payload and effective
    // human-grant authorities behind the existing Agent receipt callback.
    vdrsuite::operations::MutationOperationRepository operations(db);
    SecurityIdentityRepository identities(db);
    SecurityPermissionGrantRepository mediaGrants(db);
    assert(operations.ensureSchema() && identities.ensureSchema() &&
           mediaGrants.ensureSchema());
    assert(db.execute(
        "INSERT INTO security_actors(actor_id,actor_type,display_name) "
        "VALUES('human_media','user','Media tester');"));

    vdrsuite::operations::MutationOperation op;
    op.operationId = media.operationId;
    op.idempotencyKey = "idem_media";
    op.actorId = "human_media";
    op.backendId = "default";
    op.backendGeneration = 7;
    op.resourceType = "MediaImportSource";
    op.resourceId = "local_token";
    op.expectedRevision = "1";
    op.actionFamily = "media.import";
    op.requestFingerprint = "logical_api_request_fingerprint";
    op.requestedAt = 100;
    op.deadline = 500;
    op.verificationPolicy =
        vdrsuite::operations::MutationOperationVerificationPolicy::
            reconciliationRequired;
    op.updatedAt = op.requestedAt;
    vdrsuite::operations::MutationOperationPayload payload;
    payload.operationId = op.operationId;
    payload.payloadType = "addon.media.import";
    payload.payloadVersion = 1;
    payload.payload = media.payload;
    payload.payloadFingerprint = "logical_payload_fingerprint";
    const auto reserved = operations.reserveWithPayload(op, payload);
    assert(reserved.ok());
    const auto dispatching = operations.transition(
        op.operationId, reserved.operation.operationRevision,
        vdrsuite::operations::MutationOperationState::accepted,
        vdrsuite::operations::MutationOperationState::dispatching,
        "", 110);
    assert(dispatching.ok());

    BackendRegistry registry;
    BackendNode writable;
    writable.backendId = "default";
    writable.accessMode = "read-write";
    registry.addBackend(writable);
    BackendRegistryService backendRegistry(registry);
    BackendAccessPolicy backendAccess;
    vdrsuite::addons::AddonActivationIntentRepository activation(db);
    assert(activation.ensureSchema());
    vdrsuite::addons::MediaImportOperationFence coreFence(
        operations, identities, mediaGrants, agents, commands,
        backendRegistry, backendAccess, activation);
    BackendAgentCommandPollRequest importPoll = poll;
    importPoll.supportedCommandTypes = {"probe.noop", "addon.media.import"};
    assert(service.poll(
        context(ActorType::Agent, "actor_agent"), importPoll, 117).accepted);

    std::string fenceReason;
    assert(!coreFence.check(media, mediaReceipt, 118, fenceReason) &&
           fenceReason == "media_import_permission_denied");
    assert(!coreFence.effectiveMediaImportCapability(
        "human_media", "default", 118, fenceReason) &&
        fenceReason == "media_import_permission_denied");
    // Neither role.admin nor generic wildcard can elevate add-on import.
    assert(mediaGrants.ensureGrant("human_media", "role.admin", "*"));
    assert(!coreFence.check(media, mediaReceipt, 118, fenceReason));
    assert(mediaGrants.ensureGrant(
        "human_media", "addons.media.import", "other-backend"));
    assert(!coreFence.check(media, mediaReceipt, 118, fenceReason));
    assert(mediaGrants.ensureGrant(
        "human_media", "addons.media.import", "default"));
    assert(!coreFence.check(media, mediaReceipt, 118, fenceReason) &&
           fenceReason == "media_import_activation_disabled");

    // Only a trusted administrative intent transition can enable dispatch.
    RequestSecurityContext admin = context(ActorType::User, "admin_media");
    admin.grants.push_back({"role.admin", "*"});
    vdrsuite::addons::RuntimeEvidence trusted;
    trusted.moduleId = "rectools";
    trusted.packageName = "vdr-suite-addon-media-tools";
    trusted.backendId = "default";
    trusted.installed = true;
    trusted.manifestValid = true;
    trusted.packageTrusted = true;
    trusted.versionCompatible = true;
    trusted.handlerRegistered = true;
    trusted.handlerHealthy = true;
    auto access = backendAccess.canWriteToBackend(backendRegistry, "default");
    const auto activated = activation.update(
        admin, {"rectools", "default", true, 0}, access, trusted);
    assert(activated.status == vdrsuite::addons::IntentStatus::ok);
    assert(coreFence.check(media, mediaReceipt, 118, fenceReason));
    assert(fenceReason == "media_import_operation_and_grant_verified");
    assert(coreFence.effectiveMediaImportCapability(
        "human_media", "default", 118, fenceReason));
    assert(fenceReason == "media_import_effective_capability_available");
    assert(!coreFence.effectiveMediaImportCapability(
        "human_media", "wrong-backend", 118, fenceReason));
    assert(!coreFence.effectiveMediaImportCapability(
        "human_media", "default", 1001, fenceReason));
    assert(!coreFence.effectiveMediaImportCapability(
        "admin_media", "default", 118, fenceReason));

    const auto disabled = activation.update(
        admin, {"rectools", "default", false, 1}, access, {});
    assert(disabled.status == vdrsuite::addons::IntentStatus::ok);
    assert(!coreFence.check(media, mediaReceipt, 118, fenceReason) &&
           fenceReason == "media_import_activation_disabled");
    assert(!coreFence.effectiveMediaImportCapability(
        "human_media", "default", 118, fenceReason) &&
           fenceReason == "media_import_activation_disabled");
    assert(activation.update(
        admin, {"rectools", "default", true, 2}, access, trusted).status ==
        vdrsuite::addons::IntentStatus::ok);
    assert(coreFence.check(media, mediaReceipt, 118, fenceReason));

    service.setMediaImportReceiptFenceCheck(
        [&](const BackendAgentCommandAssignment& original,
            const BackendAgentCommandReceipt& incoming,
            std::string& checkedReason) {
            return coreFence.check(original, incoming, 119, checkedReason);
        });
    const auto verifiedReplay = service.receipt(
        context(ActorType::Agent, "actor_agent"), mediaReceipt, 119);
    assert(verifiedReplay.accepted && verifiedReplay.replayed);
    assert(mediaGrants.revokeGrant(
        "human_media", "addons.media.import", "default"));
    assert(!coreFence.check(media, mediaReceipt, 120, fenceReason));
    assert(!coreFence.effectiveMediaImportCapability(
        "human_media", "default", 120, fenceReason));
    const auto revokedRealGrant = service.receipt(
        context(ActorType::Agent, "actor_agent"), mediaReceipt, 120);
    assert(!revokedRealGrant.accepted);

    assert(mediaGrants.ensureGrant(
        "human_media", "addons.media.import", "default"));
    auto tampered = media;
    tampered.payload = "{\"sourceId\":\"different_token\"}";
    tampered.requestFingerprint = backendAgentCommandFingerprint(tampered);
    assert(!coreFence.check(tampered, mediaReceipt, 120, fenceReason));
    assert(!coreFence.check(media, mediaReceipt, 501, fenceReason));
    assert(identities.revokeActor("human_media"));
    assert(!coreFence.check(media, mediaReceipt, 120, fenceReason));

    // Applying the durable Agent result uses ADR-0042's CAS transition.
    // A remux process exit never implies successful recording reconciliation.
    service.setMediaImportResultApply(
        [&](const BackendAgentCommandAssignment& original,
            const BackendAgentCommandResult& outcome,
            std::string& appliedReason) {
            return coreFence.applyResult(original, outcome, 121, appliedReason);
        });
    BackendAgentCommandResult mediaOutcome;
    mediaOutcome.commandId = media.commandId;
    mediaOutcome.requestFingerprint = media.requestFingerprint;
    mediaOutcome.jobId = media.jobId;
    mediaOutcome.attemptId = media.attemptId;
    mediaOutcome.claimEpoch = media.claimEpoch;
    mediaOutcome.backendId = media.backendId;
    mediaOutcome.agentId = media.agentId;
    mediaOutcome.agentInstanceId = media.agentInstanceId;
    mediaOutcome.backendGeneration = media.backendGeneration;
    mediaOutcome.dispatchState = "accepted_by_executor";
    mediaOutcome.verificationState = "outcome_unknown";
    mediaOutcome.resultCategory = "outcome_unknown";
    mediaOutcome.errorCategory = "none";
    mediaOutcome.retryClassification = "reconcile_only";
    mediaOutcome.boundedDiagnostics = "recording reconciliation pending";
    const std::string nativeImportedPath =
        "/srv/vdr/video/Imported/2026-10-21.18.00.00-0.rec";
    mediaOutcome.resultEvidence =
        vdrsuite::mediatools::recordingPathEvidence(nativeImportedPath);
    assert(vdrsuite::mediatools::validRecordingPathEvidence(
        mediaOutcome.resultEvidence));
    mediaOutcome.completedAt = 121;
    assert(backendAgentCommandValidResult(mediaOutcome));
    auto mediaApplied = service.result(
        context(ActorType::Agent, "actor_agent"), mediaOutcome, 121);
    assert(mediaApplied.accepted);
    const auto resultOperation = operations.findById(media.operationId);
    assert(resultOperation.ok());
    assert(resultOperation.operation.state ==
        vdrsuite::operations::MutationOperationState::outcomeUnknown);
    assert(resultOperation.operation.resultReference ==
        "agent.media.import:" + media.commandId);
    auto duplicateMediaOutcome = service.result(
        context(ActorType::Agent, "actor_agent"), mediaOutcome, 122);
    assert(duplicateMediaOutcome.accepted && duplicateMediaOutcome.replayed);
    assert(operations.findById(media.operationId).operation.operationRevision ==
           resultOperation.operation.operationRevision);
    auto fakeSuccess = mediaOutcome;
    fakeSuccess.dispatchState = "effect_reported";
    fakeSuccess.resultCategory = "succeeded";
    fakeSuccess.verificationState = "verified";
    fakeSuccess.retryClassification = "none";
    assert(!coreFence.applyResult(media, fakeSuccess, 123, fenceReason));

    // Provider-readback, NOT Agent exit, is the only path to succeeded.
    VdrRecordingCacheStatus readback;
    readback.backendId = "default";
    readback.state = "ready";
    readback.startedAt = "1970-01-01 00:02:04"; // after Agent result at 121
    readback.finishedAt = "1970-01-01 00:02:06";
    readback.totalCount = 1;
    VdrRecording imported;
    imported.id = "rec-imported";
    imported.backendId = "default";
    imported.backendNativeId = nativeImportedPath;
    imported.path = nativeImportedPath;
    imported.title = "Imported";
    std::vector<VdrRecording> actualRecordings{imported};
    auto readbackStatus = readback;
    readbackStatus.state = "stale";
    assert(coreFence.reconcileRecordingCache(
        "default", readbackStatus, actualRecordings, 128) == 0);
    readbackStatus = readback;
    readbackStatus.startedAt = "1970-01-01 00:02:01"; // not newer
    assert(coreFence.reconcileRecordingCache(
        "default", readbackStatus, actualRecordings, 128) == 0);
    auto wrongRecording = imported;
    wrongRecording.path = "/srv/vdr/video/Other/2026-10-21.18.00.00-0.rec";
    assert(coreFence.reconcileRecordingCache(
        "default", readback, {wrongRecording}, 128) == 0);
    assert(coreFence.reconcileRecordingCache(
        "wrong-backend", readback, actualRecordings, 128) == 0);
    assert(coreFence.reconcileRecordingCache(
        "default", readback, {imported, imported}, 128) == 0);
    assert(operations.findById(media.operationId).operation.state ==
        vdrsuite::operations::MutationOperationState::outcomeUnknown);
    assert(coreFence.reconcileRecordingCache(
        "default", readback, actualRecordings, 128) == 1);
    const auto reconciledOp = operations.findById(media.operationId);
    assert(reconciledOp.ok() && reconciledOp.operation.state ==
        vdrsuite::operations::MutationOperationState::succeeded);
    assert(coreFence.reconcileRecordingCache(
        "default", readback, actualRecordings, 129) == 0);
    assert(operations.findById(media.operationId).operation.operationRevision ==
           reconciledOp.operation.operationRevision);
    const auto completedReplay = service.result(
        context(ActorType::Agent, "actor_agent"), mediaOutcome, 130);
    assert(completedReplay.accepted && completedReplay.replayed);
    assert(operations.findById(media.operationId).operation.operationRevision ==
           reconciledOp.operation.operationRevision);


    // A6: the actual Agent command reservation uses only the existing
    // persistent ADR-0042 operation + Agent command table, including replay.
    assert(db.execute(
        "INSERT INTO security_actors(actor_id,actor_type,display_name) "
        "VALUES('human_assign','user','Assignment tester');"));
    assert(mediaGrants.ensureGrant(
        "human_assign", "addons.media.import", "default"));
    auto queued = op;
    queued.operationId = "op_media_assign";
    queued.idempotencyKey = "idem_media_assign";
    queued.actorId = "human_assign";
    queued.resourceId = "queued_token";
    queued.requestFingerprint = "logical_queued_request";
    queued.requestedAt = 125;
    queued.updatedAt = queued.requestedAt;
    auto queuedPayload = payload;
    queuedPayload.operationId = queued.operationId;
    queuedPayload.payload = "{\"sourceId\":\"queued_token\"}";
    queuedPayload.payloadFingerprint = "logical_queued_payload";
    assert(operations.reserveWithPayload(queued, queuedPayload).ok());

    vdrsuite::agent::BackendAgentMediaImportAssignmentService assigner(
        operations, commands, agents, coreFence);
    const auto authorized = assigner.assign(
        context(ActorType::System, "system_admin"), queued.operationId, 130);
    assert(authorized.accepted && !authorized.replayed);
    assert(backendAgentCommandValidAssignment(authorized.assignment));
    assert(authorized.assignment.payload == queuedPayload.payload);
    assert(authorized.assignment.claimEpoch == 1);
    assert(operations.findById(queued.operationId).operation.state ==
        vdrsuite::operations::MutationOperationState::dispatching);
    const auto replayed = assigner.assign(
        context(ActorType::System, "system_admin"), queued.operationId, 131);
    assert(replayed.accepted && replayed.replayed &&
           replayed.assignment.commandId == authorized.assignment.commandId);
    assert(!assigner.assign(
        context(ActorType::User, "human_assign"), queued.operationId, 132
        ).accepted);

    // Crash after the Core operation CAS but before command persistence:
    // the same operation can complete its reservation with no new DB/queue.
    queued.operationId = "op_media_crash";
    queued.idempotencyKey = "idem_media_crash";
    queued.requestFingerprint = "logical_crash_request";
    queuedPayload.operationId = queued.operationId;
    queuedPayload.payloadFingerprint = "logical_crash_payload";
    const auto crashReserved = operations.reserveWithPayload(
        queued, queuedPayload);
    assert(crashReserved.ok());
    assert(operations.transition(
        queued.operationId, crashReserved.operation.operationRevision,
        vdrsuite::operations::MutationOperationState::accepted,
        vdrsuite::operations::MutationOperationState::dispatching,
        "", 129).ok());
    const auto recovered = assigner.assign(
        context(ActorType::System, "system_admin"), queued.operationId, 133);
    assert(recovered.accepted && !recovered.replayed);
    assert(assigner.assign(
        context(ActorType::System, "system_admin"), queued.operationId, 134
        ).replayed);



    std::cout << "P7_AGENT_RESULT_EVIDENCE_MIGRATION=PASS\n";
    std::cout<<"Backend Agent command delivery tests passed"<<std::endl;return 0;
}

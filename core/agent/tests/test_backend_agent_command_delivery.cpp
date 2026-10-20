#include "AccountabilityEventRepository.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerModifyExecutor.h"
#include "BackendAgentNativeTimerModifyLocalState.h"
#include "BackendAgentNativeTimerModifyPayload.h"
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
    // A native UPDATE was received under generation 7. The Agent persisted
    // its uncertain result locally; heartbeat advanced the Agent to
    // generation 8 before the result could be delivered. The old command
    // identity/evidence must be accepted once, without opening stale polling,
    // receipts, or success results.
    using namespace vdrsuite::agent;
    BackendAgentNativeTimerModifyPayload modify;
    modify.kind = BackendAgentNativeTimerModifyKind::update;
    modify.operationRevision = "1";
    modify.timerAssignmentId = "assignment_historical_modify";
    modify.expectedAssignmentRevision = "1";
    modify.expectedIntentRevision = "1";
    modify.assignmentEpoch = 1;
    modify.nativeTimerBindingId = "binding_historical_modify";
    modify.expectedBindingRevision = "1";
    modify.backendId = "default";
    modify.backendGeneration = 7;
    modify.backendNativeTimerId = "native_historical_modify";
    modify.expectedCurrentFingerprint = "sha256:" + std::string(64, 'a');
    modify.specification.channelId = "S19.2E-1-1019-10301";
    modify.specification.title = "Test MODIFY";
    modify.specification.day = "2026-11-20";
    modify.specification.startTime = "1200";
    modify.specification.endTime = "1230";
    modify.expectedSpecificationFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(modify.specification);
    modify.controlPlaneClaimedAt = 110;
    auto& provider = modify.localProviderSelection;
    provider.backendId = "default";
    provider.authorityDomain = "vdr.timer";
    provider.providerId = "suitebridge:local";
    provider.providerKind = "suitebridge";
    provider.ownershipGeneration = 1;
    provider.providerInstanceEpoch = "pie_historical";
    provider.providerGeneration = 1;
    provider.capabilityRevision = 1;
    provider.requiredCapability = "vdr.timer.update";

    BackendAgentCommandAssignment historical;
    historical.present = true;
    historical.requestId = "req_historical_modify";
    historical.correlationId = historical.requestId;
    historical.operationId = "op_historical_modify";
    historical.jobId = "job_historical_modify";
    historical.attemptId = "att_historical_modify";
    historical.claimEpoch = 1;
    historical.commandId = "cmd_historical_modify";
    historical.backendId = "default";
    historical.agentId = "agt_test";
    historical.agentInstanceId = "inst_test";
    historical.backendGeneration = 7;
    historical.commandType = kBackendAgentNativeTimerUpdateCommandType;
    historical.payloadVersion = kBackendAgentNativeTimerModifyPayloadVersion;
    historical.payload = backendAgentNativeTimerModifyPayload(modify);
    historical.verificationPolicy = "readback_required";
    historical.assignedAt = 110;
    historical.deadline = 700;
    historical.requestFingerprint = backendAgentCommandFingerprint(historical);
    assert(!historical.payload.empty());
    assert(commands.insertAssignment(historical, &provider));

    BackendAgentCommandReceipt received;
    received.commandId = historical.commandId;
    received.requestFingerprint = historical.requestFingerprint;
    received.jobId = historical.jobId;
    received.attemptId = historical.attemptId;
    received.claimEpoch = historical.claimEpoch;
    received.backendId = historical.backendId;
    received.agentId = historical.agentId;
    received.agentInstanceId = historical.agentInstanceId;
    received.backendGeneration = historical.backendGeneration;
    received.receiptCategory = "accepted";
    received.receivedAt = 111;
    received.reasonCode = "durably_recorded";
    assert(service.receipt(context(ActorType::Agent,"actor_agent"), received,111).accepted);

    BackendAgentNativeTimerModifyLocalState starting;
    assert(backendAgentNativeTimerModifyPrepareLocalStarting(
        historical, 112, starting, reason));
    const auto recovery = backendAgentNativeTimerModifyRecoverLocalState(
        starting, "default", "agt_test", "inst_test", 7, 113);
    assert(recovery.decision ==
        BackendAgentNativeTimerModifyRecoveryDecision::reconcileOnly);
    BackendAgentCommandResult uncertain;
    uncertain.commandId = historical.commandId;
    uncertain.requestFingerprint = historical.requestFingerprint;
    uncertain.jobId = historical.jobId;
    uncertain.attemptId = historical.attemptId;
    uncertain.claimEpoch = historical.claimEpoch;
    uncertain.backendId = historical.backendId;
    uncertain.agentId = historical.agentId;
    uncertain.agentInstanceId = historical.agentInstanceId;
    uncertain.backendGeneration = historical.backendGeneration;
    uncertain.dispatchState = "starting";
    uncertain.verificationState = "outcome_unknown";
    uncertain.resultCategory = "outcome_unknown";
    uncertain.errorCategory = "executor_unknown";
    uncertain.retryClassification = "reconcile_only";
    uncertain.boundedDiagnostics = "native Timer modify outcome unknown";
    uncertain.resultEvidence = backendAgentNativeTimerModifyResultEvidence(
        recovery.evidence, historical, reason);
    uncertain.completedAt = recovery.evidence.completedAt;
    assert(!uncertain.resultEvidence.empty());

    assert(db.execute("UPDATE backend_agents SET agent_instance_id='inst_new',"
        "backend_generation=8 WHERE agent_id='agt_test';"));
    assert(!service.poll(context(ActorType::Agent,"actor_agent"),poll,115).accepted);
    assert(!service.receipt(
        context(ActorType::Agent,"actor_agent"),received,115).accepted);
    assert(!service.result(context(ActorType::Agent,"actor_agent"),result,115).accepted);
    assert(!service.result(
        context(ActorType::Agent,"different_actor"),uncertain,115).accepted);
    const auto staleAccepted =
        service.result(context(ActorType::Agent,"actor_agent"),uncertain,115);
    assert(staleAccepted.accepted && !staleAccepted.replayed);
    const auto staleReplayed =
        service.result(context(ActorType::Agent,"actor_agent"),uncertain,116);
    assert(staleReplayed.accepted && staleReplayed.replayed);
    const auto staleSummary = service.summaryForBackend("default");
    assert(staleSummary.present &&
        staleSummary.commandId == historical.commandId &&
        staleSummary.state == "waiting_reconciliation" &&
        staleSummary.resultCategory == "outcome_unknown");
    assert(sqlCount(db.handle(),
        "SELECT count(*) FROM backend_agent_command_results "
        "WHERE command_id='cmd_historical_modify'") == 1);
    std::cout << "HISTORICAL_MODIFY_RESULT_GENERATION_RECOVERY=PASS\\n";

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
    std::cout << "P7_AGENT_RESULT_EVIDENCE_MIGRATION=PASS\n";
    std::cout<<"Backend Agent command delivery tests passed"<<std::endl;return 0;
}

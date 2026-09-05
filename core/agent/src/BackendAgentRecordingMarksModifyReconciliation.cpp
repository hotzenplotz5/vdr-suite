#include "BackendAgentCommandDelivery.h"

#include "BackendAgentRecordingMarksModify.h"
#include "BackendAgentRecordingMarksModifyPayload.h"
#include "Database.h"

#include <sqlite3.h>

#include <string>
#include <utility>
#include <vector>

namespace
{
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool bindInt(sqlite3_stmt* statement, int index, std::int64_t value)
{
    return sqlite3_bind_int64(statement, index, value) == SQLITE_OK;
}

std::string text(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value
        ? std::string(reinterpret_cast<const char*>(value))
        : std::string();
}

BackendAgentCommandAssignment readAssignment(sqlite3_stmt* statement)
{
    BackendAgentCommandAssignment assignment;
    assignment.present = true;
    assignment.protocolVersion = text(statement, 0);
    assignment.requestId = text(statement, 1);
    assignment.correlationId = text(statement, 2);
    assignment.operationId = text(statement, 3);
    assignment.jobId = text(statement, 4);
    assignment.attemptId = text(statement, 5);
    assignment.claimEpoch = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 6));
    assignment.commandId = text(statement, 7);
    assignment.backendId = text(statement, 8);
    assignment.agentId = text(statement, 9);
    assignment.agentInstanceId = text(statement, 10);
    assignment.backendGeneration = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 11));
    assignment.commandType = text(statement, 12);
    assignment.payloadVersion = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 13));
    assignment.payload = text(statement, 14);
    assignment.requestFingerprint = text(statement, 15);
    assignment.verificationPolicy = text(statement, 16);
    assignment.assignedAt = sqlite3_column_int64(statement, 17);
    assignment.deadline = sqlite3_column_int64(statement, 18);
    return assignment;
}

constexpr const char* AssignmentColumns =
    "c.protocol_version,c.request_id,c.correlation_id,c.operation_id,"
    "c.job_id,c.attempt_id,c.claim_epoch,c.command_id,c.backend_id,"
    "c.agent_id,c.agent_instance_id,c.backend_generation,c.command_type,"
    "c.payload_version,c.payload,c.request_fingerprint,c.verification_policy,"
    "c.assigned_at,c.deadline";

bool readVerification(
    sqlite3_stmt* statement,
    BackendAgentRecordingMarksModifyVerification& verification)
{
    verification = {};
    verification.present = true;
    verification.commandId = text(statement, 0);
    verification.requestFingerprint = text(statement, 1);
    verification.operationId = text(statement, 2);
    verification.backendId = text(statement, 3);
    verification.recordingKey = text(statement, 4);
    verification.expectedMarksRevision = text(statement, 5);
    verification.canonicalMarksRevision = text(statement, 6);
    verification.verifiedAt = sqlite3_column_int64(statement, 7);
    return backendAgentCommandSafeIdentifier(verification.commandId) &&
        backendAgentCommandSafeIdentifier(verification.requestFingerprint) &&
        backendAgentCommandSafeIdentifier(verification.operationId) &&
        backendAgentCommandSafeIdentifier(verification.backendId) &&
        vdrsuite::agent::backendAgentRecordingMarksModifyRevisionTokenValid(
            verification.recordingKey) &&
        vdrsuite::agent::backendAgentRecordingMarksModifyRevisionTokenValid(
            verification.expectedMarksRevision) &&
        vdrsuite::agent::backendAgentRecordingMarksModifyRevisionTokenValid(
            verification.canonicalMarksRevision) &&
        verification.canonicalMarksRevision != verification.expectedMarksRevision &&
        verification.verifiedAt > 0;
}

bool exactVerificationRequest(
    const BackendAgentRecordingMarksModifyVerification& verification,
    const std::string& commandId,
    const std::string& requestFingerprint,
    const std::string& recordingKey,
    const std::string& expectedMarksRevision,
    const std::string& canonicalMarksRevision)
{
    return verification.present &&
        verification.commandId == commandId &&
        verification.requestFingerprint == requestFingerprint &&
        verification.recordingKey == recordingKey &&
        verification.expectedMarksRevision == expectedMarksRevision &&
        verification.canonicalMarksRevision == canonicalMarksRevision;
}
}

bool BackendAgentCommandRepository::ensureRecordingMarksModifyReconciliationSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_agent_recording_marks_modify_readbacks ("
        "command_id TEXT PRIMARY KEY,request_fingerprint TEXT NOT NULL,"
        "operation_id TEXT NOT NULL,backend_id TEXT NOT NULL,"
        "recording_key TEXT NOT NULL,expected_marks_revision TEXT NOT NULL,"
        "canonical_marks_revision TEXT NOT NULL,verified_at INTEGER NOT NULL,"
        "FOREIGN KEY(command_id) REFERENCES backend_agent_commands(command_id));") &&
        database_.execute(
            "CREATE UNIQUE INDEX IF NOT EXISTS "
            "idx_backend_agent_recording_marks_modify_readback_operation "
            "ON backend_agent_recording_marks_modify_readbacks(backend_id,operation_id);");
}

std::vector<BackendAgentRecordingMarksModifyReconciliationCandidate>
BackendAgentCommandRepository::recordingMarksModifyReconciliationCandidates() const
{
    using namespace vdrsuite::agent;
    std::vector<BackendAgentRecordingMarksModifyReconciliationCandidate> candidates;
    sqlite3_stmt* statement = nullptr;
    const std::string sql = std::string("SELECT ") + AssignmentColumns +
        ",x.completed_at FROM backend_agent_commands c "
        "JOIN backend_agent_command_results x ON x.command_id=c.command_id "
        "WHERE c.command_type='vdr.recording.marks.modify' "
        "AND c.state='waiting_reconciliation' "
        "AND x.dispatch_state='accepted_by_executor' "
        "AND x.verification_state='outcome_unknown' "
        "AND x.result_category='outcome_unknown' "
        "AND x.retry_classification='reconcile_only' "
        "ORDER BY c.assigned_at,c.command_id LIMIT 64;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK)
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return candidates;
    }

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        BackendAgentRecordingMarksModifyReconciliationCandidate candidate;
        candidate.assignment = readAssignment(statement);
        candidate.executorCompletedAt = sqlite3_column_int64(statement, 19);

        BackendAgentRecordingMarksModifyPayload payload;
        std::string reasonCode;
        if (!backendAgentCommandValidAssignment(candidate.assignment) ||
            candidate.assignment.commandType !=
                kBackendAgentRecordingMarksModifyCommandType ||
            candidate.assignment.verificationPolicy != "readback_required" ||
            !backendAgentRecordingMarksModifyParsePayload(
                candidate.assignment.payload, payload, reasonCode) ||
            payload.backendId != candidate.assignment.backendId ||
            payload.backendGeneration != candidate.assignment.backendGeneration ||
            candidate.executorCompletedAt < candidate.assignment.assignedAt ||
            candidate.executorCompletedAt < payload.controlPlaneClaimedAt)
        {
            continue;
        }

        candidate.recordingKey = payload.recordingKey;
        candidate.expectedMarksRevision = payload.expectedMarksRevision;
        candidates.push_back(std::move(candidate));
    }
    sqlite3_finalize(statement);
    return candidates;
}

BackendAgentRecordingMarksModifyVerification
BackendAgentCommandRepository::recordingMarksModifyVerificationForOperation(
    const std::string& backendId,
    const std::string& operationId) const
{
    BackendAgentRecordingMarksModifyVerification verification;
    if (!backendAgentCommandSafeIdentifier(backendId) ||
        !backendAgentCommandSafeIdentifier(operationId))
    {
        return verification;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT r.command_id,r.request_fingerprint,r.operation_id,r.backend_id,"
        "r.recording_key,r.expected_marks_revision,r.canonical_marks_revision,"
        "r.verified_at FROM backend_agent_recording_marks_modify_readbacks r "
        "JOIN backend_agent_commands c ON c.command_id=r.command_id "
        "WHERE r.backend_id=? AND r.operation_id=? "
        "AND c.command_type='vdr.recording.marks.modify' AND c.state='completed' "
        "LIMIT 1;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, backendId) ||
        !bindText(statement, 2, operationId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return verification;
    }

    if (sqlite3_step(statement) == SQLITE_ROW &&
        !readVerification(statement, verification))
    {
        verification = {};
    }
    sqlite3_finalize(statement);
    return verification;
}

bool BackendAgentCommandRepository::verifyRecordingMarksModifyReadback(
    const std::string& commandId,
    const std::string& requestFingerprint,
    const std::string& recordingKey,
    const std::string& expectedMarksRevision,
    const std::string& canonicalMarksRevision,
    std::int64_t observedAt,
    BackendAgentRecordingMarksModifyVerification& verification,
    std::string& reasonCode)
{
    using namespace vdrsuite::agent;
    verification = {};
    if (!backendAgentCommandSafeIdentifier(commandId) ||
        !backendAgentCommandSafeIdentifier(requestFingerprint) ||
        !backendAgentRecordingMarksModifyRevisionTokenValid(recordingKey) ||
        !backendAgentRecordingMarksModifyRevisionTokenValid(expectedMarksRevision) ||
        !backendAgentRecordingMarksModifyRevisionTokenValid(canonicalMarksRevision) ||
        canonicalMarksRevision == expectedMarksRevision || observedAt <= 0)
    {
        reasonCode = "recording_marks_modify_readback_invalid";
        return false;
    }

    sqlite3_stmt* existingStatement = nullptr;
    const char* existingSql =
        "SELECT command_id,request_fingerprint,operation_id,backend_id,"
        "recording_key,expected_marks_revision,canonical_marks_revision,verified_at "
        "FROM backend_agent_recording_marks_modify_readbacks WHERE command_id=?;";
    if (sqlite3_prepare_v2(
            database_.handle(), existingSql, -1,
            &existingStatement, nullptr) != SQLITE_OK ||
        !bindText(existingStatement, 1, commandId))
    {
        if (existingStatement != nullptr) sqlite3_finalize(existingStatement);
        reasonCode = "recording_marks_modify_readback_repository_error";
        return false;
    }
    if (sqlite3_step(existingStatement) == SQLITE_ROW)
    {
        const bool valid = readVerification(existingStatement, verification);
        sqlite3_finalize(existingStatement);
        if (valid && exactVerificationRequest(
                verification,
                commandId,
                requestFingerprint,
                recordingKey,
                expectedMarksRevision,
                canonicalMarksRevision))
        {
            reasonCode = "recording_marks_modify_readback_replayed";
            return true;
        }
        verification = {};
        reasonCode = "recording_marks_modify_readback_conflict";
        return false;
    }
    sqlite3_finalize(existingStatement);

    std::string providerReason;
    if (!localProviderSelectionCurrent(commandId, providerReason))
    {
        reasonCode = providerReason.empty()
            ? "recording_marks_modify_provider_selection_stale"
            : providerReason;
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const std::string query = std::string("SELECT ") + AssignmentColumns +
        ",c.state,x.dispatch_state,x.verification_state,x.result_category,"
        "x.retry_classification,x.completed_at "
        "FROM backend_agent_commands c "
        "JOIN backend_agent_command_results x ON x.command_id=c.command_id "
        "WHERE c.command_id=?;";
    if (sqlite3_prepare_v2(
            database_.handle(), query.c_str(), -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, commandId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        reasonCode = "recording_marks_modify_readback_repository_error";
        return false;
    }
    if (sqlite3_step(statement) != SQLITE_ROW)
    {
        sqlite3_finalize(statement);
        reasonCode = "recording_marks_modify_readback_candidate_not_found";
        return false;
    }

    const BackendAgentCommandAssignment assignment = readAssignment(statement);
    const std::string commandState = text(statement, 19);
    const std::string dispatchState = text(statement, 20);
    const std::string verificationState = text(statement, 21);
    const std::string resultCategory = text(statement, 22);
    const std::string retryClassification = text(statement, 23);
    const std::int64_t executorCompletedAt = sqlite3_column_int64(statement, 24);
    sqlite3_finalize(statement);

    BackendAgentRecordingMarksModifyPayload payload;
    std::string payloadReason;
    if (!backendAgentCommandValidAssignment(assignment) ||
        assignment.commandType != kBackendAgentRecordingMarksModifyCommandType ||
        assignment.verificationPolicy != "readback_required" ||
        assignment.requestFingerprint != requestFingerprint ||
        commandState != "waiting_reconciliation" ||
        dispatchState != "accepted_by_executor" ||
        verificationState != "outcome_unknown" ||
        resultCategory != "outcome_unknown" ||
        retryClassification != "reconcile_only" ||
        !backendAgentRecordingMarksModifyParsePayload(
            assignment.payload, payload, payloadReason) ||
        payload.recordingKey != recordingKey ||
        payload.expectedMarksRevision != expectedMarksRevision ||
        payload.backendId != assignment.backendId ||
        payload.backendGeneration != assignment.backendGeneration ||
        observedAt < executorCompletedAt ||
        observedAt < payload.controlPlaneClaimedAt)
    {
        reasonCode = "recording_marks_modify_readback_candidate_conflict";
        return false;
    }

    const auto selectedProvider = localProviderSelectionForCommand(commandId);
    if (!selectedProvider.has_value())
    {
        reasonCode = "recording_marks_modify_provider_selection_required";
        return false;
    }
    if (selectedProvider->backendId != assignment.backendId ||
        selectedProvider->authorityDomain !=
            kBackendAgentRecordingMarksModifyAuthorityDomain ||
        selectedProvider->providerId !=
            kBackendAgentRecordingMarksModifyProviderId ||
        selectedProvider->providerKind !=
            kBackendAgentRecordingMarksModifyProviderKind ||
        selectedProvider->requiredCapability !=
            kBackendAgentRecordingMarksModifyCapability)
    {
        reasonCode = "recording_marks_modify_provider_selection_mismatch";
        return false;
    }

    const auto currentProvider = selectLocalProvider(
        assignment.backendId,
        assignment.agentId,
        assignment.agentInstanceId,
        assignment.backendGeneration,
        kBackendAgentRecordingMarksModifyAuthorityDomain,
        kBackendAgentRecordingMarksModifyCapability,
        providerReason);
    if (!currentProvider.has_value())
    {
        reasonCode = providerReason.empty()
            ? "recording_marks_modify_provider_selection_stale"
            : providerReason;
        return false;
    }
    if (!backendAgentLocalProviderSameFence(*selectedProvider, *currentProvider))
    {
        reasonCode = "recording_marks_modify_provider_selection_stale";
        return false;
    }

    auto transactionLease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE;"))
    {
        reasonCode = "recording_marks_modify_readback_repository_error";
        return false;
    }

    sqlite3_stmt* insert = nullptr;
    const char* insertSql =
        "INSERT INTO backend_agent_recording_marks_modify_readbacks("
        "command_id,request_fingerprint,operation_id,backend_id,recording_key,"
        "expected_marks_revision,canonical_marks_revision,verified_at) "
        "VALUES(?,?,?,?,?,?,?,?);";
    bool ok = sqlite3_prepare_v2(
            database_.handle(), insertSql, -1, &insert, nullptr) == SQLITE_OK &&
        bindText(insert, 1, assignment.commandId) &&
        bindText(insert, 2, assignment.requestFingerprint) &&
        bindText(insert, 3, assignment.operationId) &&
        bindText(insert, 4, assignment.backendId) &&
        bindText(insert, 5, recordingKey) &&
        bindText(insert, 6, expectedMarksRevision) &&
        bindText(insert, 7, canonicalMarksRevision) &&
        bindInt(insert, 8, observedAt) &&
        sqlite3_step(insert) == SQLITE_DONE;
    if (insert != nullptr) sqlite3_finalize(insert);

    sqlite3_stmt* update = nullptr;
    const char* updateSql =
        "UPDATE backend_agent_commands SET state='completed',updated_at=? "
        "WHERE command_id=? AND state='waiting_reconciliation';";
    ok = ok && sqlite3_prepare_v2(
            database_.handle(), updateSql, -1, &update, nullptr) == SQLITE_OK &&
        bindInt(update, 1, observedAt) &&
        bindText(update, 2, assignment.commandId) &&
        sqlite3_step(update) == SQLITE_DONE &&
        sqlite3_changes(database_.handle()) == 1;
    if (update != nullptr) sqlite3_finalize(update);

    if (!ok || !database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        reasonCode = "recording_marks_modify_readback_repository_error";
        return false;
    }

    verification.present = true;
    verification.commandId = assignment.commandId;
    verification.requestFingerprint = assignment.requestFingerprint;
    verification.operationId = assignment.operationId;
    verification.backendId = assignment.backendId;
    verification.recordingKey = recordingKey;
    verification.expectedMarksRevision = expectedMarksRevision;
    verification.canonicalMarksRevision = canonicalMarksRevision;
    verification.verifiedAt = observedAt;
    reasonCode = "recording_marks_modify_readback_verified";
    return true;
}
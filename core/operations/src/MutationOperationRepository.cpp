#include "MutationOperationRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::operations
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxResultReferenceLength = 512;

bool safeIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool bindInt64(sqlite3_stmt* statement, int index, std::int64_t value)
{
    return sqlite3_bind_int64(statement, index, value) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string()
        : std::string(reinterpret_cast<const char*>(value));
}

bool parseRevision(const std::string& token, std::int64_t& value)
{
    if (token.empty()) return false;
    value = 0;
    for (char ch : token)
    {
        if (ch < '0' || ch > '9') return false;
        const std::int64_t digit = ch - '0';
        if (value > (std::numeric_limits<std::int64_t>::max() - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    return value > 0;
}

MutationOperationRepositoryResult statusResult(
    MutationOperationRepositoryStatus status)
{
    MutationOperationRepositoryResult result;
    result.status = status;
    return result;
}

MutationOperationRepositoryResult operationResult(
    MutationOperationRepositoryStatus status,
    const MutationOperation& operation)
{
    MutationOperationRepositoryResult result;
    result.status = status;
    result.operation = operation;
    return result;
}

bool readOperation(sqlite3_stmt* statement, MutationOperation& operation)
{
    operation.operationId = columnText(statement, 0);
    operation.operationRevision = columnText(statement, 1);
    operation.idempotencyKey = columnText(statement, 2);
    operation.actorId = columnText(statement, 3);
    operation.backendId = columnText(statement, 4);
    const sqlite3_int64 backendGeneration = sqlite3_column_int64(statement, 5);
    if (backendGeneration <= 0) return false;
    operation.backendGeneration = static_cast<std::uint64_t>(backendGeneration);
    operation.resourceType = columnText(statement, 6);
    operation.resourceId = columnText(statement, 7);
    operation.expectedRevision = columnText(statement, 8);
    operation.actionFamily = columnText(statement, 9);
    operation.requestFingerprint = columnText(statement, 10);
    operation.requestedAt = sqlite3_column_int64(statement, 11);
    operation.deadline = sqlite3_column_int64(statement, 12);
    if (!mutationOperationVerificationPolicyFromName(
            columnText(statement, 13), operation.verificationPolicy) ||
        !mutationOperationStateFromName(columnText(statement, 14), operation.state))
        return false;
    operation.resultReference = columnText(statement, 15);
    operation.updatedAt = sqlite3_column_int64(statement, 16);
    return mutationOperationValidDurable(operation);
}

constexpr const char* kColumns =
    "operation_id,operation_revision,idempotency_key,actor_id,backend_id,"
    "backend_generation,resource_type,resource_id,expected_revision,action_family,"
    "request_fingerprint,requested_at,deadline,verification_policy,state,"
    "result_reference,updated_at";

bool selectById(
    Database& database,
    const std::string& operationId,
    MutationOperation& operation,
    bool& found)
{
    found = false;
    sqlite3_stmt* statement = nullptr;
    const std::string sql = std::string("SELECT ") + kColumns +
        " FROM mutation_operations WHERE operation_id=? LIMIT 1;";
    if (sqlite3_prepare_v2(database.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, operationId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return false;
    }
    const int step = sqlite3_step(statement);
    if (step == SQLITE_ROW)
    {
        found = readOperation(statement, operation);
        sqlite3_finalize(statement);
        return found;
    }
    sqlite3_finalize(statement);
    return step == SQLITE_DONE;
}

bool selectByScope(
    Database& database,
    const std::string& actorId,
    const std::string& backendId,
    const std::string& resourceType,
    const std::string& resourceId,
    const std::string& actionFamily,
    const std::string& idempotencyKey,
    MutationOperation& operation,
    bool& found)
{
    found = false;
    sqlite3_stmt* statement = nullptr;
    const std::string sql = std::string("SELECT ") + kColumns +
        " FROM mutation_operations WHERE actor_id=? AND backend_id=? AND "
        "resource_type=? AND resource_id=? AND action_family=? AND idempotency_key=? LIMIT 1;";
    if (sqlite3_prepare_v2(database.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, actorId) ||
        !bindText(statement, 2, backendId) ||
        !bindText(statement, 3, resourceType) ||
        !bindText(statement, 4, resourceId) ||
        !bindText(statement, 5, actionFamily) ||
        !bindText(statement, 6, idempotencyKey))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return false;
    }
    const int step = sqlite3_step(statement);
    if (step == SQLITE_ROW)
    {
        found = readOperation(statement, operation);
        sqlite3_finalize(statement);
        return found;
    }
    sqlite3_finalize(statement);
    return step == SQLITE_DONE;
}

bool sameLogicalOperation(
    const MutationOperation& left,
    const MutationOperation& right)
{
    return left.operationId == right.operationId &&
        left.idempotencyKey == right.idempotencyKey &&
        left.actorId == right.actorId &&
        left.backendId == right.backendId &&
        left.backendGeneration == right.backendGeneration &&
        left.resourceType == right.resourceType &&
        left.resourceId == right.resourceId &&
        left.expectedRevision == right.expectedRevision &&
        left.actionFamily == right.actionFamily &&
        left.requestFingerprint == right.requestFingerprint &&
        left.requestedAt == right.requestedAt &&
        left.deadline == right.deadline &&
        left.verificationPolicy == right.verificationPolicy;
}
}

MutationOperationRepository::MutationOperationRepository(Database& database)
    : database_(database)
{
}

bool MutationOperationRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS mutation_operations ("
        "operation_id TEXT PRIMARY KEY,"
        "operation_revision INTEGER NOT NULL,"
        "idempotency_key TEXT NOT NULL,actor_id TEXT NOT NULL,backend_id TEXT NOT NULL,"
        "backend_generation INTEGER NOT NULL,resource_type TEXT NOT NULL,resource_id TEXT NOT NULL,"
        "expected_revision TEXT NOT NULL,action_family TEXT NOT NULL,request_fingerprint TEXT NOT NULL,"
        "requested_at INTEGER NOT NULL,deadline INTEGER NOT NULL,verification_policy TEXT NOT NULL,"
        "state TEXT NOT NULL,result_reference TEXT NOT NULL,updated_at INTEGER NOT NULL,"
        "CHECK(operation_revision>0),CHECK(backend_generation>0),"
        "CHECK(verification_policy IN ('none','readback_required','event_confirmation','reconciliation_required')),"
        "CHECK(state IN ('accepted','rejected','conflict','queued','dispatching','executed_unverified','succeeded','failed_before_dispatch','failed_verified','outcome_unknown','cancelled'))"
        ");"
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_mutation_operations_idempotency_scope "
        "ON mutation_operations(actor_id,backend_id,resource_type,resource_id,action_family,idempotency_key);"
        "CREATE INDEX IF NOT EXISTS idx_mutation_operations_backend_state "
        "ON mutation_operations(backend_id,state,updated_at);");
}

MutationOperationRepositoryResult MutationOperationRepository::reserve(
    const MutationOperation& operation)
{
    if (!mutationOperationValidForCreate(operation))
        return statusResult(MutationOperationRepositoryStatus::invalid);

    MutationOperation durable = operation;
    durable.operationRevision = "1";

    auto lease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
        return statusResult(MutationOperationRepositoryStatus::storageError);
    auto rollback = [this]() { database_.execute("ROLLBACK;"); };

    MutationOperation existing;
    bool found = false;
    if (!selectById(database_, operation.operationId, existing, found))
    {
        rollback();
        return statusResult(MutationOperationRepositoryStatus::storageError);
    }
    if (found)
    {
        rollback();
        return operationResult(
            sameLogicalOperation(existing, durable)
                ? MutationOperationRepositoryStatus::idempotentReplay
                : MutationOperationRepositoryStatus::operationConflict,
            existing);
    }

    if (!selectByScope(
            database_, operation.actorId, operation.backendId,
            operation.resourceType, operation.resourceId,
            operation.actionFamily, operation.idempotencyKey,
            existing, found))
    {
        rollback();
        return statusResult(MutationOperationRepositoryStatus::storageError);
    }
    if (found)
    {
        rollback();
        return operationResult(
            MutationOperationRepositoryStatus::idempotencyConflict, existing);
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO mutation_operations("
        "operation_id,operation_revision,idempotency_key,actor_id,backend_id,backend_generation,"
        "resource_type,resource_id,expected_revision,action_family,request_fingerprint,requested_at,"
        "deadline,verification_policy,state,result_reference,updated_at)"
        " VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
    const bool prepared =
        sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) == SQLITE_OK;
    const bool bound = prepared &&
        bindText(statement, 1, durable.operationId) &&
        bindInt64(statement, 2, 1) &&
        bindText(statement, 3, durable.idempotencyKey) &&
        bindText(statement, 4, durable.actorId) &&
        bindText(statement, 5, durable.backendId) &&
        bindInt64(statement, 6, static_cast<std::int64_t>(durable.backendGeneration)) &&
        bindText(statement, 7, durable.resourceType) &&
        bindText(statement, 8, durable.resourceId) &&
        bindText(statement, 9, durable.expectedRevision) &&
        bindText(statement, 10, durable.actionFamily) &&
        bindText(statement, 11, durable.requestFingerprint) &&
        bindInt64(statement, 12, durable.requestedAt) &&
        bindInt64(statement, 13, durable.deadline) &&
        bindText(statement, 14, mutationOperationVerificationPolicyName(durable.verificationPolicy)) &&
        bindText(statement, 15, mutationOperationStateName(durable.state)) &&
        bindText(statement, 16, durable.resultReference) &&
        bindInt64(statement, 17, durable.updatedAt);
    const int step = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    if (statement != nullptr) sqlite3_finalize(statement);
    if (step != SQLITE_DONE || !database_.execute("COMMIT;"))
    {
        rollback();
        return statusResult(MutationOperationRepositoryStatus::storageError);
    }
    return operationResult(MutationOperationRepositoryStatus::ok, durable);
}

MutationOperationRepositoryResult MutationOperationRepository::findById(
    const std::string& operationId)
{
    if (!safeIdentity(operationId))
        return statusResult(MutationOperationRepositoryStatus::invalid);
    MutationOperation operation;
    bool found = false;
    if (!selectById(database_, operationId, operation, found))
        return statusResult(MutationOperationRepositoryStatus::storageError);
    return found
        ? operationResult(MutationOperationRepositoryStatus::ok, operation)
        : statusResult(MutationOperationRepositoryStatus::notFound);
}

MutationOperationRepositoryResult MutationOperationRepository::findByIdempotencyScope(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& resourceType,
    const std::string& resourceId,
    const std::string& actionFamily,
    const std::string& idempotencyKey)
{
    if (!safeIdentity(actorId) || !safeIdentity(backendId) ||
        !safeIdentity(resourceType) || !safeIdentity(resourceId) ||
        !safeIdentity(actionFamily) || !safeIdentity(idempotencyKey))
        return statusResult(MutationOperationRepositoryStatus::invalid);
    MutationOperation operation;
    bool found = false;
    if (!selectByScope(
            database_, actorId, backendId, resourceType, resourceId,
            actionFamily, idempotencyKey, operation, found))
        return statusResult(MutationOperationRepositoryStatus::storageError);
    return found
        ? operationResult(MutationOperationRepositoryStatus::ok, operation)
        : statusResult(MutationOperationRepositoryStatus::notFound);
}

MutationOperationRepositoryResult MutationOperationRepository::transition(
    const std::string& operationId,
    const std::string& expectedOperationRevision,
    MutationOperationState expectedState,
    MutationOperationState nextState,
    const std::string& resultReference,
    std::int64_t updatedAt)
{
    std::int64_t expectedRevision = 0;
    if (!safeIdentity(operationId) ||
        !parseRevision(expectedOperationRevision, expectedRevision) ||
        resultReference.size() > kMaxResultReferenceLength || updatedAt <= 0)
        return statusResult(MutationOperationRepositoryStatus::invalid);

    auto lease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
        return statusResult(MutationOperationRepositoryStatus::storageError);
    auto rollback = [this]() { database_.execute("ROLLBACK;"); };

    MutationOperation current;
    bool found = false;
    if (!selectById(database_, operationId, current, found))
    {
        rollback();
        return statusResult(MutationOperationRepositoryStatus::storageError);
    }
    if (!found)
    {
        rollback();
        return statusResult(MutationOperationRepositoryStatus::notFound);
    }

    if (current.state == nextState && current.resultReference == resultReference)
    {
        rollback();
        return operationResult(
            MutationOperationRepositoryStatus::idempotentReplay, current);
    }
    if (current.operationRevision != expectedOperationRevision)
    {
        rollback();
        return operationResult(
            MutationOperationRepositoryStatus::revisionConflict, current);
    }
    if (current.state != expectedState ||
        !mutationOperationTransitionAllowed(current.state, nextState))
    {
        rollback();
        return operationResult(
            MutationOperationRepositoryStatus::stateConflict, current);
    }
    if (updatedAt < current.updatedAt ||
        expectedRevision == std::numeric_limits<std::int64_t>::max())
    {
        rollback();
        return statusResult(MutationOperationRepositoryStatus::invalid);
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE mutation_operations SET operation_revision=?,state=?,result_reference=?,updated_at=? "
        "WHERE operation_id=? AND operation_revision=?;";
    const std::int64_t nextRevision = expectedRevision + 1;
    const bool prepared =
        sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) == SQLITE_OK;
    const bool bound = prepared &&
        bindInt64(statement, 1, nextRevision) &&
        bindText(statement, 2, mutationOperationStateName(nextState)) &&
        bindText(statement, 3, resultReference) &&
        bindInt64(statement, 4, updatedAt) &&
        bindText(statement, 5, operationId) &&
        bindInt64(statement, 6, expectedRevision);
    const int step = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    const int changed = step == SQLITE_DONE ? sqlite3_changes(database_.handle()) : 0;
    if (statement != nullptr) sqlite3_finalize(statement);
    if (step != SQLITE_DONE || changed != 1)
    {
        rollback();
        return operationResult(
            MutationOperationRepositoryStatus::revisionConflict, current);
    }
    if (!database_.execute("COMMIT;"))
    {
        rollback();
        return statusResult(MutationOperationRepositoryStatus::storageError);
    }

    current.operationRevision = std::to_string(nextRevision);
    current.state = nextState;
    current.resultReference = resultReference;
    current.updatedAt = updatedAt;
    return operationResult(MutationOperationRepositoryStatus::ok, current);
}

}

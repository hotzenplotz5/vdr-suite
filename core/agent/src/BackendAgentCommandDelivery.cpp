#include "BackendAgentCommandDelivery.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <system_error>

#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace
{
constexpr std::size_t MaxIdentifierBytes = 128;
constexpr std::size_t MaxPayloadBytes = 16384;
constexpr std::size_t MaxDiagnosticsBytes = 4096;

bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > MaxIdentifierBytes) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return (character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9') ||
            character == '-' || character == '_' || character == '.' ||
            character == ':';
    });
}

bool fitsDatabaseInteger(std::uint64_t value)
{
    return value <= static_cast<std::uint64_t>(
        std::numeric_limits<std::int64_t>::max());
}

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(statement, index, value.c_str(), -1,
        SQLITE_TRANSIENT) == SQLITE_OK;
}

bool bindInt64(sqlite3_stmt* statement, int index, std::int64_t value)
{
    return sqlite3_bind_int64(statement, index, value) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const auto* value = sqlite3_column_text(statement, column);
    return value == nullptr ? std::string() :
        std::string(reinterpret_cast<const char*>(value));
}

bool executeStatement(sqlite3_stmt* statement)
{
    const int result = sqlite3_step(statement);
    sqlite3_finalize(statement);
    return result == SQLITE_DONE;
}

class Transaction
{
public:
    explicit Transaction(Database& database)
        : database_(database),
          lease_(database.acquireTransactionLease()),
          active_(database.execute("BEGIN IMMEDIATE;"))
    {
    }

    ~Transaction()
    {
        if (active_) database_.execute("ROLLBACK;");
    }

    bool active() const { return active_; }
    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;")) return false;
        active_ = false;
        return true;
    }

private:
    Database& database_;
    Database::TransactionLease lease_;
    bool active_;
};

std::string hexEncode(const std::string& value)
{
    static constexpr char Hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(value.size() * 2);
    for (unsigned char character : value)
    {
        result.push_back(Hex[(character >> 4) & 0x0f]);
        result.push_back(Hex[character & 0x0f]);
    }
    return result;
}

int hexNibble(char value)
{
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

bool hexDecode(const std::string& value, std::string& result)
{
    if ((value.size() % 2) != 0) return false;
    result.clear();
    result.reserve(value.size() / 2);
    for (std::size_t index = 0; index < value.size(); index += 2)
    {
        const int high = hexNibble(value[index]);
        const int low = hexNibble(value[index + 1]);
        if (high < 0 || low < 0) return false;
        result.push_back(static_cast<char>((high << 4) | low));
    }
    return true;
}

std::string dispatchName(BackendAgentCommandDispatchState state)
{
    return backendAgentCommandDispatchStateName(state);
}

bool parseDispatch(const std::string& value,
    BackendAgentCommandDispatchState& state)
{
    if (value == "not_started") state = BackendAgentCommandDispatchState::NotStarted;
    else if (value == "starting") state = BackendAgentCommandDispatchState::Starting;
    else if (value == "accepted_by_executor") state = BackendAgentCommandDispatchState::AcceptedByExecutor;
    else if (value == "effect_reported") state = BackendAgentCommandDispatchState::EffectReported;
    else return false;
    return true;
}

void append(std::ostringstream& output, const std::string& key,
    const std::string& value)
{
    output << key << '=' << hexEncode(value) << '\n';
}

void appendNumber(std::ostringstream& output, const std::string& key,
    std::uint64_t value)
{
    output << key << '=' << value << '\n';
}

void appendSigned(std::ostringstream& output, const std::string& key,
    std::int64_t value)
{
    output << key << '=' << value << '\n';
}

bool parseUnsigned(const std::string& value, std::uint64_t& result)
{
    try
    {
        std::size_t consumed = 0;
        const auto parsed = std::stoull(value, &consumed, 10);
        if (consumed != value.size()) return false;
        result = parsed;
        return true;
    }
    catch (...) { return false; }
}

bool parseSigned(const std::string& value, std::int64_t& result)
{
    try
    {
        std::size_t consumed = 0;
        const auto parsed = std::stoll(value, &consumed, 10);
        if (consumed != value.size()) return false;
        result = parsed;
        return true;
    }
    catch (...) { return false; }
}

std::optional<std::map<std::string, std::string>> readFields(
    const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) return std::nullopt;
    std::map<std::string, std::string> fields;
    std::string line;
    std::size_t total = 0;
    while (std::getline(input, line))
    {
        total += line.size();
        if (total > 131072) return std::nullopt;
        const auto separator = line.find('=');
        if (separator == std::string::npos || separator == 0) return std::nullopt;
        if (!fields.emplace(line.substr(0, separator),
            line.substr(separator + 1)).second) return std::nullopt;
    }
    return fields;
}

bool decoded(const std::map<std::string, std::string>& fields,
    const std::string& key, std::string& value)
{
    const auto found = fields.find(key);
    return found != fields.end() && hexDecode(found->second, value);
}

bool unsignedField(const std::map<std::string, std::string>& fields,
    const std::string& key, std::uint64_t& value)
{
    const auto found = fields.find(key);
    return found != fields.end() && parseUnsigned(found->second, value);
}

bool signedField(const std::map<std::string, std::string>& fields,
    const std::string& key, std::int64_t& value)
{
    const auto found = fields.find(key);
    return found != fields.end() && parseSigned(found->second, value);
}
}

std::string backendAgentCommandDispatchStateName(
    BackendAgentCommandDispatchState state)
{
    switch (state)
    {
        case BackendAgentCommandDispatchState::Starting: return "starting";
        case BackendAgentCommandDispatchState::AcceptedByExecutor:
            return "accepted_by_executor";
        case BackendAgentCommandDispatchState::EffectReported:
            return "effect_reported";
        case BackendAgentCommandDispatchState::NotStarted:
        default: return "not_started";
    }
}

bool backendAgentValidCommandEnvelope(
    const BackendAgentCommandEnvelope& envelope,
    std::string& reasonCode)
{
    if (envelope.protocolVersion != "vdr-suite-agent/1")
    {
        reasonCode = "unsupported_protocol";
        return false;
    }
    for (const auto* value : {&envelope.requestId, &envelope.correlationId,
         &envelope.jobId, &envelope.attemptId, &envelope.commandId,
         &envelope.backendId, &envelope.agentId, &envelope.agentInstanceId,
         &envelope.commandType, &envelope.requestFingerprint})
    {
        if (!safeIdentifier(*value))
        {
            reasonCode = "invalid_identifier";
            return false;
        }
    }
    if (!envelope.operationId.empty() && !safeIdentifier(envelope.operationId))
    {
        reasonCode = "invalid_operation_id";
        return false;
    }
    if (envelope.claimEpoch == 0 || envelope.backendGeneration == 0 ||
        envelope.payloadVersion == 0 ||
        !fitsDatabaseInteger(envelope.claimEpoch) ||
        !fitsDatabaseInteger(envelope.backendGeneration) ||
        !fitsDatabaseInteger(envelope.payloadVersion))
    {
        reasonCode = "invalid_generation_or_version";
        return false;
    }
    if (envelope.payload.size() > MaxPayloadBytes || envelope.deadline <= 0 ||
        envelope.assignedAt <= 0 || envelope.deadline < envelope.assignedAt)
    {
        reasonCode = "invalid_bounds_or_deadline";
        return false;
    }
    reasonCode.clear();
    return true;
}

BackendAgentCommandRepository::BackendAgentCommandRepository(Database& database)
    : database_(database)
{
}

bool BackendAgentCommandRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_agent_commands ("
        "command_id TEXT PRIMARY KEY, request_fingerprint TEXT NOT NULL,"
        "protocol_version TEXT NOT NULL, request_id TEXT NOT NULL,"
        "correlation_id TEXT NOT NULL, operation_id TEXT NOT NULL,"
        "job_id TEXT NOT NULL, attempt_id TEXT NOT NULL, claim_epoch INTEGER NOT NULL,"
        "backend_id TEXT NOT NULL, agent_id TEXT NOT NULL, agent_instance_id TEXT NOT NULL,"
        "backend_generation INTEGER NOT NULL, command_type TEXT NOT NULL,"
        "payload_version INTEGER NOT NULL, payload TEXT NOT NULL,"
        "assigned_at INTEGER NOT NULL, deadline INTEGER NOT NULL,"
        "assignment_state TEXT NOT NULL DEFAULT 'pending',"
        "receipt_category TEXT NOT NULL DEFAULT '', receipt_reason TEXT NOT NULL DEFAULT '',"
        "received_at INTEGER NOT NULL DEFAULT 0, dispatch_state TEXT NOT NULL DEFAULT 'not_started',"
        "verification_state TEXT NOT NULL DEFAULT '', result_category TEXT NOT NULL DEFAULT '',"
        "error_category TEXT NOT NULL DEFAULT '', retry_classification TEXT NOT NULL DEFAULT '',"
        "bounded_diagnostics TEXT NOT NULL DEFAULT '', completed_at INTEGER NOT NULL DEFAULT 0,"
        "result_acknowledged_at INTEGER NOT NULL DEFAULT 0,"
        "CHECK(assignment_state IN ('pending','receipted','resulted','reconciliation')),"
        "CHECK(dispatch_state IN ('not_started','starting','accepted_by_executor','effect_reported'))"
        ");") &&
        database_.execute(
        "CREATE INDEX IF NOT EXISTS idx_backend_agent_commands_poll "
        "ON backend_agent_commands(backend_id, agent_id, agent_instance_id, "
        "backend_generation, assignment_state, assigned_at);");
}

BackendAgentCommandDecision BackendAgentCommandRepository::createAssignment(
    const BackendAgentCommandEnvelope& envelope)
{
    BackendAgentCommandDecision decision;
    if (!backendAgentValidCommandEnvelope(envelope, decision.reasonCode)) return decision;
    if (const auto existing = find(envelope.commandId))
    {
        decision.state = *existing;
        decision.duplicate = existing->envelope.requestFingerprint ==
            envelope.requestFingerprint;
        decision.conflict = !decision.duplicate;
        decision.accepted = decision.duplicate;
        decision.reasonCode = decision.duplicate ? "duplicate" :
            "conflicting_duplicate";
        return decision;
    }
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO backend_agent_commands (command_id, request_fingerprint, "
        "protocol_version, request_id, correlation_id, operation_id, job_id, attempt_id, "
        "claim_epoch, backend_id, agent_id, agent_instance_id, backend_generation, "
        "command_type, payload_version, payload, assigned_at, deadline) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        decision.reasonCode = "database_prepare_failed";
        return decision;
    }
    const bool bound =
        bindText(statement,1,envelope.commandId) && bindText(statement,2,envelope.requestFingerprint) &&
        bindText(statement,3,envelope.protocolVersion) && bindText(statement,4,envelope.requestId) &&
        bindText(statement,5,envelope.correlationId) && bindText(statement,6,envelope.operationId) &&
        bindText(statement,7,envelope.jobId) && bindText(statement,8,envelope.attemptId) &&
        bindInt64(statement,9,static_cast<std::int64_t>(envelope.claimEpoch)) &&
        bindText(statement,10,envelope.backendId) && bindText(statement,11,envelope.agentId) &&
        bindText(statement,12,envelope.agentInstanceId) &&
        bindInt64(statement,13,static_cast<std::int64_t>(envelope.backendGeneration)) &&
        bindText(statement,14,envelope.commandType) &&
        bindInt64(statement,15,static_cast<std::int64_t>(envelope.payloadVersion)) &&
        bindText(statement,16,envelope.payload) && bindInt64(statement,17,envelope.assignedAt) &&
        bindInt64(statement,18,envelope.deadline);
    if (!bound || !executeStatement(statement))
    {
        if (!bound) sqlite3_finalize(statement);
        decision.reasonCode = "database_write_failed";
        return decision;
    }
    decision.accepted = true;
    decision.reasonCode = "accepted";
    decision.state.envelope = envelope;
    return decision;
}

std::optional<BackendAgentCommandEnvelope>
BackendAgentCommandRepository::nextAssignment(
    const std::string& backendId, const std::string& agentId,
    const std::string& agentInstanceId, std::uint64_t backendGeneration,
    std::int64_t now) const
{
    if (!fitsDatabaseInteger(backendGeneration)) return std::nullopt;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT command_id FROM backend_agent_commands WHERE backend_id=? AND agent_id=? "
        "AND agent_instance_id=? AND backend_generation=? AND assignment_state='pending' "
        "AND deadline>=? ORDER BY assigned_at, command_id LIMIT 1;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement,1,backendId) || !bindText(statement,2,agentId) ||
        !bindText(statement,3,agentInstanceId) ||
        !bindInt64(statement,4,static_cast<std::int64_t>(backendGeneration)) ||
        !bindInt64(statement,5,now))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return std::nullopt;
    }
    std::string commandId;
    if (sqlite3_step(statement) == SQLITE_ROW) commandId = columnText(statement,0);
    sqlite3_finalize(statement);
    if (commandId.empty()) return std::nullopt;
    const auto state = find(commandId);
    return state ? std::optional<BackendAgentCommandEnvelope>(state->envelope) : std::nullopt;
}

std::optional<BackendAgentCommandState> BackendAgentCommandRepository::find(
    const std::string& commandId) const
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT protocol_version,request_id,correlation_id,operation_id,job_id,attempt_id,"
        "claim_epoch,command_id,backend_id,agent_id,agent_instance_id,backend_generation,"
        "command_type,payload_version,payload,request_fingerprint,assigned_at,deadline,"
        "receipt_category,receipt_reason,received_at,dispatch_state,verification_state,"
        "result_category,error_category,retry_classification,bounded_diagnostics,completed_at,"
        "result_acknowledged_at FROM backend_agent_commands WHERE command_id=?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement,1,commandId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return std::nullopt;
    }
    if (sqlite3_step(statement) != SQLITE_ROW)
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }
    BackendAgentCommandState state;
    auto& e=state.envelope;
    e.protocolVersion=columnText(statement,0); e.requestId=columnText(statement,1);
    e.correlationId=columnText(statement,2); e.operationId=columnText(statement,3);
    e.jobId=columnText(statement,4); e.attemptId=columnText(statement,5);
    e.claimEpoch=static_cast<std::uint64_t>(sqlite3_column_int64(statement,6));
    e.commandId=columnText(statement,7); e.backendId=columnText(statement,8);
    e.agentId=columnText(statement,9); e.agentInstanceId=columnText(statement,10);
    e.backendGeneration=static_cast<std::uint64_t>(sqlite3_column_int64(statement,11));
    e.commandType=columnText(statement,12);
    e.payloadVersion=static_cast<std::uint64_t>(sqlite3_column_int64(statement,13));
    e.payload=columnText(statement,14); e.requestFingerprint=columnText(statement,15);
    e.assignedAt=sqlite3_column_int64(statement,16); e.deadline=sqlite3_column_int64(statement,17);
    const std::string receiptCategory=columnText(statement,18);
    state.receiptPresent=!receiptCategory.empty();
    if (state.receiptPresent)
    {
        auto& r=state.receipt; r.commandId=e.commandId; r.requestFingerprint=e.requestFingerprint;
        r.jobId=e.jobId; r.attemptId=e.attemptId; r.claimEpoch=e.claimEpoch;
        r.backendId=e.backendId; r.agentId=e.agentId; r.agentInstanceId=e.agentInstanceId;
        r.backendGeneration=e.backendGeneration; r.receiptCategory=receiptCategory;
        r.reasonCode=columnText(statement,19); r.receivedAt=sqlite3_column_int64(statement,20);
    }
    parseDispatch(columnText(statement,21),state.dispatchState);
    const std::string resultCategory=columnText(statement,23);
    state.resultPresent=!resultCategory.empty();
    if (state.resultPresent)
    {
        auto& r=state.result; r.commandId=e.commandId; r.requestFingerprint=e.requestFingerprint;
        r.jobId=e.jobId; r.attemptId=e.attemptId; r.claimEpoch=e.claimEpoch;
        r.backendId=e.backendId; r.agentId=e.agentId; r.agentInstanceId=e.agentInstanceId;
        r.backendGeneration=e.backendGeneration; r.dispatchState=state.dispatchState;
        r.verificationState=columnText(statement,22); r.resultCategory=resultCategory;
        r.errorCategory=columnText(statement,24); r.retryClassification=columnText(statement,25);
        r.boundedDiagnostics=columnText(statement,26); r.completedAt=sqlite3_column_int64(statement,27);
    }
    state.resultAcknowledged=sqlite3_column_int64(statement,28)>0;
    sqlite3_finalize(statement);
    return state;
}

BackendAgentCommandDecision BackendAgentCommandRepository::recordReceipt(
    const BackendAgentCommandReceipt& receipt)
{
    BackendAgentCommandDecision decision;
    const auto existing=find(receipt.commandId);
    if (!existing) { decision.reasonCode="unknown_command"; return decision; }
    decision.state=*existing;
    const auto& e=existing->envelope;
    const bool identity=e.requestFingerprint==receipt.requestFingerprint && e.jobId==receipt.jobId &&
        e.attemptId==receipt.attemptId && e.claimEpoch==receipt.claimEpoch &&
        e.backendId==receipt.backendId && e.agentId==receipt.agentId &&
        e.agentInstanceId==receipt.agentInstanceId && e.backendGeneration==receipt.backendGeneration;
    if (!identity) { decision.conflict=true; decision.reasonCode="fence_mismatch"; return decision; }
    if (existing->receiptPresent)
    {
        decision.duplicate=existing->receipt.receiptCategory==receipt.receiptCategory &&
            existing->receipt.reasonCode==receipt.reasonCode;
        decision.conflict=!decision.duplicate; decision.accepted=decision.duplicate;
        decision.reasonCode=decision.duplicate?"duplicate":"conflicting_receipt";
        return decision;
    }
    sqlite3_stmt* statement=nullptr;
    const char* sql="UPDATE backend_agent_commands SET receipt_category=?,receipt_reason=?,"
        "received_at=?,assignment_state='receipted' WHERE command_id=? AND request_fingerprint=?;";
    if (sqlite3_prepare_v2(database_.handle(),sql,-1,&statement,nullptr)!=SQLITE_OK ||
        !bindText(statement,1,receipt.receiptCategory) || !bindText(statement,2,receipt.reasonCode) ||
        !bindInt64(statement,3,receipt.receivedAt) || !bindText(statement,4,receipt.commandId) ||
        !bindText(statement,5,receipt.requestFingerprint) || !executeStatement(statement))
    {
        if (statement!=nullptr) sqlite3_finalize(statement);
        decision.reasonCode="database_write_failed"; return decision;
    }
    decision.accepted=true; decision.reasonCode="accepted";
    decision.state=*find(receipt.commandId); return decision;
}

BackendAgentCommandDecision BackendAgentCommandRepository::recordResult(
    const BackendAgentCommandResult& result)
{
    BackendAgentCommandDecision decision;
    const auto existing=find(result.commandId);
    if (!existing) { decision.reasonCode="unknown_command"; return decision; }
    decision.state=*existing;
    const auto& e=existing->envelope;
    const bool identity=e.requestFingerprint==result.requestFingerprint && e.jobId==result.jobId &&
        e.attemptId==result.attemptId && e.claimEpoch==result.claimEpoch &&
        e.backendId==result.backendId && e.agentId==result.agentId &&
        e.agentInstanceId==result.agentInstanceId && e.backendGeneration==result.backendGeneration;
    if (!identity) { decision.conflict=true; decision.reasonCode="fence_mismatch"; return decision; }
    if (result.boundedDiagnostics.size()>MaxDiagnosticsBytes)
    { decision.reasonCode="diagnostics_too_large"; return decision; }
    if (existing->resultPresent)
    {
        decision.duplicate=existing->result.resultCategory==result.resultCategory &&
            existing->result.dispatchState==result.dispatchState &&
            existing->result.verificationState==result.verificationState &&
            existing->result.errorCategory==result.errorCategory &&
            existing->result.retryClassification==result.retryClassification &&
            existing->result.boundedDiagnostics==result.boundedDiagnostics;
        decision.conflict=!decision.duplicate; decision.accepted=decision.duplicate;
        decision.reasonCode=decision.duplicate?"duplicate":"conflicting_result";
        return decision;
    }
    sqlite3_stmt* statement=nullptr;
    const char* sql="UPDATE backend_agent_commands SET dispatch_state=?,verification_state=?,"
        "result_category=?,error_category=?,retry_classification=?,bounded_diagnostics=?,"
        "completed_at=?,assignment_state='resulted' WHERE command_id=? AND request_fingerprint=?;";
    if (sqlite3_prepare_v2(database_.handle(),sql,-1,&statement,nullptr)!=SQLITE_OK ||
        !bindText(statement,1,dispatchName(result.dispatchState)) ||
        !bindText(statement,2,result.verificationState) || !bindText(statement,3,result.resultCategory) ||
        !bindText(statement,4,result.errorCategory) || !bindText(statement,5,result.retryClassification) ||
        !bindText(statement,6,result.boundedDiagnostics) || !bindInt64(statement,7,result.completedAt) ||
        !bindText(statement,8,result.commandId) || !bindText(statement,9,result.requestFingerprint) ||
        !executeStatement(statement))
    {
        if (statement!=nullptr) sqlite3_finalize(statement);
        decision.reasonCode="database_write_failed"; return decision;
    }
    decision.accepted=true; decision.reasonCode="accepted";
    decision.state=*find(result.commandId); return decision;
}

bool BackendAgentCommandRepository::acknowledgeResult(
    const std::string& commandId, const std::string& requestFingerprint,
    std::int64_t acknowledgedAt)
{
    sqlite3_stmt* statement=nullptr;
    const char* sql="UPDATE backend_agent_commands SET result_acknowledged_at=? "
        "WHERE command_id=? AND request_fingerprint=? AND result_category<>'';";
    if (sqlite3_prepare_v2(database_.handle(),sql,-1,&statement,nullptr)!=SQLITE_OK ||
        !bindInt64(statement,1,acknowledgedAt) || !bindText(statement,2,commandId) ||
        !bindText(statement,3,requestFingerprint))
    { if(statement!=nullptr) sqlite3_finalize(statement); return false; }
    return executeStatement(statement) && sqlite3_changes(database_.handle())==1;
}

BackendAgentLocalCommandStore::BackendAgentLocalCommandStore(
    std::string stateDirectory) : stateDirectory_(std::move(stateDirectory)) {}

bool BackendAgentLocalCommandStore::ensurePrivateStateDirectory() const
{
    std::error_code error;
    std::filesystem::create_directories(stateDirectory_,error);
    if (error || !std::filesystem::is_directory(stateDirectory_)) return false;
#ifndef _WIN32
    return ::chmod(stateDirectory_.c_str(),0700)==0;
#else
    return true;
#endif
}

std::string BackendAgentLocalCommandStore::commandPath(
    const std::string& commandId) const
{
    if (!safeIdentifier(commandId)) return {};
    return (std::filesystem::path(stateDirectory_) / (commandId+".command")).string();
}

bool BackendAgentLocalCommandStore::writeState(
    const BackendAgentCommandState& state) const
{
    if (!ensurePrivateStateDirectory()) return false;
    const std::string path=commandPath(state.envelope.commandId);
    if (path.empty()) return false;
    std::ostringstream output;
    output<<"format=1\n";
    const auto& e=state.envelope;
    append(output,"protocolVersion",e.protocolVersion); append(output,"requestId",e.requestId);
    append(output,"correlationId",e.correlationId); append(output,"operationId",e.operationId);
    append(output,"jobId",e.jobId); append(output,"attemptId",e.attemptId);
    appendNumber(output,"claimEpoch",e.claimEpoch); append(output,"commandId",e.commandId);
    append(output,"backendId",e.backendId); append(output,"agentId",e.agentId);
    append(output,"agentInstanceId",e.agentInstanceId); appendNumber(output,"backendGeneration",e.backendGeneration);
    append(output,"commandType",e.commandType); appendNumber(output,"payloadVersion",e.payloadVersion);
    append(output,"payload",e.payload); append(output,"requestFingerprint",e.requestFingerprint);
    appendSigned(output,"assignedAt",e.assignedAt); appendSigned(output,"deadline",e.deadline);
    append(output,"dispatchState",dispatchName(state.dispatchState));
    appendNumber(output,"receiptPresent",state.receiptPresent?1:0);
    append(output,"receiptCategory",state.receipt.receiptCategory);
    append(output,"receiptReason",state.receipt.reasonCode);
    appendSigned(output,"receivedAt",state.receipt.receivedAt);
    appendNumber(output,"resultPresent",state.resultPresent?1:0);
    append(output,"verificationState",state.result.verificationState);
    append(output,"resultCategory",state.result.resultCategory);
    append(output,"errorCategory",state.result.errorCategory);
    append(output,"retryClassification",state.result.retryClassification);
    append(output,"boundedDiagnostics",state.result.boundedDiagnostics);
    appendSigned(output,"completedAt",state.result.completedAt);
    appendNumber(output,"resultAcknowledged",state.resultAcknowledged?1:0);
    const std::string temporary=path+".tmp";
    {
        std::ofstream file(temporary,std::ios::binary|std::ios::trunc);
        if (!file || !(file<<output.str()) || !file.flush()) return false;
    }
#ifndef _WIN32
    if (::chmod(temporary.c_str(),0600)!=0) { std::filesystem::remove(temporary); return false; }
#endif
    std::error_code error;
    std::filesystem::rename(temporary,path,error);
    if (error) { std::filesystem::remove(temporary); return false; }
    return true;
}

std::optional<BackendAgentCommandState> BackendAgentLocalCommandStore::readState(
    const std::string& commandId) const
{
    const std::string path=commandPath(commandId);
    if (path.empty()) return std::nullopt;
    const auto fields=readFields(path);
    if (!fields || fields->find("format")==fields->end() || fields->at("format")!="1") return std::nullopt;
    BackendAgentCommandState state; auto& e=state.envelope;
    std::uint64_t flag=0; std::string dispatch;
    if (!decoded(*fields,"protocolVersion",e.protocolVersion) || !decoded(*fields,"requestId",e.requestId) ||
        !decoded(*fields,"correlationId",e.correlationId) || !decoded(*fields,"operationId",e.operationId) ||
        !decoded(*fields,"jobId",e.jobId) || !decoded(*fields,"attemptId",e.attemptId) ||
        !unsignedField(*fields,"claimEpoch",e.claimEpoch) || !decoded(*fields,"commandId",e.commandId) ||
        e.commandId!=commandId || !decoded(*fields,"backendId",e.backendId) ||
        !decoded(*fields,"agentId",e.agentId) || !decoded(*fields,"agentInstanceId",e.agentInstanceId) ||
        !unsignedField(*fields,"backendGeneration",e.backendGeneration) ||
        !decoded(*fields,"commandType",e.commandType) || !unsignedField(*fields,"payloadVersion",e.payloadVersion) ||
        !decoded(*fields,"payload",e.payload) || !decoded(*fields,"requestFingerprint",e.requestFingerprint) ||
        !signedField(*fields,"assignedAt",e.assignedAt) || !signedField(*fields,"deadline",e.deadline) ||
        !decoded(*fields,"dispatchState",dispatch) || !parseDispatch(dispatch,state.dispatchState) ||
        !unsignedField(*fields,"receiptPresent",flag)) return std::nullopt;
    state.receiptPresent=flag==1;
    auto& receipt=state.receipt; receipt.commandId=e.commandId; receipt.requestFingerprint=e.requestFingerprint;
    receipt.jobId=e.jobId; receipt.attemptId=e.attemptId; receipt.claimEpoch=e.claimEpoch;
    receipt.backendId=e.backendId; receipt.agentId=e.agentId; receipt.agentInstanceId=e.agentInstanceId;
    receipt.backendGeneration=e.backendGeneration;
    if (!decoded(*fields,"receiptCategory",receipt.receiptCategory) ||
        !decoded(*fields,"receiptReason",receipt.reasonCode) ||
        !signedField(*fields,"receivedAt",receipt.receivedAt) ||
        !unsignedField(*fields,"resultPresent",flag)) return std::nullopt;
    state.resultPresent=flag==1;
    auto& result=state.result; result.commandId=e.commandId; result.requestFingerprint=e.requestFingerprint;
    result.jobId=e.jobId; result.attemptId=e.attemptId; result.claimEpoch=e.claimEpoch;
    result.backendId=e.backendId; result.agentId=e.agentId; result.agentInstanceId=e.agentInstanceId;
    result.backendGeneration=e.backendGeneration; result.dispatchState=state.dispatchState;
    if (!decoded(*fields,"verificationState",result.verificationState) ||
        !decoded(*fields,"resultCategory",result.resultCategory) ||
        !decoded(*fields,"errorCategory",result.errorCategory) ||
        !decoded(*fields,"retryClassification",result.retryClassification) ||
        !decoded(*fields,"boundedDiagnostics",result.boundedDiagnostics) ||
        !signedField(*fields,"completedAt",result.completedAt) ||
        !unsignedField(*fields,"resultAcknowledged",flag)) return std::nullopt;
    state.resultAcknowledged=flag==1;
    std::string reason; if (!backendAgentValidCommandEnvelope(e,reason)) return std::nullopt;
    return state;
}

std::optional<BackendAgentCommandState> BackendAgentLocalCommandStore::find(
    const std::string& commandId) const { return readState(commandId); }

BackendAgentCommandDecision BackendAgentLocalCommandStore::accept(
    const BackendAgentCommandEnvelope& envelope, const std::string& currentAgentId,
    const std::string& currentAgentInstanceId, std::uint64_t currentBackendGeneration,
    std::int64_t now)
{
    BackendAgentCommandDecision decision;
    if (!backendAgentValidCommandEnvelope(envelope,decision.reasonCode)) return decision;
    if (envelope.agentId!=currentAgentId || envelope.agentInstanceId!=currentAgentInstanceId ||
        envelope.backendGeneration!=currentBackendGeneration)
    { decision.stale=true; decision.reasonCode="stale_agent_fence"; return decision; }
    if (now>envelope.deadline) { decision.expired=true; decision.reasonCode="expired"; return decision; }
    if (const auto existing=readState(envelope.commandId))
    {
        decision.state=*existing;
        decision.duplicate=existing->envelope.requestFingerprint==envelope.requestFingerprint;
        decision.conflict=!decision.duplicate; decision.accepted=decision.duplicate;
        decision.reasonCode=decision.duplicate?"duplicate":"conflicting_duplicate";
        return decision;
    }
    BackendAgentCommandState state; state.envelope=envelope; state.receiptPresent=true;
    state.receipt.commandId=envelope.commandId; state.receipt.requestFingerprint=envelope.requestFingerprint;
    state.receipt.jobId=envelope.jobId; state.receipt.attemptId=envelope.attemptId;
    state.receipt.claimEpoch=envelope.claimEpoch; state.receipt.backendId=envelope.backendId;
    state.receipt.agentId=envelope.agentId; state.receipt.agentInstanceId=envelope.agentInstanceId;
    state.receipt.backendGeneration=envelope.backendGeneration;
    state.receipt.receiptCategory="accepted"; state.receipt.receivedAt=now;
    state.receipt.reasonCode="durably_recorded";
    if (!writeState(state)) { decision.reasonCode="inbox_persistence_failed"; return decision; }
    decision.accepted=true; decision.reasonCode="accepted"; decision.state=state; return decision;
}

bool BackendAgentLocalCommandStore::markStarting(const std::string& commandId)
{
    auto state=readState(commandId); if(!state || state->dispatchState!=BackendAgentCommandDispatchState::NotStarted) return false;
    state->dispatchState=BackendAgentCommandDispatchState::Starting; return writeState(*state);
}

bool BackendAgentLocalCommandStore::markAcceptedByExecutor(const std::string& commandId)
{
    auto state=readState(commandId); if(!state || state->dispatchState!=BackendAgentCommandDispatchState::Starting) return false;
    state->dispatchState=BackendAgentCommandDispatchState::AcceptedByExecutor; return writeState(*state);
}

bool BackendAgentLocalCommandStore::persistResult(const BackendAgentCommandResult& result)
{
    auto state=readState(result.commandId); if(!state || result.requestFingerprint!=state->envelope.requestFingerprint ||
        result.jobId!=state->envelope.jobId || result.attemptId!=state->envelope.attemptId ||
        result.claimEpoch!=state->envelope.claimEpoch || result.backendId!=state->envelope.backendId ||
        result.agentId!=state->envelope.agentId || result.agentInstanceId!=state->envelope.agentInstanceId ||
        result.backendGeneration!=state->envelope.backendGeneration ||
        result.boundedDiagnostics.size()>MaxDiagnosticsBytes) return false;
    if (state->resultPresent)
    {
        return state->result.resultCategory==result.resultCategory &&
            state->result.dispatchState==result.dispatchState &&
            state->result.verificationState==result.verificationState &&
            state->result.errorCategory==result.errorCategory &&
            state->result.retryClassification==result.retryClassification &&
            state->result.boundedDiagnostics==result.boundedDiagnostics;
    }
    if (static_cast<int>(result.dispatchState)<static_cast<int>(state->dispatchState)) return false;
    state->dispatchState=result.dispatchState; state->result=result; state->resultPresent=true;
    state->resultAcknowledged=false; return writeState(*state);
}

std::vector<BackendAgentCommandResult> BackendAgentLocalCommandStore::pendingResults() const
{
    std::vector<BackendAgentCommandResult> results; std::error_code error;
    if (!std::filesystem::is_directory(stateDirectory_,error)) return results;
    for (const auto& entry:std::filesystem::directory_iterator(stateDirectory_,error))
    {
        if(error) break; if(!entry.is_regular_file() || entry.path().extension()!=".command") continue;
        const auto state=readState(entry.path().stem().string());
        if(state && state->resultPresent && !state->resultAcknowledged) results.push_back(state->result);
    }
    std::sort(results.begin(),results.end(),[](const auto& left,const auto& right){
        if(left.completedAt!=right.completedAt) return left.completedAt<right.completedAt;
        return left.commandId<right.commandId;
    });
    return results;
}

bool BackendAgentLocalCommandStore::acknowledgeResult(
    const std::string& commandId,const std::string& requestFingerprint)
{
    auto state=readState(commandId); if(!state || !state->resultPresent ||
        state->envelope.requestFingerprint!=requestFingerprint) return false;
    state->resultAcknowledged=true; return writeState(*state);
}

#include "BackendAgentCommandDelivery.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeProbe.h"
#include "BackendAgentNativeTimerCreatePayload.h"
#include "BackendAgentNativeTimerDeleteAdvertisement.h"
#include "BackendAgentNativeTimerDeletePayload.h"
#include "BackendAgentNativeTimerModifyPayload.h"
#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <limits>
#include <sstream>
#include <string>

namespace
{
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(statement,index,value.c_str(),-1,SQLITE_TRANSIENT)==SQLITE_OK;
}
bool bindInt(sqlite3_stmt* statement, int index, std::int64_t value)
{
    return sqlite3_bind_int64(statement,index,value)==SQLITE_OK;
}
std::string text(sqlite3_stmt* statement, int column)
{
    const unsigned char* value=sqlite3_column_text(statement,column);
    return value?std::string(reinterpret_cast<const char*>(value)):std::string();
}
bool done(sqlite3_stmt* statement)
{
    const int result=sqlite3_step(statement);
    sqlite3_finalize(statement);
    return result==SQLITE_DONE;
}
std::string identifiers(const std::vector<std::string>& values)
{
    std::ostringstream output;
    for (std::size_t index=0; index<values.size(); ++index)
    {
        if (index!=0) output << ',';
        output << values[index];
    }
    return output.str();
}
bool parseIdentifiers(const std::string& encoded,std::vector<std::string>& values)
{
    values.clear();
    if(encoded.empty())return false;
    std::size_t offset=0;
    while(offset<=encoded.size())
    {
        const std::size_t separator=encoded.find(',',offset);
        const std::string value=separator==std::string::npos
            ? encoded.substr(offset):encoded.substr(offset,separator-offset);
        if(!backendAgentCommandSafeIdentifier(value)||
            std::find(values.begin(),values.end(),value)!=values.end())return false;
        values.push_back(value);
        if(values.size()>64)return false;
        if(separator==std::string::npos)break;
        offset=separator+1;
    }
    return !values.empty();
}
BackendAgentCommandAssignment readAssignment(sqlite3_stmt* s)
{
    BackendAgentCommandAssignment a;
    a.present=true; a.protocolVersion=text(s,0); a.requestId=text(s,1); a.correlationId=text(s,2);
    a.operationId=text(s,3); a.jobId=text(s,4); a.attemptId=text(s,5); a.claimEpoch=static_cast<std::uint64_t>(sqlite3_column_int64(s,6));
    a.commandId=text(s,7); a.backendId=text(s,8); a.agentId=text(s,9); a.agentInstanceId=text(s,10);
    a.backendGeneration=static_cast<std::uint64_t>(sqlite3_column_int64(s,11)); a.commandType=text(s,12);
    a.payloadVersion=static_cast<std::uint64_t>(sqlite3_column_int64(s,13)); a.payload=text(s,14);
    a.requestFingerprint=text(s,15); a.verificationPolicy=text(s,16); a.assignedAt=sqlite3_column_int64(s,17); a.deadline=sqlite3_column_int64(s,18);
    return a;
}
constexpr const char* AssignmentColumns=
    "protocol_version,request_id,correlation_id,operation_id,job_id,attempt_id,claim_epoch,command_id,backend_id,agent_id,agent_instance_id,backend_generation,command_type,payload_version,payload,request_fingerprint,verification_policy,assigned_at,deadline";
}

BackendAgentCommandRepository::BackendAgentCommandRepository(Database& database):database_(database){}

bool BackendAgentCommandRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_agent_commands ("
        "command_id TEXT PRIMARY KEY,protocol_version TEXT NOT NULL,request_id TEXT NOT NULL,correlation_id TEXT NOT NULL,"
        "operation_id TEXT NOT NULL,job_id TEXT NOT NULL,attempt_id TEXT NOT NULL,claim_epoch INTEGER NOT NULL,"
        "backend_id TEXT NOT NULL,agent_id TEXT NOT NULL,agent_instance_id TEXT NOT NULL,backend_generation INTEGER NOT NULL,"
        "command_type TEXT NOT NULL,payload_version INTEGER NOT NULL,payload TEXT NOT NULL,request_fingerprint TEXT NOT NULL,"
        "verification_policy TEXT NOT NULL,assigned_at INTEGER NOT NULL,deadline INTEGER NOT NULL,state TEXT NOT NULL DEFAULT 'assigned',"
        "replay_requested INTEGER NOT NULL DEFAULT 0,last_delivered_at INTEGER NOT NULL DEFAULT 0,delivery_count INTEGER NOT NULL DEFAULT 0,receipt_replay_count INTEGER NOT NULL DEFAULT 0,result_replay_count INTEGER NOT NULL DEFAULT 0,updated_at INTEGER NOT NULL,"
        "UNIQUE(job_id,attempt_id,claim_epoch),CHECK(state IN ('assigned','received','completed','expired','waiting_reconciliation'))"
        ");") &&
        database_.execute("CREATE INDEX IF NOT EXISTS idx_backend_agent_commands_delivery ON backend_agent_commands(backend_id,state,replay_requested,assigned_at);") &&
        database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_agent_command_capabilities ("
        "backend_id TEXT NOT NULL,agent_id TEXT NOT NULL,agent_instance_id TEXT NOT NULL,backend_generation INTEGER NOT NULL,"
        "command_type TEXT NOT NULL,published_at INTEGER NOT NULL,PRIMARY KEY(backend_id,command_type));") &&
        database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_agent_local_provider_facts ("
        "backend_id TEXT NOT NULL,agent_id TEXT NOT NULL,agent_instance_id TEXT NOT NULL,backend_generation INTEGER NOT NULL,"
        "provider_id TEXT NOT NULL,provider_kind TEXT NOT NULL,provider_instance_epoch TEXT NOT NULL,provider_generation INTEGER NOT NULL,"
        "capability_revision INTEGER NOT NULL,available INTEGER NOT NULL,capabilities TEXT NOT NULL,observed_at INTEGER NOT NULL,"
        "PRIMARY KEY(backend_id,provider_id));") &&
        database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_agent_local_provider_ownership ("
        "backend_id TEXT NOT NULL,authority_domain TEXT NOT NULL,provider_id TEXT NOT NULL,provider_kind TEXT NOT NULL,"
        "ownership_generation INTEGER NOT NULL,allowed_capabilities TEXT NOT NULL,active INTEGER NOT NULL,updated_at INTEGER NOT NULL,"
        "PRIMARY KEY(backend_id,authority_domain));") &&
        database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_agent_command_provider_selections ("
        "command_id TEXT PRIMARY KEY,selection_identity TEXT NOT NULL,backend_id TEXT NOT NULL,authority_domain TEXT NOT NULL,"
        "provider_id TEXT NOT NULL,provider_kind TEXT NOT NULL,ownership_generation INTEGER NOT NULL,provider_instance_epoch TEXT NOT NULL,"
        "provider_generation INTEGER NOT NULL,capability_revision INTEGER NOT NULL,required_capability TEXT NOT NULL,"
        "FOREIGN KEY(command_id) REFERENCES backend_agent_commands(command_id));") &&
        database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_agent_command_receipts ("
        "command_id TEXT PRIMARY KEY,receipt_identity TEXT NOT NULL,receipt_category TEXT NOT NULL,reason_code TEXT NOT NULL,received_at INTEGER NOT NULL,"
        "FOREIGN KEY(command_id) REFERENCES backend_agent_commands(command_id));") &&
        database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_agent_command_results ("
        "command_id TEXT PRIMARY KEY,result_identity TEXT NOT NULL,dispatch_state TEXT NOT NULL,verification_state TEXT NOT NULL,"
        "result_category TEXT NOT NULL,error_category TEXT NOT NULL,retry_classification TEXT NOT NULL,bounded_diagnostics TEXT NOT NULL,completed_at INTEGER NOT NULL,"
        "FOREIGN KEY(command_id) REFERENCES backend_agent_commands(command_id));") &&
        database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_agent_command_faults ("
        "backend_id TEXT PRIMARY KEY,drop_next_receipt INTEGER NOT NULL DEFAULT 0,drop_next_result INTEGER NOT NULL DEFAULT 0); ") &&
        database_.execute(
        "DROP TRIGGER IF EXISTS trg_backend_agent_timer_delete_dormant_capability;");
}

bool BackendAgentCommandRepository::insertAssignment(
    const BackendAgentCommandAssignment& a,
    const vdrsuite::agent::BackendAgentLocalProviderSelection* selection)
{
    using namespace vdrsuite::agent;
    if (!backendAgentCommandValidAssignment(a) ||
        (selection != nullptr &&
         (!backendAgentLocalProviderValidSelection(*selection) ||
          selection->backendId != a.backendId ||
          selection->requiredCapability != a.commandType))) return false;
    auto transactionLease=database_.acquireTransactionLease();
    if(!database_.execute("BEGIN IMMEDIATE;"))return false;
    sqlite3_stmt* s=nullptr;
    const std::string sql=std::string("INSERT INTO backend_agent_commands (")+AssignmentColumns+",updated_at) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
    bool ok=sqlite3_prepare_v2(database_.handle(),sql.c_str(),-1,&s,nullptr)==SQLITE_OK&&
        bindText(s,1,a.protocolVersion)&&bindText(s,2,a.requestId)&&bindText(s,3,a.correlationId)&&bindText(s,4,a.operationId)&&
        bindText(s,5,a.jobId)&&bindText(s,6,a.attemptId)&&bindInt(s,7,static_cast<std::int64_t>(a.claimEpoch))&&bindText(s,8,a.commandId)&&
        bindText(s,9,a.backendId)&&bindText(s,10,a.agentId)&&bindText(s,11,a.agentInstanceId)&&bindInt(s,12,static_cast<std::int64_t>(a.backendGeneration))&&
        bindText(s,13,a.commandType)&&bindInt(s,14,static_cast<std::int64_t>(a.payloadVersion))&&bindText(s,15,a.payload)&&bindText(s,16,a.requestFingerprint)&&
        bindText(s,17,a.verificationPolicy)&&bindInt(s,18,a.assignedAt)&&bindInt(s,19,a.deadline)&&bindInt(s,20,a.assignedAt)&&done(s);
    if(ok&&selection!=nullptr)
    {
        const std::string identity=backendAgentLocalProviderSelectionIdentity(*selection);
        sqlite3_stmt* selected=nullptr;
        const char* selectedSql="INSERT INTO backend_agent_command_provider_selections(command_id,selection_identity,backend_id,authority_domain,provider_id,provider_kind,ownership_generation,provider_instance_epoch,provider_generation,capability_revision,required_capability) VALUES(?,?,?,?,?,?,?,?,?,?,?);";
        ok=!identity.empty()&&sqlite3_prepare_v2(database_.handle(),selectedSql,-1,&selected,nullptr)==SQLITE_OK&&
            bindText(selected,1,a.commandId)&&bindText(selected,2,identity)&&bindText(selected,3,selection->backendId)&&
            bindText(selected,4,selection->authorityDomain)&&bindText(selected,5,selection->providerId)&&bindText(selected,6,selection->providerKind)&&
            bindInt(selected,7,static_cast<std::int64_t>(selection->ownershipGeneration))&&bindText(selected,8,selection->providerInstanceEpoch)&&
            bindInt(selected,9,static_cast<std::int64_t>(selection->providerGeneration))&&bindInt(selected,10,static_cast<std::int64_t>(selection->capabilityRevision))&&
            bindText(selected,11,selection->requiredCapability)&&done(selected);
    }
    if(!ok||!database_.execute("COMMIT;")){database_.execute("ROLLBACK;");return false;}
    return true;
}

std::optional<BackendAgentCommandAssignment>
BackendAgentCommandRepository::findAssignmentForOperation(
    const std::string& backendId,
    const std::string& operationId,
    const std::string& commandType) const
{
    if (!backendAgentCommandSafeIdentifier(backendId) ||
        !backendAgentCommandSafeIdentifier(operationId) ||
        !backendAgentCommandSafeIdentifier(commandType))
        return std::nullopt;

    sqlite3_stmt* statement = nullptr;
    const std::string sql = std::string("SELECT ") + AssignmentColumns +
        " FROM backend_agent_commands WHERE backend_id=? AND operation_id=? "
        "AND command_type=? ORDER BY assigned_at,command_id LIMIT 1;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, backendId) ||
        !bindText(statement, 2, operationId) ||
        !bindText(statement, 3, commandType))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::optional<BackendAgentCommandAssignment> result;
    if (sqlite3_step(statement) == SQLITE_ROW)
        result = readAssignment(statement);
    sqlite3_finalize(statement);
    return result;
}

std::optional<vdrsuite::agent::BackendAgentLocalProviderSelection>
BackendAgentCommandRepository::localProviderSelectionForCommand(
    const std::string& commandId) const
{
    using namespace vdrsuite::agent;
    if (!backendAgentCommandSafeIdentifier(commandId)) return std::nullopt;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT selection_identity,backend_id,authority_domain,provider_id,"
        "provider_kind,ownership_generation,provider_instance_epoch,"
        "provider_generation,capability_revision,required_capability "
        "FROM backend_agent_command_provider_selections WHERE command_id=?;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, commandId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return std::nullopt;
    }

    if (sqlite3_step(statement) != SQLITE_ROW)
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    const std::string identity = text(statement, 0);
    BackendAgentLocalProviderSelection selection;
    selection.backendId = text(statement, 1);
    selection.authorityDomain = text(statement, 2);
    selection.providerId = text(statement, 3);
    selection.providerKind = text(statement, 4);
    selection.ownershipGeneration = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 5));
    selection.providerInstanceEpoch = text(statement, 6);
    selection.providerGeneration = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 7));
    selection.capabilityRevision = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 8));
    selection.requiredCapability = text(statement, 9);
    sqlite3_finalize(statement);

    if (!backendAgentLocalProviderValidSelection(selection) ||
        identity != backendAgentLocalProviderSelectionIdentity(selection))
        return std::nullopt;
    return selection;
}


bool BackendAgentCommandRepository::hasCapability(const std::string& backendId,const std::string& agentId,const std::string& agentInstanceId,std::uint64_t backendGeneration,const std::string& commandType) const
{
    sqlite3_stmt* statement=nullptr;
    const char* sql="SELECT 1 FROM backend_agent_command_capabilities WHERE backend_id=? AND agent_id=? AND agent_instance_id=? AND backend_generation=? AND command_type=? LIMIT 1;";
    const bool prepared=sqlite3_prepare_v2(database_.handle(),sql,-1,&statement,nullptr)==SQLITE_OK&&
        bindText(statement,1,backendId)&&bindText(statement,2,agentId)&&bindText(statement,3,agentInstanceId)&&
        bindInt(statement,4,static_cast<std::int64_t>(backendGeneration))&&bindText(statement,5,commandType);
    if(!prepared){if(statement)sqlite3_finalize(statement);return false;}
    const bool present=sqlite3_step(statement)==SQLITE_ROW;
    sqlite3_finalize(statement);
    return present;
}

BackendAgentLocalProviderOwnershipStatus BackendAgentCommandRepository::localProviderOwnershipStatus(
    const std::string& backendId,const std::string& authorityDomain) const
{
    BackendAgentLocalProviderOwnershipStatus status;
    sqlite3_stmt* s=nullptr;
    const char* sql="SELECT provider_id,provider_kind,ownership_generation,allowed_capabilities,active FROM backend_agent_local_provider_ownership WHERE backend_id=? AND authority_domain=?;";
    if(sqlite3_prepare_v2(database_.handle(),sql,-1,&s,nullptr)!=SQLITE_OK||
       !bindText(s,1,backendId)||!bindText(s,2,authorityDomain))
    {if(s)sqlite3_finalize(s);return status;}
    if(sqlite3_step(s)==SQLITE_ROW)
    {
        status.present=true;
        status.active=sqlite3_column_int(s,4)!=0;
        auto& ownership=status.ownership;
        ownership.backendId=backendId;
        ownership.authorityDomain=authorityDomain;
        ownership.providerId=text(s,0);
        ownership.providerKind=text(s,1);
        ownership.ownershipGeneration=static_cast<std::uint64_t>(sqlite3_column_int64(s,2));
        if(!parseIdentifiers(text(s,3),ownership.allowedCapabilities)||
           !vdrsuite::agent::backendAgentLocalProviderValidOwnership(ownership))status={};
    }
    sqlite3_finalize(s);
    return status;
}

bool BackendAgentCommandRepository::setLocalProviderOwnership(
    const std::string& backendId,const std::string& authorityDomain,
    const std::string& providerId,const std::string& providerKind,
    const std::vector<std::string>& allowedCapabilities,std::int64_t updatedAt,
    vdrsuite::agent::BackendAgentLocalProviderOwnership& ownership,std::string& reason)
{
    using namespace vdrsuite::agent;
    BackendAgentLocalProviderOwnership candidate;
    candidate.backendId=backendId;candidate.authorityDomain=authorityDomain;
    candidate.providerId=providerId;candidate.providerKind=providerKind;
    candidate.ownershipGeneration=1;candidate.allowedCapabilities=allowedCapabilities;
    if(updatedAt<=0||!backendAgentLocalProviderValidOwnership(candidate))
    {reason="invalid_local_provider_ownership";return false;}
    auto transactionLease=database_.acquireTransactionLease();
    const auto current=localProviderOwnershipStatus(backendId,authorityDomain);
    if(current.present&&current.active&&current.ownership.providerId==providerId&&
       current.ownership.providerKind==providerKind&&
       current.ownership.allowedCapabilities==allowedCapabilities)
    {ownership=current.ownership;reason="local_provider_ownership_unchanged";return true;}
    const std::uint64_t maximum=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    if(current.present&&current.ownership.ownershipGeneration>=maximum)
    {reason="local_provider_ownership_generation_exhausted";return false;}
    candidate.ownershipGeneration=current.present?current.ownership.ownershipGeneration+1:1;
    sqlite3_stmt* s=nullptr;
    const char* sql="INSERT INTO backend_agent_local_provider_ownership(backend_id,authority_domain,provider_id,provider_kind,ownership_generation,allowed_capabilities,active,updated_at) VALUES(?,?,?,?,?,?,1,?) ON CONFLICT(backend_id,authority_domain) DO UPDATE SET provider_id=excluded.provider_id,provider_kind=excluded.provider_kind,ownership_generation=excluded.ownership_generation,allowed_capabilities=excluded.allowed_capabilities,active=1,updated_at=excluded.updated_at;";
    const bool ok=sqlite3_prepare_v2(database_.handle(),sql,-1,&s,nullptr)==SQLITE_OK&&
        bindText(s,1,backendId)&&bindText(s,2,authorityDomain)&&bindText(s,3,providerId)&&bindText(s,4,providerKind)&&
        bindInt(s,5,static_cast<std::int64_t>(candidate.ownershipGeneration))&&bindText(s,6,identifiers(allowedCapabilities))&&bindInt(s,7,updatedAt)&&done(s);
    if(!ok){reason="local_provider_ownership_persist_failed";return false;}
    ownership=candidate;reason="local_provider_ownership_set";return true;
}

bool BackendAgentCommandRepository::clearLocalProviderOwnership(
    const std::string& backendId,const std::string& authorityDomain,
    std::int64_t updatedAt,std::string& reason)
{
    if(updatedAt<=0||!backendAgentCommandSafeIdentifier(backendId)||
       !backendAgentCommandSafeIdentifier(authorityDomain))
    {reason="invalid_local_provider_ownership_clear";return false;}
    auto transactionLease=database_.acquireTransactionLease();
    const auto current=localProviderOwnershipStatus(backendId,authorityDomain);
    if(!current.present||!current.active)
    {reason="local_provider_ownership_absent";return true;}
    if(current.ownership.ownershipGeneration>=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
    {reason="local_provider_ownership_generation_exhausted";return false;}
    sqlite3_stmt* s=nullptr;
    const char* sql="UPDATE backend_agent_local_provider_ownership SET active=0,ownership_generation=?,updated_at=? WHERE backend_id=? AND authority_domain=? AND active=1;";
    const bool ok=sqlite3_prepare_v2(database_.handle(),sql,-1,&s,nullptr)==SQLITE_OK&&
        bindInt(s,1,static_cast<std::int64_t>(current.ownership.ownershipGeneration+1))&&
        bindInt(s,2,updatedAt)&&bindText(s,3,backendId)&&bindText(s,4,authorityDomain)&&done(s)&&
        sqlite3_changes(database_.handle())==1;
    reason=ok?"local_provider_ownership_cleared":"local_provider_ownership_clear_failed";
    return ok;
}

std::optional<vdrsuite::agent::BackendAgentLocalProviderSelection>
BackendAgentCommandRepository::selectLocalProvider(
    const std::string& backendId,const std::string& agentId,
    const std::string& agentInstanceId,std::uint64_t backendGeneration,
    const std::string& authorityDomain,const std::string& requiredCapability,
    std::string& reason) const
{
    using namespace vdrsuite::agent;
    const auto status=localProviderOwnershipStatus(backendId,authorityDomain);
    if(!status.present||!status.active)
    {reason="local_provider_ownership_required";return std::nullopt;}
    sqlite3_stmt* s=nullptr;
    const char* sql="SELECT provider_kind,provider_instance_epoch,provider_generation,capability_revision,available,capabilities FROM backend_agent_local_provider_facts WHERE backend_id=? AND agent_id=? AND agent_instance_id=? AND backend_generation=? AND provider_id=?;";
    const auto& ownership=status.ownership;
    if(sqlite3_prepare_v2(database_.handle(),sql,-1,&s,nullptr)!=SQLITE_OK||
       !bindText(s,1,backendId)||!bindText(s,2,agentId)||!bindText(s,3,agentInstanceId)||
       !bindInt(s,4,static_cast<std::int64_t>(backendGeneration))||!bindText(s,5,ownership.providerId))
    {if(s)sqlite3_finalize(s);reason="local_provider_facts_unavailable";return std::nullopt;}
    if(sqlite3_step(s)!=SQLITE_ROW)
    {sqlite3_finalize(s);reason="local_provider_facts_required";return std::nullopt;}
    BackendAgentLocalProviderFacts facts;
    facts.providerId=ownership.providerId;facts.providerKind=text(s,0);
    facts.providerInstanceEpoch=text(s,1);
    facts.providerGeneration=static_cast<std::uint64_t>(sqlite3_column_int64(s,2));
    facts.capabilityRevision=static_cast<std::uint64_t>(sqlite3_column_int64(s,3));
    facts.available=sqlite3_column_int(s,4)!=0;
    const bool parsed=parseIdentifiers(text(s,5),facts.capabilities);
    sqlite3_finalize(s);
    if(!parsed||!backendAgentLocalProviderValidFacts(facts))
    {reason="local_provider_facts_invalid";return std::nullopt;}
    const auto selection=backendAgentLocalProviderSelect(
        ownership,facts,requiredCapability,reason);
    return backendAgentLocalProviderValidSelection(selection)
        ?std::optional<BackendAgentLocalProviderSelection>(selection):std::nullopt;
}

bool BackendAgentCommandRepository::localProviderSelectionCurrent(
    const std::string& commandId,std::string& reason) const
{
    using namespace vdrsuite::agent;
    sqlite3_stmt* s=nullptr;
    const char* sql="SELECT c.command_type,c.agent_id,c.agent_instance_id,c.backend_generation,s.backend_id,s.authority_domain,s.provider_id,s.provider_kind,s.ownership_generation,s.provider_instance_epoch,s.provider_generation,s.capability_revision,s.required_capability,s.selection_identity FROM backend_agent_commands c LEFT JOIN backend_agent_command_provider_selections s ON s.command_id=c.command_id WHERE c.command_id=?;";
    if(sqlite3_prepare_v2(database_.handle(),sql,-1,&s,nullptr)!=SQLITE_OK||!bindText(s,1,commandId))
    {if(s)sqlite3_finalize(s);reason="local_provider_selection_lookup_failed";return false;}
    if(sqlite3_step(s)!=SQLITE_ROW)
    {sqlite3_finalize(s);reason="command_not_found";return false;}
    const std::string commandType=text(s,0);
    const bool nativeProbe=commandType=="vdr.native.probe";
    const bool timerCreate=commandType==kBackendAgentNativeTimerCreateCommandType;
    const bool timerDelete=commandType==kBackendAgentNativeTimerDeleteCommandType;
    const bool timerModify=
        commandType==kBackendAgentNativeTimerUpdateCommandType||
        commandType==kBackendAgentNativeTimerToggleCommandType;
    if(!nativeProbe&&!timerCreate&&!timerDelete&&!timerModify)
    {sqlite3_finalize(s);reason="local_provider_selection_not_required";return true;}
    if(sqlite3_column_type(s,4)==SQLITE_NULL)
    {sqlite3_finalize(s);reason="local_provider_selection_required";return false;}
    const std::string agentId=text(s,1),instance=text(s,2);
    const std::uint64_t backendGeneration=static_cast<std::uint64_t>(sqlite3_column_int64(s,3));
    BackendAgentLocalProviderSelection selection;
    selection.backendId=text(s,4);selection.authorityDomain=text(s,5);
    selection.providerId=text(s,6);selection.providerKind=text(s,7);
    selection.ownershipGeneration=static_cast<std::uint64_t>(sqlite3_column_int64(s,8));
    selection.providerInstanceEpoch=text(s,9);
    selection.providerGeneration=static_cast<std::uint64_t>(sqlite3_column_int64(s,10));
    selection.capabilityRevision=static_cast<std::uint64_t>(sqlite3_column_int64(s,11));
    selection.requiredCapability=text(s,12);
    const std::string identity=text(s,13);
    sqlite3_finalize(s);
    if(!backendAgentLocalProviderValidSelection(selection)||
       identity!=backendAgentLocalProviderSelectionIdentity(selection))
    {reason="local_provider_selection_invalid";return false;}
    if(timerCreate&&
       (selection.authorityDomain!=kBackendAgentNativeTimerCreateAuthorityDomain||
        selection.providerId!=kBackendAgentNativeTimerCreateProviderId||
        selection.providerKind!=kBackendAgentNativeTimerCreateProviderKind||
        selection.requiredCapability!=kBackendAgentNativeTimerCreateCapability))
    {reason="native_timer_create_provider_selection_mismatch";return false;}
    if(timerDelete&&
       (selection.authorityDomain!=kBackendAgentNativeTimerDeleteAuthorityDomain||
        selection.providerId!=kBackendAgentNativeTimerDeleteProviderId||
        selection.providerKind!=kBackendAgentNativeTimerDeleteProviderKind||
        selection.requiredCapability!=kBackendAgentNativeTimerDeleteCapability))
    {reason="native_timer_delete_provider_selection_mismatch";return false;}
    if(timerModify&&
       (selection.authorityDomain!=kBackendAgentNativeTimerModifyAuthorityDomain||
        selection.providerId!=kBackendAgentNativeTimerModifyProviderId||
        selection.providerKind!=kBackendAgentNativeTimerModifyProviderKind||
        selection.requiredCapability!=
          (commandType==kBackendAgentNativeTimerToggleCommandType
            ?kBackendAgentNativeTimerToggleCapability
            :kBackendAgentNativeTimerUpdateCapability)))
    {reason="native_timer_modify_provider_selection_mismatch";return false;}
    const auto current=selectLocalProvider(
        selection.backendId,agentId,instance,backendGeneration,
        selection.authorityDomain,selection.requiredCapability,reason);
    if(!current.has_value())return false;
    if(!backendAgentLocalProviderSameFence(selection,*current))
    {reason="local_provider_selection_stale";return false;}
    reason="local_provider_selection_current";return true;
}

BackendAgentCommandPollResult BackendAgentCommandRepository::poll(const BackendAgentCommandPollRequest& request,const std::string& agentId,std::int64_t now)
{
    using namespace vdrsuite::agent;
    auto transactionLease = database_.acquireTransactionLease();
    BackendAgentCommandPollResult result; result.accepted=true; result.reasonCode="no_command_available";
    std::string advertisementReason;
    if(!backendAgentNativeTimerDeleteAdvertisementValid(request,advertisementReason))
    {result.accepted=false;result.reasonCode=advertisementReason;return result;}
    if (!database_.execute("BEGIN IMMEDIATE;")) { result.accepted=false; result.reasonCode="command_database_unavailable"; return result; }
    bool ok=database_.execute(
        "DROP TRIGGER IF EXISTS trg_backend_agent_timer_delete_dormant_capability;");
    sqlite3_stmt* clear=nullptr;
    const char* clearSql="DELETE FROM backend_agent_command_capabilities WHERE backend_id=?;";
    ok=ok&&sqlite3_prepare_v2(database_.handle(),clearSql,-1,&clear,nullptr)==SQLITE_OK&&
        bindText(clear,1,request.backendId)&&done(clear);
    sqlite3_stmt* clearProviders=nullptr;
    const char* clearProvidersSql="DELETE FROM backend_agent_local_provider_facts WHERE backend_id=?;";
    ok=ok&&sqlite3_prepare_v2(database_.handle(),clearProvidersSql,-1,&clearProviders,nullptr)==SQLITE_OK&&
        bindText(clearProviders,1,request.backendId)&&done(clearProviders);
    for (const std::string& type:request.supportedCommandTypes)
    {
        sqlite3_stmt* cap=nullptr;
        const char* sql="INSERT INTO backend_agent_command_capabilities(backend_id,agent_id,agent_instance_id,backend_generation,command_type,published_at) VALUES(?,?,?,?,?,?);";
        if (sqlite3_prepare_v2(database_.handle(),sql,-1,&cap,nullptr)!=SQLITE_OK || !bindText(cap,1,request.backendId)||!bindText(cap,2,agentId)||
            !bindText(cap,3,request.agentInstanceId)||!bindInt(cap,4,static_cast<std::int64_t>(request.backendGeneration))||!bindText(cap,5,type)||!bindInt(cap,6,now)||!done(cap)) { ok=false; break; }
    }
    for(const auto& facts:request.localProviders)
    {
        if(!ok)break;
        if(!backendAgentLocalProviderValidFacts(facts)){ok=false;break;}
        sqlite3_stmt* provider=nullptr;
        const char* sql="INSERT INTO backend_agent_local_provider_facts(backend_id,agent_id,agent_instance_id,backend_generation,provider_id,provider_kind,provider_instance_epoch,provider_generation,capability_revision,available,capabilities,observed_at) VALUES(?,?,?,?,?,?,?,?,?,?,?,?);";
        if(sqlite3_prepare_v2(database_.handle(),sql,-1,&provider,nullptr)!=SQLITE_OK||
           !bindText(provider,1,request.backendId)||!bindText(provider,2,agentId)||!bindText(provider,3,request.agentInstanceId)||
           !bindInt(provider,4,static_cast<std::int64_t>(request.backendGeneration))||!bindText(provider,5,facts.providerId)||
           !bindText(provider,6,facts.providerKind)||!bindText(provider,7,facts.providerInstanceEpoch)||
           !bindInt(provider,8,static_cast<std::int64_t>(facts.providerGeneration))||
           !bindInt(provider,9,static_cast<std::int64_t>(facts.capabilityRevision))||
           !bindInt(provider,10,facts.available?1:0)||!bindText(provider,11,identifiers(facts.capabilities))||
           !bindInt(provider,12,now)||!done(provider)){ok=false;break;}
    }
    if (ok)
    {
        sqlite3_stmt* expire=nullptr;
        const char* sql="UPDATE backend_agent_commands SET state='expired',updated_at=? WHERE backend_id=? AND state IN('assigned','received') AND deadline<?;";
        if (sqlite3_prepare_v2(database_.handle(),sql,-1,&expire,nullptr)!=SQLITE_OK||!bindInt(expire,1,now)||!bindText(expire,2,request.backendId)||!bindInt(expire,3,now)||!done(expire)) ok=false;
    }
    if (ok)
    {
        sqlite3_stmt* select=nullptr;
        const std::string sql=std::string("SELECT ")+AssignmentColumns+" FROM backend_agent_commands c WHERE c.backend_id=? AND c.agent_id=? AND c.agent_instance_id=? AND c.backend_generation=? AND c.deadline>=? AND ((c.state='assigned' AND NOT EXISTS(SELECT 1 FROM backend_agent_command_receipts r WHERE r.command_id=c.command_id)) OR c.replay_requested=1) AND EXISTS(SELECT 1 FROM backend_agent_command_capabilities x WHERE x.backend_id=c.backend_id AND x.agent_id=c.agent_id AND x.agent_instance_id=c.agent_instance_id AND x.backend_generation=c.backend_generation AND x.command_type=c.command_type) AND (c.command_type!='vdr.native.probe' OR EXISTS(SELECT 1 FROM backend_agent_command_provider_selections s JOIN backend_agent_local_provider_ownership o ON o.backend_id=s.backend_id AND o.authority_domain=s.authority_domain JOIN backend_agent_local_provider_facts f ON f.backend_id=s.backend_id AND f.provider_id=s.provider_id WHERE s.command_id=c.command_id AND o.active=1 AND o.provider_id=s.provider_id AND o.provider_kind=s.provider_kind AND o.ownership_generation=s.ownership_generation AND f.agent_id=c.agent_id AND f.agent_instance_id=c.agent_instance_id AND f.backend_generation=c.backend_generation AND f.provider_kind=s.provider_kind AND f.provider_instance_epoch=s.provider_instance_epoch AND f.provider_generation=s.provider_generation AND f.capability_revision=s.capability_revision AND f.available=1)) ORDER BY c.assigned_at LIMIT 1;";
        if (sqlite3_prepare_v2(database_.handle(),sql.c_str(),-1,&select,nullptr)!=SQLITE_OK||!bindText(select,1,request.backendId)||!bindText(select,2,agentId)||!bindText(select,3,request.agentInstanceId)||!bindInt(select,4,static_cast<std::int64_t>(request.backendGeneration))||!bindInt(select,5,now)) ok=false;
        else
        {
            if (sqlite3_step(select)==SQLITE_ROW) result.assignment=readAssignment(select);
            sqlite3_finalize(select);
        }
    }
    if(ok&&result.assignment.present&&
       (result.assignment.commandType==kBackendAgentNativeTimerCreateCommandType||
        result.assignment.commandType==kBackendAgentNativeTimerDeleteCommandType))
    {
        std::string providerReason;
        if(!localProviderSelectionCurrent(result.assignment.commandId,providerReason))
        {result.assignment={};result.reasonCode=providerReason;}
    }
    if (ok && result.assignment.present)
    {
        sqlite3_stmt* update=nullptr;
        const char* sql="UPDATE backend_agent_commands SET replay_requested=0,last_delivered_at=?,delivery_count=delivery_count+1,updated_at=? WHERE command_id=?;";
        if (sqlite3_prepare_v2(database_.handle(),sql,-1,&update,nullptr)!=SQLITE_OK||!bindInt(update,1,now)||!bindInt(update,2,now)||!bindText(update,3,result.assignment.commandId)||!done(update)) ok=false;
        else result.reasonCode="command_assigned";
    }
    if (!ok || !database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;"); result.accepted=false; result.assignment={}; result.reasonCode="command_database_unavailable";
    }
    return result;
}

BackendAgentCommandReceiptResult BackendAgentCommandRepository::acceptReceipt(const BackendAgentCommandReceipt& r)
{
    using namespace vdrsuite::agent;
    auto transactionLease = database_.acquireTransactionLease();
    BackendAgentCommandReceiptResult result;
    sqlite3_stmt* query=nullptr;
    const char* sql="SELECT job_id,attempt_id,claim_epoch,backend_id,agent_id,agent_instance_id,backend_generation,request_fingerprint,deadline,state,command_type,payload_version FROM backend_agent_commands WHERE command_id=?;";
    if (sqlite3_prepare_v2(database_.handle(),sql,-1,&query,nullptr)!=SQLITE_OK||!bindText(query,1,r.commandId)) { if(query)sqlite3_finalize(query); result.reasonCode="command_database_unavailable"; return result; }
    if (sqlite3_step(query)!=SQLITE_ROW) { sqlite3_finalize(query); result.reasonCode="command_not_found"; return result; }
    const bool match=text(query,0)==r.jobId&&text(query,1)==r.attemptId&&static_cast<std::uint64_t>(sqlite3_column_int64(query,2))==r.claimEpoch&&
        text(query,3)==r.backendId&&text(query,4)==r.agentId&&text(query,5)==r.agentInstanceId&&static_cast<std::uint64_t>(sqlite3_column_int64(query,6))==r.backendGeneration&&text(query,7)==r.requestFingerprint;
    const std::string commandType=text(query,10);
    const std::uint64_t payloadVersion=static_cast<std::uint64_t>(sqlite3_column_int64(query,11));
    sqlite3_finalize(query);
    if (!match) { result.reasonCode="command_receipt_fenced"; return result; }
    if(commandType=="vdr.native.probe")
    {
        if(payloadVersion==2)
        {
            std::string providerReason;
            if(!localProviderSelectionCurrent(r.commandId,providerReason))
            {result.reasonCode=providerReason;return result;}
        }
        else if(payloadVersion!=1)
        {
            result.reasonCode="local_provider_selection_required";
            return result;
        }
    }
    else if(commandType==kBackendAgentNativeTimerCreateCommandType&&
            payloadVersion!=kBackendAgentNativeTimerCreatePayloadVersion)
    {
        result.reasonCode="local_provider_selection_required";
        return result;
    }
    else if(commandType==kBackendAgentNativeTimerDeleteCommandType&&
            payloadVersion!=kBackendAgentNativeTimerDeletePayloadVersion)
    {
        result.reasonCode="local_provider_selection_required";
        return result;
    }
    else if((commandType==kBackendAgentNativeTimerUpdateCommandType||
             commandType==kBackendAgentNativeTimerToggleCommandType)&&
            payloadVersion!=kBackendAgentNativeTimerModifyPayloadVersion)
    {
        result.reasonCode="local_provider_selection_required";
        return result;
    }
    const std::string identity=backendAgentCommandReceiptIdentity(r);
    sqlite3_stmt* existing=nullptr;
    if (sqlite3_prepare_v2(database_.handle(),"SELECT receipt_identity FROM backend_agent_command_receipts WHERE command_id=?;",-1,&existing,nullptr)!=SQLITE_OK||!bindText(existing,1,r.commandId)) { if(existing)sqlite3_finalize(existing); result.reasonCode="command_database_unavailable"; return result; }
    if (sqlite3_step(existing)==SQLITE_ROW)
    {
        result.accepted=text(existing,0)==identity; result.replayed=result.accepted; result.reasonCode=result.accepted?"command_receipt_replayed":"command_receipt_conflict"; sqlite3_finalize(existing); if(result.accepted){sqlite3_stmt* replay=nullptr;const char* replaySql="UPDATE backend_agent_commands SET receipt_replay_count=receipt_replay_count+1 WHERE command_id=?;";if(sqlite3_prepare_v2(database_.handle(),replaySql,-1,&replay,nullptr)!=SQLITE_OK||!bindText(replay,1,r.commandId)||!done(replay)){result.accepted=false;result.replayed=false;result.reasonCode="command_database_unavailable";}} return result;
    }
    sqlite3_finalize(existing);
    if(commandType==kBackendAgentNativeTimerCreateCommandType||
       commandType==kBackendAgentNativeTimerDeleteCommandType||
       commandType==kBackendAgentNativeTimerUpdateCommandType||
       commandType==kBackendAgentNativeTimerToggleCommandType)
    {
        std::string providerReason;
        if(!localProviderSelectionCurrent(r.commandId,providerReason))
        {result.reasonCode=providerReason;return result;}
    }
    if (!database_.execute("BEGIN IMMEDIATE;")) { result.reasonCode="command_database_unavailable"; return result; }
    sqlite3_stmt* insert=nullptr;
    const char* insertSql="INSERT INTO backend_agent_command_receipts(command_id,receipt_identity,receipt_category,reason_code,received_at) VALUES(?,?,?,?,?);";
    bool ok=sqlite3_prepare_v2(database_.handle(),insertSql,-1,&insert,nullptr)==SQLITE_OK&&bindText(insert,1,r.commandId)&&bindText(insert,2,identity)&&bindText(insert,3,r.receiptCategory)&&bindText(insert,4,r.reasonCode)&&bindInt(insert,5,r.receivedAt)&&done(insert);
    sqlite3_stmt* update=nullptr;
    const char* updateSql="UPDATE backend_agent_commands SET state='received',updated_at=? WHERE command_id=? AND state='assigned';";
    ok=ok&&sqlite3_prepare_v2(database_.handle(),updateSql,-1,&update,nullptr)==SQLITE_OK&&bindInt(update,1,r.receivedAt)&&bindText(update,2,r.commandId)&&done(update);
    if (!ok||!database_.execute("COMMIT;")) { database_.execute("ROLLBACK;"); result.reasonCode="command_database_unavailable"; return result; }
    result.accepted=true; result.reasonCode="command_receipt_accepted"; return result;
}

BackendAgentCommandResultAck BackendAgentCommandRepository::acceptResult(const BackendAgentCommandResult& r)
{
    using namespace vdrsuite::agent;
    auto transactionLease = database_.acquireTransactionLease();
    BackendAgentCommandResultAck result;
    sqlite3_stmt* query=nullptr;
    const char* sql="SELECT job_id,attempt_id,claim_epoch,backend_id,agent_id,agent_instance_id,backend_generation,request_fingerprint,command_type,payload_version,payload FROM backend_agent_commands WHERE command_id=?;";
    if (sqlite3_prepare_v2(database_.handle(),sql,-1,&query,nullptr)!=SQLITE_OK||!bindText(query,1,r.commandId)) { if(query)sqlite3_finalize(query); result.reasonCode="command_database_unavailable"; return result; }
    if (sqlite3_step(query)!=SQLITE_ROW) { sqlite3_finalize(query); result.reasonCode="command_not_found"; return result; }
    const bool match=text(query,0)==r.jobId&&text(query,1)==r.attemptId&&static_cast<std::uint64_t>(sqlite3_column_int64(query,2))==r.claimEpoch&&text(query,3)==r.backendId&&text(query,4)==r.agentId&&text(query,5)==r.agentInstanceId&&static_cast<std::uint64_t>(sqlite3_column_int64(query,6))==r.backendGeneration&&text(query,7)==r.requestFingerprint;
    const std::string commandType=text(query,8);
    const std::uint64_t payloadVersion=static_cast<std::uint64_t>(sqlite3_column_int64(query,9));
    const std::string payload=text(query,10);
    sqlite3_finalize(query);
    if(!match){result.reasonCode="command_result_fenced";return result;}
    if(commandType=="vdr.native.probe")
    {
        if(payloadVersion==2)
        {
            BackendAgentNativeProbePayload selectedPayload;
            std::string payloadReason;
            if(!backendAgentNativeProbeParseSelectedPayload(payload,selectedPayload,payloadReason))
            {result.reasonCode="local_provider_selection_invalid";return result;}
            const std::string expectedIdentity=backendAgentLocalProviderSelectionIdentity(
                selectedPayload.localProviderSelection);
            sqlite3_stmt* selected=nullptr;
            if(sqlite3_prepare_v2(database_.handle(),"SELECT selection_identity FROM backend_agent_command_provider_selections WHERE command_id=?;",-1,&selected,nullptr)!=SQLITE_OK||!bindText(selected,1,r.commandId))
            {if(selected)sqlite3_finalize(selected);result.reasonCode="command_database_unavailable";return result;}
            const bool recorded=sqlite3_step(selected)==SQLITE_ROW&&
                text(selected,0)==expectedIdentity;
            sqlite3_finalize(selected);
            if(!recorded)
            {result.reasonCode="local_provider_selection_required";return result;}
        }
        else if(payloadVersion!=1)
        {
            result.reasonCode="local_provider_selection_required";
            return result;
        }
    }
    else if(commandType==kBackendAgentNativeTimerCreateCommandType)
    {
        if(payloadVersion!=kBackendAgentNativeTimerCreatePayloadVersion)
        {result.reasonCode="local_provider_selection_required";return result;}
        BackendAgentNativeTimerCreatePayload createPayload;
        std::string payloadReason;
        if(!backendAgentNativeTimerCreateParsePayload(payload,createPayload,payloadReason))
        {result.reasonCode="local_provider_selection_invalid";return result;}
        const std::string expectedIdentity=backendAgentLocalProviderSelectionIdentity(
            createPayload.localProviderSelection);
        sqlite3_stmt* selected=nullptr;
        if(sqlite3_prepare_v2(database_.handle(),"SELECT selection_identity FROM backend_agent_command_provider_selections WHERE command_id=?;",-1,&selected,nullptr)!=SQLITE_OK||!bindText(selected,1,r.commandId))
        {if(selected)sqlite3_finalize(selected);result.reasonCode="command_database_unavailable";return result;}
        const bool recorded=sqlite3_step(selected)==SQLITE_ROW&&
            text(selected,0)==expectedIdentity;
        sqlite3_finalize(selected);
        if(!recorded)
        {result.reasonCode="local_provider_selection_required";return result;}
    }
    else if(commandType==kBackendAgentNativeTimerDeleteCommandType)
    {
        if(payloadVersion!=kBackendAgentNativeTimerDeletePayloadVersion)
        {result.reasonCode="local_provider_selection_required";return result;}
        BackendAgentNativeTimerDeletePayload deletePayload;
        std::string payloadReason;
        if(!backendAgentNativeTimerDeleteParsePayload(payload,deletePayload,payloadReason))
        {result.reasonCode="local_provider_selection_invalid";return result;}
        const std::string expectedIdentity=backendAgentLocalProviderSelectionIdentity(
            deletePayload.localProviderSelection);
        sqlite3_stmt* selected=nullptr;
        if(sqlite3_prepare_v2(database_.handle(),"SELECT selection_identity FROM backend_agent_command_provider_selections WHERE command_id=?;",-1,&selected,nullptr)!=SQLITE_OK||!bindText(selected,1,r.commandId))
        {if(selected)sqlite3_finalize(selected);result.reasonCode="command_database_unavailable";return result;}
        const bool recorded=sqlite3_step(selected)==SQLITE_ROW&&
            text(selected,0)==expectedIdentity;
        sqlite3_finalize(selected);
        if(!recorded)
        {result.reasonCode="local_provider_selection_required";return result;}
    }

    else if(commandType==kBackendAgentNativeTimerUpdateCommandType||
            commandType==kBackendAgentNativeTimerToggleCommandType)
    {
        if(payloadVersion!=kBackendAgentNativeTimerModifyPayloadVersion)
        {result.reasonCode="local_provider_selection_required";return result;}
        BackendAgentNativeTimerModifyPayload modifyPayload;
        std::string payloadReason;
        if(!backendAgentNativeTimerModifyParsePayload(payload,modifyPayload,payloadReason))
        {result.reasonCode="local_provider_selection_invalid";return result;}
        const std::string expectedIdentity=backendAgentLocalProviderSelectionIdentity(
            modifyPayload.localProviderSelection);
        sqlite3_stmt* selected=nullptr;
        if(sqlite3_prepare_v2(database_.handle(),"SELECT selection_identity FROM backend_agent_command_provider_selections WHERE command_id=?;",-1,&selected,nullptr)!=SQLITE_OK||!bindText(selected,1,r.commandId))
        {if(selected)sqlite3_finalize(selected);result.reasonCode="command_database_unavailable";return result;}
        const bool recorded=sqlite3_step(selected)==SQLITE_ROW&&
            text(selected,0)==expectedIdentity;
        sqlite3_finalize(selected);
        if(!recorded)
        {result.reasonCode="local_provider_selection_required";return result;}
    }
    sqlite3_stmt* receipt=nullptr;
    if(sqlite3_prepare_v2(database_.handle(),"SELECT 1 FROM backend_agent_command_receipts WHERE command_id=?;",-1,&receipt,nullptr)!=SQLITE_OK||!bindText(receipt,1,r.commandId)){if(receipt)sqlite3_finalize(receipt);result.reasonCode="command_database_unavailable";return result;}
    const bool hasReceipt=sqlite3_step(receipt)==SQLITE_ROW; sqlite3_finalize(receipt); if(!hasReceipt){result.reasonCode="command_receipt_required";return result;}
    const std::string identity=backendAgentCommandResultIdentity(r);
    sqlite3_stmt* existing=nullptr;
    if(sqlite3_prepare_v2(database_.handle(),"SELECT result_identity FROM backend_agent_command_results WHERE command_id=?;",-1,&existing,nullptr)!=SQLITE_OK||!bindText(existing,1,r.commandId)){if(existing)sqlite3_finalize(existing);result.reasonCode="command_database_unavailable";return result;}
    if(sqlite3_step(existing)==SQLITE_ROW){result.accepted=text(existing,0)==identity;result.replayed=result.accepted;result.reasonCode=result.accepted?"command_result_replayed":"command_result_conflict";sqlite3_finalize(existing);if(result.accepted){sqlite3_stmt* replay=nullptr;const char* replaySql="UPDATE backend_agent_commands SET result_replay_count=result_replay_count+1 WHERE command_id=?;";if(sqlite3_prepare_v2(database_.handle(),replaySql,-1,&replay,nullptr)!=SQLITE_OK||!bindText(replay,1,r.commandId)||!done(replay)){result.accepted=false;result.replayed=false;result.reasonCode="command_database_unavailable";}}return result;} sqlite3_finalize(existing);
    if(!database_.execute("BEGIN IMMEDIATE;")){result.reasonCode="command_database_unavailable";return result;}
    sqlite3_stmt* insert=nullptr;
    const char* insertSql="INSERT INTO backend_agent_command_results(command_id,result_identity,dispatch_state,verification_state,result_category,error_category,retry_classification,bounded_diagnostics,completed_at) VALUES(?,?,?,?,?,?,?,?,?);";
    bool ok=sqlite3_prepare_v2(database_.handle(),insertSql,-1,&insert,nullptr)==SQLITE_OK&&bindText(insert,1,r.commandId)&&bindText(insert,2,identity)&&bindText(insert,3,r.dispatchState)&&bindText(insert,4,r.verificationState)&&bindText(insert,5,r.resultCategory)&&bindText(insert,6,r.errorCategory)&&bindText(insert,7,r.retryClassification)&&bindText(insert,8,r.boundedDiagnostics)&&bindInt(insert,9,r.completedAt)&&done(insert);
    sqlite3_stmt* update=nullptr;
    const char* updateSql="UPDATE backend_agent_commands SET state=?,updated_at=? WHERE command_id=?;";
    const std::string state=r.resultCategory=="outcome_unknown"?"waiting_reconciliation":"completed";
    ok=ok&&sqlite3_prepare_v2(database_.handle(),updateSql,-1,&update,nullptr)==SQLITE_OK&&bindText(update,1,state)&&bindInt(update,2,r.completedAt)&&bindText(update,3,r.commandId)&&done(update);
    if(!ok||!database_.execute("COMMIT;")){database_.execute("ROLLBACK;");result.reasonCode="command_database_unavailable";return result;}
    result.accepted=true;result.reasonCode="command_result_accepted";return result;
}

bool BackendAgentCommandRepository::requestReplay(const std::string& backendId,const std::string& commandId)
{
    sqlite3_stmt* s=nullptr; const char* sql="UPDATE backend_agent_commands SET replay_requested=1 WHERE backend_id=? AND command_id=?;";
    return sqlite3_prepare_v2(database_.handle(),sql,-1,&s,nullptr)==SQLITE_OK&&bindText(s,1,backendId)&&bindText(s,2,commandId)&&done(s)&&sqlite3_changes(database_.handle())==1;
}
bool BackendAgentCommandRepository::armFault(const std::string& backendId,const std::string& kind)
{
    if(kind!="receipt"&&kind!="result")return false;
    const std::string column=kind=="receipt"?"drop_next_receipt":"drop_next_result";
    const std::string sql="INSERT INTO backend_agent_command_faults(backend_id,"+column+") VALUES(?,1) ON CONFLICT(backend_id) DO UPDATE SET "+column+"=1;";
    sqlite3_stmt* s=nullptr; return sqlite3_prepare_v2(database_.handle(),sql.c_str(),-1,&s,nullptr)==SQLITE_OK&&bindText(s,1,backendId)&&done(s);
}
bool BackendAgentCommandRepository::consumeFault(const std::string& backendId,const std::string& kind)
{
    auto transactionLease = database_.acquireTransactionLease();
    if (kind != "receipt" && kind != "result") return false;
    const std::string column = kind == "receipt"
        ? "drop_next_receipt"
        : "drop_next_result";
    if(!database_.execute("BEGIN IMMEDIATE;"))return false;
    sqlite3_stmt* q=nullptr; const std::string select="SELECT "+column+" FROM backend_agent_command_faults WHERE backend_id=?;";
    bool armed=false,ok=sqlite3_prepare_v2(database_.handle(),select.c_str(),-1,&q,nullptr)==SQLITE_OK&&bindText(q,1,backendId);
    if (ok && sqlite3_step(q) == SQLITE_ROW)
        armed = sqlite3_column_int(q, 0) != 0;
    if (q != nullptr) sqlite3_finalize(q);
    if(ok&&armed){sqlite3_stmt* u=nullptr;const std::string update="UPDATE backend_agent_command_faults SET "+column+"=0 WHERE backend_id=?;";ok=sqlite3_prepare_v2(database_.handle(),update.c_str(),-1,&u,nullptr)==SQLITE_OK&&bindText(u,1,backendId)&&done(u);}
    if(!ok||!database_.execute("COMMIT;")){database_.execute("ROLLBACK;");return false;} return armed;
}
BackendAgentCommandSummary BackendAgentCommandRepository::summaryForBackend(const std::string& backendId) const
{
    BackendAgentCommandSummary summary; sqlite3_stmt* s=nullptr;
    const char* sql="SELECT c.command_id,c.command_type,c.state,COALESCE(r.receipt_category,''),COALESCE(x.result_category,''),COALESCE(x.dispatch_state,''),COALESCE(x.verification_state,''),c.backend_generation,c.claim_epoch,c.delivery_count,c.receipt_replay_count,c.result_replay_count,c.deadline FROM backend_agent_commands c LEFT JOIN backend_agent_command_receipts r ON r.command_id=c.command_id LEFT JOIN backend_agent_command_results x ON x.command_id=c.command_id WHERE c.backend_id=? ORDER BY c.assigned_at DESC LIMIT 1;";
    if(sqlite3_prepare_v2(database_.handle(),sql,-1,&s,nullptr)!=SQLITE_OK||!bindText(s,1,backendId)){if(s)sqlite3_finalize(s);return summary;}
    if(sqlite3_step(s)==SQLITE_ROW){summary.present=true;summary.commandId=text(s,0);summary.commandType=text(s,1);summary.state=text(s,2);summary.receiptCategory=text(s,3);summary.resultCategory=text(s,4);summary.dispatchState=text(s,5);summary.verificationState=text(s,6);summary.backendGeneration=static_cast<std::uint64_t>(sqlite3_column_int64(s,7));summary.claimEpoch=static_cast<std::uint64_t>(sqlite3_column_int64(s,8));summary.deliveryCount=static_cast<std::uint64_t>(sqlite3_column_int64(s,9));summary.receiptReplayCount=static_cast<std::uint64_t>(sqlite3_column_int64(s,10));summary.resultReplayCount=static_cast<std::uint64_t>(sqlite3_column_int64(s,11));summary.deadline=sqlite3_column_int64(s,12);} sqlite3_finalize(s);return summary;
}

BackendAgentCommandDeliveryService::BackendAgentCommandDeliveryService(BackendAgentCommandRepository& c,BackendAgentRepository& a,AccountabilityEventRepository& e):commandRepository_(c),agentRepository_(a),accountabilityRepository_(e){}

bool BackendAgentCommandDeliveryService::appendEvent(const RequestSecurityContext& context,const std::string& eventType,const std::string& backendId,const std::string& operationId,const std::string& action,const std::string& decision,const std::string& reason,const std::string& outcome,std::int64_t now) const
{
    AccountabilityEvent event; event.eventId=backendAgentGenerateOpaqueId("evt_",12); event.classes="accountability,backend-agent-command";event.eventType=eventType;event.severity=decision=="allow"?"info":"warning";event.occurredAt=std::to_string(now);event.actorId=context.actor.actorId;event.actorType=actorTypeName(context.actor.type);event.deviceId=context.device.has_value()?context.device->deviceId:"";event.authenticationState=authenticationStateName(context.authenticationState);event.permission="backend.agent.command.deliver";event.backendId=backendId;event.operationId=operationId;event.requestId=context.requestId;event.correlationId=context.correlationId;event.action=action;event.decision=decision;event.reasonCode=reason;event.outcome=outcome;return !event.eventId.empty()&&accountabilityRepository_.append(event);
}

bool BackendAgentCommandDeliveryService::agentContextMatches(const RequestSecurityContext& context,const std::string& backendId,const std::string& agentId,const std::string& instance,std::uint64_t generation,bool requireLease,std::int64_t now,std::string& reason) const
{
    if(!context.authenticated()||context.actor.type!=ActorType::Agent){reason="agent_authentication_failed";return false;}
    const auto agent=agentRepository_.findAgentForBackend(backendId);
    if(!agent.has_value()||agent->revoked||agent->incompatible){reason="agent_binding_unavailable";return false;}
    if(agent->agentId!=agentId||agent->actorId!=context.actor.actorId||agent->agentInstanceId!=instance||agent->backendGeneration!=generation){reason="command_generation_fenced";return false;}
    if(requireLease&&agent->leaseExpiresAt<now){reason="command_lease_expired";return false;} reason="agent_context_accepted";return true;
}

BackendAgentCommandPollResult BackendAgentCommandDeliveryService::poll(const RequestSecurityContext& context,const BackendAgentCommandPollRequest& request,std::int64_t now)
{
    BackendAgentCommandPollResult result; const auto agent=agentRepository_.findAgentForBackend(request.backendId); std::string reason;
    if(!agent.has_value()||!agentContextMatches(context,request.backendId,agent->agentId,request.agentInstanceId,request.backendGeneration,true,now,reason)){result.reasonCode=reason.empty()?"agent_binding_unavailable":reason;return result;}
    result=commandRepository_.poll(request,agent->agentId,now); return result;
}
BackendAgentCommandReceiptResult BackendAgentCommandDeliveryService::receipt(const RequestSecurityContext& context,const BackendAgentCommandReceipt& receipt,std::int64_t now)
{
    BackendAgentCommandReceiptResult result; std::string reason;
    if(!backendAgentCommandValidReceipt(receipt)||!agentContextMatches(context,receipt.backendId,receipt.agentId,receipt.agentInstanceId,receipt.backendGeneration,false,now,reason)){result.reasonCode=reason.empty()?"invalid_command_receipt":reason;return result;}
    if(!appendEvent(context,"agent.command.receipt",receipt.backendId,"", "receive-command-receipt","allow","command_receipt_persist","attempted",now)){result.reasonCode="command_accountability_unavailable";return result;}
    result=commandRepository_.acceptReceipt(receipt); if(result.accepted)result.dropResponse=commandRepository_.consumeFault(receipt.backendId,"receipt"); return result;
}
BackendAgentCommandResultAck BackendAgentCommandDeliveryService::result(const RequestSecurityContext& context,const BackendAgentCommandResult& value,std::int64_t now)
{
    BackendAgentCommandResultAck result; std::string reason;
    if(!backendAgentCommandValidResult(value)||!agentContextMatches(context,value.backendId,value.agentId,value.agentInstanceId,value.backendGeneration,false,now,reason)){result.reasonCode=reason.empty()?"invalid_command_result":reason;return result;}
    if(!appendEvent(context,"agent.command.result",value.backendId,"", "receive-command-result","allow","command_result_persist","attempted",now)){result.reasonCode="command_accountability_unavailable";return result;}
    result=commandRepository_.acceptResult(value); if(result.accepted)result.dropResponse=commandRepository_.consumeFault(value.backendId,"result"); return result;
}

std::optional<BackendAgentCommandAssignment> BackendAgentCommandDeliveryService::assignProbe(const RequestSecurityContext& context,const std::string& backendId,std::int64_t now,std::int64_t deadline,std::string& reason)
{
    if(!context.authenticated()||context.actor.type!=ActorType::System||!backendAgentCommandSafeIdentifier(backendId)||deadline<=now||deadline-now>3600){reason="invalid_command_assignment_request";return std::nullopt;}
    const auto agent=agentRepository_.findAgentForBackend(backendId); if(!agent.has_value()||agent->revoked||agent->incompatible||agent->agentInstanceId.empty()||agent->backendGeneration==0||agent->leaseExpiresAt<now){reason="active_agent_lease_required";return std::nullopt;}
    if(!commandRepository_.hasCapability(backendId,agent->agentId,agent->agentInstanceId,agent->backendGeneration,"probe.noop")){reason="command_capability_required";return std::nullopt;}
    BackendAgentCommandAssignment a; a.present=true;a.requestId=backendAgentGenerateOpaqueId("req_",8);a.correlationId=a.requestId;a.operationId=backendAgentGenerateOpaqueId("op_",12);a.jobId=backendAgentGenerateOpaqueId("job_",12);a.attemptId=backendAgentGenerateOpaqueId("att_",12);a.claimEpoch=1;a.commandId=backendAgentGenerateOpaqueId("cmd_",12);a.backendId=backendId;a.agentId=agent->agentId;a.agentInstanceId=agent->agentInstanceId;a.backendGeneration=agent->backendGeneration;a.commandType="probe.noop";a.payloadVersion=1;a.payload="{}";a.verificationPolicy="none";a.assignedAt=now;a.deadline=deadline;a.requestFingerprint=backendAgentCommandFingerprint(a);
    if(!backendAgentCommandValidAssignment(a)||!appendEvent(context,"agent.command.assigned",backendId,a.operationId,"assign-probe-command","allow","non_mutating_probe","attempted",now)||!commandRepository_.insertAssignment(a)){reason="command_assignment_persist_failed";return std::nullopt;} reason="command_probe_assigned";return a;
}
bool BackendAgentCommandDeliveryService::requestReplay(const RequestSecurityContext& context,const std::string& backendId,const std::string& commandId,std::int64_t now,std::string& reason)
{
    if(!context.authenticated()||context.actor.type!=ActorType::System||!backendAgentCommandSafeIdentifier(backendId)||!backendAgentCommandSafeIdentifier(commandId)){reason="invalid_command_replay_request";return false;}
    if(!appendEvent(context,"agent.command.replay.requested",backendId,"","replay-command","allow","operator_fixture","attempted",now)||!commandRepository_.requestReplay(backendId,commandId)){reason="command_replay_failed";return false;} reason="command_replay_requested";return true;
}
bool BackendAgentCommandDeliveryService::armFault(const RequestSecurityContext& context,const std::string& backendId,const std::string& kind,std::int64_t now,std::string& reason)
{
    if(!context.authenticated()||context.actor.type!=ActorType::System||!backendAgentCommandSafeIdentifier(backendId)||(kind!="receipt"&&kind!="result")){reason="invalid_command_fault_request";return false;}
    if(!appendEvent(context,"agent.command.acceptance-fault.armed",backendId,"","arm-lost-response","allow",kind,"attempted",now)||!commandRepository_.armFault(backendId,kind)){reason="command_fault_arm_failed";return false;} reason="command_fault_armed";return true;
}
BackendAgentCommandSummary BackendAgentCommandDeliveryService::summaryForBackend(const std::string& backendId) const{return commandRepository_.summaryForBackend(backendId);}

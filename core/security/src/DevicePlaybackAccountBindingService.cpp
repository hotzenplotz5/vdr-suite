#include "DevicePlaybackAccountBindingService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "AuthorizationService.h"
#include "Database.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <sqlite3.h>
#include <sys/random.h>
#include <algorithm>
#include <cerrno>
#include <array>
#include <cctype>
#include <ctime>
#include <limits>
#include <string>

namespace {
bool safeIdentity(const std::string& value, bool allowEmpty = false) {
    if (value.empty()) return allowEmpty;
    return value.size() <= 128U && std::all_of(
        value.begin(),value.end(),[](unsigned char c) {
            return std::isalnum(c) || c == '-' || c == '_' || c == '.';
        });
}
bool safeBackend(const std::string& value) {
    return safeIdentity(value) && value != "." && value != "..";
}
bool bind(sqlite3_stmt* statement,int index,const std::string& value) {
    return sqlite3_bind_text(statement,index,value.c_str(),-1,SQLITE_TRANSIENT)
        == SQLITE_OK;
}
std::string columnText(sqlite3_stmt* statement,int index) {
    const auto* value = sqlite3_column_text(statement,index);
    return value ? reinterpret_cast<const char*>(value) : std::string();
}
class Transaction {
public:
    explicit Transaction(Database& database) :
        database_(database), active_(database.execute("BEGIN IMMEDIATE;")) {}
    ~Transaction() { if(active_) database_.execute("ROLLBACK;"); }
    bool active() const { return active_; }
    bool commit() {
        if (!active_ || !database_.execute("COMMIT;")) return false;
        active_ = false; return true;
    }
private:
    Database& database_;
    bool active_;
};
std::string eventId() {
    std::array<unsigned char,16> bytes{};
    std::size_t done=0;
    while(done<bytes.size()) {
        const ssize_t n=getrandom(bytes.data()+done,bytes.size()-done,0);
        if(n<0 && errno==EINTR) continue;
        if(n<=0) return {};
        done+=static_cast<std::size_t>(n);
    }
    const char* hex="0123456789abcdef";
    std::string id="ace_";
    for(auto b:bytes) {id+=hex[b>>4U]; id+=hex[b&15U];}
    return id;
}
std::string timestamp() {
    const std::time_t now=std::time(nullptr);
    std::tm utc{};
    char buf[32]{};
    if(gmtime_r(&now,&utc)==nullptr ||
       std::strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",&utc)==0) return {};
    return buf;
}
} // namespace

DevicePlaybackAccountBindingService::DevicePlaybackAccountBindingService(
    Database& database, SecurityIdentityRepository& identities,
    HumanAccountRepository& accounts, SecurityPermissionGrantRepository& grants,
    AccountabilityEventRepository& audit)
    : database_(database),identities_(identities),accounts_(accounts),
      grants_(grants),audit_(audit) {}

bool DevicePlaybackAccountBindingService::ensureSchema() {
    auto lease=database_.acquireTransactionLease();
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS security_device_playback_accounts ("
        "device_id TEXT NOT NULL PRIMARY KEY,"
        "account_id TEXT NOT NULL DEFAULT '',"
        "revision INTEGER NOT NULL DEFAULT 1 CHECK(revision >= 1),"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");");
}

DevicePlaybackAccountResult
DevicePlaybackAccountBindingService::readInActiveTransaction(
    const std::string& deviceId) const {
    DevicePlaybackAccountResult result;
    if (!safeIdentity(deviceId)) {
        result.status=DevicePlaybackAccountStatus::invalid;
        return result;
    }
    const auto device=identities_.findDevice(deviceId);
    if(!device) {
        result.status=DevicePlaybackAccountStatus::notFound;
        return result;
    }
    const auto actor=identities_.findActor(device->actorId);
    if(!actor || !actor->active || actor->revoked ||
       actor->type!=ActorType::Service || !device->active || device->revoked) {
        result.status=DevicePlaybackAccountStatus::notFound;
        return result;
    }
    sqlite3_stmt* statement=nullptr;
    const char* sql=
        "SELECT account_id,revision FROM security_device_playback_accounts "
        "WHERE device_id=?1;";
    if(sqlite3_prepare_v2(database_.handle(),sql,-1,&statement,nullptr)
       !=SQLITE_OK) return result;
    if(!bind(statement,1,deviceId)) {
        sqlite3_finalize(statement);
        return result;
    }
    const int step=sqlite3_step(statement);
    result.binding.deviceId=deviceId;
    if(step==SQLITE_ROW) {
        result.binding.accountId=columnText(statement,0);
        result.binding.revision=sqlite3_column_int64(statement,1);
        if(result.binding.revision>0 && safeIdentity(result.binding.accountId,true))
            result.status=DevicePlaybackAccountStatus::ok;
    } else if(step==SQLITE_DONE) {
        result.status=DevicePlaybackAccountStatus::ok;
    }
    sqlite3_finalize(statement);
    return result;
}
DevicePlaybackAccountResult
DevicePlaybackAccountBindingService::read(const std::string& deviceId) const {
    auto lease=database_.acquireTransactionLease();
    return readInActiveTransaction(deviceId);
}

DevicePlaybackAccountResult DevicePlaybackAccountBindingService::set(
    const std::string& administratorActorId,
    const std::string& deviceId,const std::string& accountId,
    std::int64_t expectedRevision,const std::string& requestId) {
    DevicePlaybackAccountResult result;
    if(!safeIdentity(administratorActorId) || !safeIdentity(deviceId) ||
       !safeIdentity(accountId,true) || !safeIdentity(requestId) ||
       expectedRevision<0 ||
       expectedRevision==std::numeric_limits<std::int64_t>::max()) {
        result.status=DevicePlaybackAccountStatus::invalid;
        return result;
    }
    const auto id=eventId();
    const auto now=timestamp();
    if(id.empty() || now.empty()) return result;

    auto lease=database_.acquireTransactionLease();
    Transaction transaction(database_);
    if(!transaction.active()) return result;
    const auto administrator=identities_.findActor(administratorActorId);
    if(!administrator || !administrator->active || administrator->revoked ||
       administrator->type!=ActorType::User)
        return result;
    // Re-evaluate the current administrator permission while holding the
    // same SQLite transaction as the association and audit write. A grant
    // revoked after HTTP admission cannot authorize the eventual commit.
    const auto currentAdminGrants=grants_.findActiveGrantsForActor(
        administratorActorId);
    if(!currentAdminGrants.available) return result;
    RequestSecurityContext administratorContext;
    administratorContext.authenticationState=AuthenticationState::Authenticated;
    administratorContext.actor={administratorActorId,ActorType::User,
                                administrator->displayName,true};
    administratorContext.permissionGrantResolution=PermissionGrantResolutionState::Resolved;
    administratorContext.grants=currentAdminGrants.grants;
    if(!AuthorizationService().authorize(administratorContext,
       {"devices.grants.modify","*","devices.playback_account.bind"}).allowed) {
        result.status=DevicePlaybackAccountStatus::invalid;
        return result;
    }
    result=readInActiveTransaction(deviceId);
    if(result.status!=DevicePlaybackAccountStatus::ok)
        return result;
    if(result.binding.revision!=expectedRevision) {
        result.status=DevicePlaybackAccountStatus::revisionConflict;
        return result;
    }
    if(!accountId.empty()) {
        const auto account=accounts_.findByAccountId(accountId);
        if(account.status==HumanAccountRepositoryStatus::notFound ||
           (account.status==HumanAccountRepositoryStatus::ok && !account.account.active)) {
            result.status=DevicePlaybackAccountStatus::notFound;
            return result;
        }
        if(account.status!=HumanAccountRepositoryStatus::ok)
            return result;
    }
    sqlite3_stmt* statement=nullptr;
    const char* insertSql=
        "INSERT INTO security_device_playback_accounts(device_id,account_id,revision)"
        " VALUES(?1,?2,1);";
    const char* updateSql=
        "UPDATE security_device_playback_accounts SET account_id=?2,"
        "revision=revision+1,updated_at=CURRENT_TIMESTAMP "
        "WHERE device_id=?1 AND revision=?3;";
    if(sqlite3_prepare_v2(database_.handle(),
        expectedRevision==0 ? insertSql:updateSql,-1,&statement,nullptr)!=SQLITE_OK)
        return {};
    bool bound=bind(statement,1,deviceId) && bind(statement,2,accountId);
    if(expectedRevision>0)
        bound=bound && sqlite3_bind_int64(statement,3,expectedRevision)==SQLITE_OK;
    const bool changed=bound && sqlite3_step(statement)==SQLITE_DONE &&
        sqlite3_changes(database_.handle())==1;
    sqlite3_finalize(statement);
    if(!changed) return {};

    AccountabilityEvent audit;
    audit.eventId=id;
    audit.classes="audit,security,identity";
    audit.eventType="operation.succeeded";
    audit.severity="info";
    audit.occurredAt=now;
    audit.actorId=administratorActorId;
    audit.actorType="user";
    audit.authenticationState="authenticated";
    audit.deviceId=deviceId;
    audit.permission="devices.grants.modify";
    audit.backendId="*";
    audit.requestId=requestId;
    audit.action="devices.playback_account.bind";
    audit.decision="allow";
    audit.reasonCode=accountId.empty() ?
        "playback_account_unlinked":"playback_account_linked";
    audit.outcome="succeeded";
    if(!audit_.append(audit) || !transaction.commit()) return {};
    result.binding.accountId=accountId;
    result.binding.revision=expectedRevision+1;
    result.status=DevicePlaybackAccountStatus::ok;
    return result;
}

DevicePlaybackActorResolution DevicePlaybackAccountBindingService::resolve(
    const RequestSecurityContext& context,const std::string& backendId) const {
    DevicePlaybackActorResolution result;
    if(!safeBackend(backendId) || !context.authenticated() ||
       context.actor.type!=ActorType::Service ||
       !context.device || !context.credential || context.session ||
       context.permissionGrantResolution!=PermissionGrantResolutionState::Resolved)
        {result.status=DevicePlaybackActorStatus::forbidden;return result;}

    auto lease=database_.acquireTransactionLease();
    const auto storedDevice=identities_.findDevice(context.device->deviceId);
    const auto storedActor=identities_.findActor(context.actor.actorId);
    const auto storedCredential=identities_.findCredential(
        context.credential->credentialId);
    if(!storedDevice || !storedDevice->active || storedDevice->revoked ||
       storedDevice->actorId!=context.actor.actorId ||
       !storedActor || !storedActor->active || storedActor->revoked ||
       storedActor->type!=ActorType::Service ||
       !storedCredential || !storedCredential->active ||
       storedCredential->revoked || storedCredential->expired ||
       storedCredential->actorId!=context.actor.actorId ||
       storedCredential->credentialType!="device-app")
        {result.status=DevicePlaybackActorStatus::forbidden;return result;}

    const auto currentDeviceGrants=grants_.findActiveGrantsForActor(
        context.actor.actorId);
    if(!currentDeviceGrants.available) return result;
    RequestSecurityContext verified=context;
    verified.grants=currentDeviceGrants.grants;
    if(!AuthorizationService().authorize(
        verified,{"media.recording.play",backendId,"media.recording.play"}).allowed)
        {result.status=DevicePlaybackActorStatus::forbidden;return result;}

    const auto link=readInActiveTransaction(context.device->deviceId);
    if(link.status!=DevicePlaybackAccountStatus::ok) return result;
    if(link.binding.accountId.empty()) {
        result.status=DevicePlaybackActorStatus::deviceOnly;
        result.actorId=context.actor.actorId;
        return result;
    }
    const auto account=accounts_.findByAccountId(link.binding.accountId);
    if(account.status==HumanAccountRepositoryStatus::notFound ||
       (account.status==HumanAccountRepositoryStatus::ok &&
        !account.account.active))
        {result.status=DevicePlaybackActorStatus::forbidden;return result;}
    if(account.status!=HumanAccountRepositoryStatus::ok) return result;
    const auto grants=grants_.findActiveGrantsForActor(account.account.actorId);
    if(!grants.available) return result;
    RequestSecurityContext human;
    human.authenticationState=AuthenticationState::Authenticated;
    human.actor.actorId=account.account.actorId;
    human.actor.type=ActorType::User;
    human.actor.active=true;
    human.permissionGrantResolution=PermissionGrantResolutionState::Resolved;
    human.grants=grants.grants;
    if(!AuthorizationService().authorize(
        human,{"media.recording.play",backendId,"media.recording.play"}).allowed)
        {result.status=DevicePlaybackActorStatus::forbidden;return result;}
    result.status=DevicePlaybackActorStatus::linked;
    result.actorId=account.account.actorId;
    result.accountId=link.binding.accountId;
    return result;
}

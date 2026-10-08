#include "DeviceGrantAdministrationService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DeviceCredentialVerifierRepository.h"
#include "HumanAccountGrantAdministrationService.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <sys/random.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <ctime>
#include <string>

namespace
{
bool safeId(const std::string& value)
{
    return !value.empty() && value.size() <= 128U &&
        std::all_of(value.begin(), value.end(),
            [](unsigned char c) {
                return std::isalnum(c) || c == '_' || c == '-';
            });
}

bool safeContextToken(const std::string& value)
{
    return !value.empty() && value.size() <= 128U &&
        std::all_of(value.begin(), value.end(),
            [](unsigned char c) {
                return std::isalnum(c) || c == '_' || c == '-' ||
                    c == '.' || c == ':';
            });
}

bool activeTuple(const std::vector<PermissionGrant>& grants,
                 const std::string& permission,
                 const std::string& backendId)
{
    return std::any_of(grants.begin(), grants.end(),
        [&](const PermissionGrant& grant) {
            return grant.permission == permission &&
                   grant.backendId == backendId;
        });
}

std::string newAuditId()
{
    std::array<unsigned char, 16> bytes{};
    std::size_t done = 0U;
    while (done < bytes.size())
    {
        const ssize_t n = getrandom(bytes.data() + done,
                                    bytes.size() - done, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return {};
        done += static_cast<std::size_t>(n);
    }
    static const char* hex = "0123456789abcdef";
    std::string id = "ace_";
    for (const unsigned char byte : bytes)
    {
        id += hex[(byte >> 4U) & 15U];
        id += hex[byte & 15U];
    }
    return id;
}

std::string nowUtc()
{
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    if (gmtime_r(&now, &utc) == nullptr) return {};
    char buf[32]{};
    if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &utc) == 0)
        return {};
    return buf;
}

class Transaction
{
public:
    Transaction(Database& database, const char* statement)
        : database_(database), active_(database.execute(statement)) {}
    ~Transaction() { if (active_) database_.execute("ROLLBACK;"); }
    bool active() const { return active_; }
    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;")) return false;
        active_ = false;
        return true;
    }
private:
    Database& database_;
    bool active_;
};
}

DeviceGrantAdministrationService::DeviceGrantAdministrationService(
    Database& database,
    SecurityIdentityRepository& identities,
    DeviceCredentialVerifierRepository& verifiers,
    SecurityPermissionGrantRepository& grants,
    AccountabilityEventRepository& audit)
    : database_(database), identities_(identities), verifiers_(verifiers),
      grants_(grants), audit_(audit)
{
}

bool DeviceGrantAdministrationService::supportedGrant(
    const std::string& permission, const std::string& backendId)
{
    // A Device Service Actor cannot receive role.admin, arbitrary
    // administrative rights, or mutation permissions through this surface.
    const bool devicePermission =
        permission == "channels.view" ||
        permission == "timers.view" ||
        permission == "media.live.play" ||
        permission == "media.recording.play";
    return devicePermission &&
        HumanAccountGrantAdministrationService::supportedGrant(
            permission, backendId);
}

DeviceGrantAdministrationResult
DeviceGrantAdministrationService::readInActiveTransaction(
    const std::string& deviceId) const
{
    DeviceGrantAdministrationResult result;
    if (!safeId(deviceId))
    {
        result.status = DeviceGrantAdministrationStatus::invalid;
        return result;
    }
    const auto device = identities_.findDevice(deviceId);
    if (!device.has_value())
    {
        result.status = DeviceGrantAdministrationStatus::notFound;
        return result;
    }
    const auto actor = identities_.findActor(device->actorId);
    if (!actor.has_value() || actor->type != ActorType::Service ||
        !actor->active || actor->revoked ||
        !device->active || device->revoked ||
        !verifiers_.hasDeviceCredentialBinding(deviceId))
    {
        result.status = DeviceGrantAdministrationStatus::notFound;
        return result;
    }

    const auto found = grants_.findActiveGrantsForActor(actor->actorId);
    if (!found.available) return result;

    result.grantSet.deviceId = deviceId;
    result.grantSet.actorId = actor->actorId;
    for (const PermissionGrant& grant : found.grants)
    {
        if (supportedGrant(grant.permission, grant.backendId))
            result.grantSet.grants.push_back(grant);
    }
    std::sort(result.grantSet.grants.begin(), result.grantSet.grants.end(),
        [](const PermissionGrant& a, const PermissionGrant& b) {
            return a.permission < b.permission ||
                (a.permission == b.permission && a.backendId < b.backendId);
        });
    result.grantSet.revision =
        HumanAccountGrantAdministrationService::computeGrantSetRevision(
            result.grantSet.grants);
    if (result.grantSet.revision.empty()) return result;
    result.status = DeviceGrantAdministrationStatus::ok;
    return result;
}

DeviceGrantAdministrationResult
DeviceGrantAdministrationService::read(const std::string& deviceId) const
{
    auto lease = database_.acquireTransactionLease();
    Transaction transaction(database_, "BEGIN;");
    if (!transaction.active()) return {};
    DeviceGrantAdministrationResult result =
        readInActiveTransaction(deviceId);
    if (result.status == DeviceGrantAdministrationStatus::ok &&
        !transaction.commit())
        result.status = DeviceGrantAdministrationStatus::unavailable;
    return result;
}

DeviceGrantAdministrationResult
DeviceGrantAdministrationService::setGrant(
    const DeviceGrantAdministrationContext& context,
    const std::string& deviceId,
    const std::string& expectedRevision,
    const std::string& permission,
    const std::string& backendId,
    bool active)
{
    DeviceGrantAdministrationResult result;
    if (!safeContextToken(context.administratorActorId) ||
        !safeContextToken(context.requestId) ||
        !safeId(deviceId) ||
        expectedRevision.empty() ||
        !supportedGrant(permission, backendId))
    {
        result.status = DeviceGrantAdministrationStatus::invalid;
        return result;
    }
    const std::string auditId = newAuditId();
    const std::string occurredAt = nowUtc();
    if (auditId.empty() || occurredAt.empty()) return result;

    auto lease = database_.acquireTransactionLease();
    Transaction transaction(database_, "BEGIN IMMEDIATE;");
    if (!transaction.active()) return result;
    result = readInActiveTransaction(deviceId);
    if (result.status != DeviceGrantAdministrationStatus::ok)
        return result;

    const bool already = activeTuple(
        result.grantSet.grants, permission, backendId);
    DeviceGrantAdministrationStatus outcome =
        DeviceGrantAdministrationStatus::ok;
    std::string reason;
    if (expectedRevision != result.grantSet.revision && already != active)
    {
        outcome = DeviceGrantAdministrationStatus::revisionConflict;
        reason = "grant_set_revision_conflict";
    }
    else if (already == active)
    {
        reason = active ? "grant_already_active" : "grant_already_absent";
    }
    else
    {
        const bool changed = active
            ? grants_.ensureGrant(result.grantSet.actorId,
                                  permission, backendId)
            : grants_.revokeGrant(result.grantSet.actorId,
                                 permission, backendId);
        if (!changed) return {};
        result = readInActiveTransaction(deviceId);
        if (result.status != DeviceGrantAdministrationStatus::ok ||
            activeTuple(result.grantSet.grants,
                        permission, backendId) != active ||
            result.grantSet.revision == expectedRevision)
            return {};
        reason = active ? "grant_ensured" : "grant_revoked";
    }

    AccountabilityEvent event;
    event.eventId = auditId;
    event.classes = outcome == DeviceGrantAdministrationStatus::ok
        ? "audit,security,identity" : "audit,security";
    event.eventType = outcome == DeviceGrantAdministrationStatus::ok
        ? "operation.succeeded" : "operation.failed";
    event.severity = outcome == DeviceGrantAdministrationStatus::ok
        ? "info" : "warning";
    event.occurredAt = occurredAt;
    event.actorId = context.administratorActorId;
    event.actorType = "user";
    event.authenticationState = "authenticated";
    event.permission = "devices.grants.modify";
    event.backendId = "*";
    event.operationId = "device-grant:" + deviceId + ":" +
                        permission + ":" + backendId;
    event.requestId = context.requestId;
    event.correlationId = context.correlationId;
    event.action = active ? "device.grant.ensure" : "device.grant.revoke";
    event.decision = outcome == DeviceGrantAdministrationStatus::ok
        ? "allow" : "deny";
    event.reasonCode = reason;
    event.outcome = outcome == DeviceGrantAdministrationStatus::ok
        ? "success" : "failed";

    if (!audit_.append(event) || !transaction.commit())
        return {};
    result.status = outcome;
    return result;
}

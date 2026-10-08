#include "DeviceLifecycleAdministrationService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DeviceCredentialVerifierRepository.h"
#include "SecurityIdentityRepository.h"

#include <sys/random.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <ctime>

namespace
{
bool safeId(const std::string& id)
{
    return !id.empty() && id.size() <= 128U &&
        std::all_of(id.begin(), id.end(),
            [](unsigned char c) {
                return std::isalnum(c) || c == '_' || c == '-';
            });
}
bool safeContext(const std::string& id)
{
    return !id.empty() && id.size() <= 128U &&
        std::all_of(id.begin(), id.end(),
            [](unsigned char c) {
                return std::isalnum(c) || c == '_' || c == '-' ||
                    c == '.' || c == ':';
            });
}
std::string newAuditId()
{
    std::array<unsigned char, 16> bytes{};
    std::size_t offset = 0U;
    while (offset < bytes.size())
    {
        const ssize_t n = getrandom(bytes.data() + offset,
                                    bytes.size() - offset, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return {};
        offset += static_cast<std::size_t>(n);
    }
    static constexpr char hex[] = "0123456789abcdef";
    std::string id = "ace_";
    for (unsigned char b : bytes)
    {
        id += hex[b >> 4U];
        id += hex[b & 0x0fU];
    }
    return id;
}
std::string occurredAt()
{
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    if (!gmtime_r(&now, &utc)) return {};
    char buffer[32]{};
    if (!std::strftime(
            buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &utc))
        return {};
    return buffer;
}
class Transaction
{
public:
    Transaction(Database& db, const char* begin)
        : db_(db), active_(db_.execute(begin)) {}
    ~Transaction() { if (active_) db_.execute("ROLLBACK;"); }
    bool active() const { return active_; }
    bool commit()
    {
        if (!active_ || !db_.execute("COMMIT;")) return false;
        active_ = false;
        return true;
    }
private:
    Database& db_;
    bool active_;
};
}

DeviceLifecycleAdministrationService::DeviceLifecycleAdministrationService(
    Database& database,
    SecurityIdentityRepository& identities,
    DeviceCredentialVerifierRepository& verifiers,
    AccountabilityEventRepository& accountability)
    : database_(database), identities_(identities),
      verifiers_(verifiers), accountability_(accountability)
{
}

DeviceLifecycleResult
DeviceLifecycleAdministrationService::readInTransaction(
    DeviceLifecycleTarget target, const std::string& deviceId,
    const std::string& credentialId) const
{
    DeviceLifecycleResult result;
    if (!safeId(deviceId) ||
        (target == DeviceLifecycleTarget::Device
            ? !credentialId.empty() : !safeId(credentialId)))
    {
        result.status = DeviceLifecycleStatus::invalid;
        return result;
    }

    const auto device = identities_.findDevice(deviceId);
    if (!device.has_value())
    {
        result.status = DeviceLifecycleStatus::notFound;
        return result;
    }
    const auto actor = identities_.findActor(device->actorId);
    if (!actor.has_value() || actor->type != ActorType::Service ||
        !verifiers_.hasDeviceCredentialBinding(deviceId))
    {
        result.status = DeviceLifecycleStatus::notFound;
        return result;
    }

    result.resource.deviceId = deviceId;
    result.resource.actorId = actor->actorId;
    if (target == DeviceLifecycleTarget::Credential)
    {
        const auto verifier = verifiers_.findByCredentialId(credentialId);
        const auto credential = identities_.findCredential(credentialId);
        if (!verifier.has_value() || !credential.has_value() ||
            verifier->deviceId != deviceId ||
            credential->actorId != actor->actorId ||
            credential->credentialType != "device-app")
        {
            result.status = DeviceLifecycleStatus::notFound;
            return result;
        }
        result.resource.credentialId = credentialId;
        result.resource.revoked = credential->revoked || !credential->active;
        result.resource.active = credential->active && !credential->revoked;
        result.resource.resourceRevision =
            "device-credential-lifecycle:" + credentialId +
            (result.resource.revoked ? ":revoked" : ":active");
    }
    else
    {
        result.resource.revoked = device->revoked || !device->active;
        result.resource.active = device->active && !device->revoked;
        result.resource.resourceRevision =
            "device-lifecycle:" + deviceId +
            (result.resource.revoked ? ":revoked" : ":active");
    }
    result.status = DeviceLifecycleStatus::ok;
    return result;
}

DeviceLifecycleResult DeviceLifecycleAdministrationService::read(
    DeviceLifecycleTarget target, const std::string& deviceId,
    const std::string& credentialId) const
{
    auto lease = database_.acquireTransactionLease();
    Transaction transaction(database_, "BEGIN;");
    if (!transaction.active()) return {};
    auto result = readInTransaction(target, deviceId, credentialId);
    if (result.status == DeviceLifecycleStatus::ok &&
        !transaction.commit())
        result.status = DeviceLifecycleStatus::unavailable;
    return result;
}

DeviceLifecycleResult DeviceLifecycleAdministrationService::revoke(
    const DeviceLifecycleContext& context,
    DeviceLifecycleTarget target, const std::string& deviceId,
    const std::string& credentialId,
    const std::string& expectedRevision)
{
    DeviceLifecycleResult result;
    if (!safeContext(context.administratorActorId) ||
        !safeContext(context.requestId) ||
        !safeId(deviceId) || expectedRevision.empty() ||
        (target == DeviceLifecycleTarget::Device
            ? !credentialId.empty() : !safeId(credentialId)))
    {
        result.status = DeviceLifecycleStatus::invalid;
        return result;
    }
    const std::string auditId = newAuditId();
    const std::string timestamp = occurredAt();
    if (auditId.empty() || timestamp.empty()) return result;

    auto lease = database_.acquireTransactionLease();
    Transaction transaction(database_, "BEGIN IMMEDIATE;");
    if (!transaction.active()) return result;

    result = readInTransaction(target, deviceId, credentialId);
    if (result.status != DeviceLifecycleStatus::ok)
        return result;

    DeviceLifecycleStatus status = DeviceLifecycleStatus::ok;
    std::string reason;
    if (result.resource.active &&
        result.resource.resourceRevision != expectedRevision)
    {
        status = DeviceLifecycleStatus::revisionConflict;
        reason = "device_lifecycle_revision_conflict";
    }
    else if (result.resource.revoked)
        reason = "already_revoked";
    else
    {
        const bool revoked = target == DeviceLifecycleTarget::Device
            ? identities_.revokeDevice(deviceId)
            : identities_.revokeCredential(credentialId);
        if (!revoked) return {};
        const auto now = readInTransaction(target, deviceId, credentialId);
        if (now.status != DeviceLifecycleStatus::ok ||
            !now.resource.revoked ||
            now.resource.resourceRevision == result.resource.resourceRevision)
            return {};
        result = now;
        reason = target == DeviceLifecycleTarget::Device
            ? "device_revoked" : "device_credential_revoked";
    }

    AccountabilityEvent event;
    event.eventId = auditId;
    event.classes = status == DeviceLifecycleStatus::ok
        ? "audit,security,identity" : "audit,security";
    event.eventType = status == DeviceLifecycleStatus::ok
        ? "operation.succeeded" : "operation.failed";
    event.severity = status == DeviceLifecycleStatus::ok ? "info" : "warning";
    event.occurredAt = timestamp;
    event.actorId = context.administratorActorId;
    event.actorType = "user";
    event.deviceId = deviceId;
    event.authenticationState = "authenticated";
    event.permission = target == DeviceLifecycleTarget::Device
        ? "devices.revoke" : "devices.credentials.revoke";
    event.backendId = "*";
    event.operationId = "device-lifecycle:" + deviceId +
        (credentialId.empty() ? "" : ":" + credentialId);
    event.requestId = context.requestId;
    event.correlationId = context.correlationId;
    event.action = target == DeviceLifecycleTarget::Device
        ? "device.revoke" : "device.credential.revoke";
    event.decision = status == DeviceLifecycleStatus::ok ? "allow" : "deny";
    event.reasonCode = reason;
    event.outcome = status == DeviceLifecycleStatus::ok
        ? "success" : "failed";

    if (!accountability_.append(event) || !transaction.commit()) return {};
    result.status = status;
    return result;
}

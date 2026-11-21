#include "DeviceCredentialRotationService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DeviceCredentialVerifierRepository.h"
#include "SecurityIdentityRepository.h"

#include <crypt.h>
#include <sys/random.h>
#include <array>
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <ctime>
#include <utility>

namespace {
void wipe(std::string& secret) noexcept {
    volatile char* p = secret.empty() ? nullptr : &secret[0];
    for (std::size_t i = 0; i < secret.size(); ++i) p[i] = 0;
    secret.clear();
}
bool safeId(const std::string& id) {
    return !id.empty() && id.size() <= 128 &&
        std::all_of(id.begin(), id.end(), [](unsigned char c) {
            return std::isalnum(c) || c == '_' || c == '-';
        });
}
bool safeContext(const std::string& id) {
    return !id.empty() && id.size() <= 128 &&
        std::all_of(id.begin(), id.end(), [](unsigned char c) {
            return std::isalnum(c) || c == '_' || c == '-' || c == '.' || c == ':';
        });
}
bool entropy(unsigned char* p, std::size_t n) {
    std::size_t i = 0;
    while (i < n) {
        const ssize_t got = getrandom(p+i, n-i, 0);
        if (got < 0 && errno == EINTR) continue;
        if (got <= 0) return false;
        i += static_cast<std::size_t>(got);
    }
    return true;
}
template<std::size_t N>
bool randomBytes(std::array<unsigned char,N>& b) {
    if (!entropy(b.data(), b.size())) {
        b.fill(0);
        return false;
    }
    return true;
}
template<std::size_t N>
std::string hex(const std::array<unsigned char,N>& b) {
    const char* alphabet = "0123456789abcdef";
    std::string result;
    result.reserve(b.size()*2);
    for (unsigned char c : b) {
        result.push_back(alphabet[c >> 4]);
        result.push_back(alphabet[c & 15]);
    }
    return result;
}
std::string timestamp() {
    std::time_t now = std::time(nullptr);
    std::tm utc{};
    if (!gmtime_r(&now, &utc)) return {};
    char buffer[32]{};
    if (!std::strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",&utc)) return {};
    return buffer;
}
class Transaction {
public:
    explicit Transaction(Database& db)
        : db_(db), active_(db.execute("BEGIN IMMEDIATE;")) {}
    ~Transaction() { if (active_) db_.execute("ROLLBACK;"); }
    bool active() const { return active_; }
    bool commit() {
        if (!active_ || !db_.execute("COMMIT;")) return false;
        active_ = false;
        return true;
    }
private:
    Database& db_;
    bool active_;
};
}

RotatedDeviceCredential::~RotatedDeviceCredential() { clearSecret(); }
RotatedDeviceCredential::RotatedDeviceCredential(RotatedDeviceCredential&& other) noexcept
    : deviceId(std::move(other.deviceId)),
      actorId(std::move(other.actorId)),
      credentialId(std::move(other.credentialId)),
      credentialSecret(std::move(other.credentialSecret)) {
    other.clearSecret();
}
RotatedDeviceCredential& RotatedDeviceCredential::operator=(RotatedDeviceCredential&& other) noexcept {
    if (this != &other) {
        clearSecret();
        deviceId = std::move(other.deviceId);
        actorId = std::move(other.actorId);
        credentialId = std::move(other.credentialId);
        credentialSecret = std::move(other.credentialSecret);
        other.clearSecret();
    }
    return *this;
}
void RotatedDeviceCredential::clearSecret() noexcept { wipe(credentialSecret); }

DeviceCredentialRotationService::DeviceCredentialRotationService(
    Database& db, SecurityIdentityRepository& ids,
    DeviceCredentialVerifierRepository& verifiers,
    AccountabilityEventRepository& audit)
    : database_(db), identities_(ids), verifiers_(verifiers), accountability_(audit) {}

DeviceCredentialRotationResult DeviceCredentialRotationService::rotate(
    const DeviceCredentialRotationRequest& request) {
    DeviceCredentialRotationResult result;
    if (!safeContext(request.administratorActorId) ||
        !safeContext(request.requestId) ||
        !safeId(request.deviceId) ||
        !safeId(request.previousCredentialId) ||
        request.correlationId.size() > 128 ||
        request.expectedResourceRevision !=
            "device-credential-lifecycle:" + request.previousCredentialId + ":active") {
        result.status = DeviceCredentialRotationStatus::invalid;
        return result;
    }

    // Prepare one-shot secret and verifier before acquiring writer lock.
    std::array<unsigned char,16> idBytes{}, saltBytes{}, auditBytes{};
    std::array<unsigned char,32> secretBytes{};
    if (!randomBytes(idBytes) || !randomBytes(saltBytes) ||
        !randomBytes(auditBytes) || !randomBytes(secretBytes)) return result;

    RotatedDeviceCredential issued;
    issued.credentialId = "credential_device_" + hex(idBytes);
    issued.credentialSecret = hex(secretBytes);
    idBytes.fill(0); secretBytes.fill(0);

    static constexpr char alphabet[] =
        "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string salt;
    salt.reserve(16);
    for (unsigned char c : saltBytes) salt.push_back(alphabet[c & 63]);
    saltBytes.fill(0);
    const std::string setting = "$6$rounds=10000$" + salt + "$";
    wipe(salt);
    crypt_data crypto{};
    const char* encoded = crypt_r(issued.credentialSecret.c_str(),setting.c_str(),&crypto);
    std::string verifier = encoded && encoded[0] != '*' ? encoded : "";
    volatile unsigned char* bytes = reinterpret_cast<volatile unsigned char*>(&crypto);
    for (std::size_t i=0; i<sizeof(crypto); ++i) bytes[i]=0;
    if (verifier.empty()) return result;

    const std::string occurred = timestamp();
    if (occurred.empty()) return result;
    auto lease = database_.acquireTransactionLease();
    Transaction transaction(database_);
    if (!transaction.active()) return result;

    const auto device = identities_.findDevice(request.deviceId);
    const auto previous = identities_.findCredential(request.previousCredentialId);
    const auto binding = verifiers_.findByCredentialId(request.previousCredentialId);
    if (!device || !previous || !binding ||
        binding->deviceId != request.deviceId ||
        previous->actorId != device->actorId ||
        previous->credentialType != "device-app") {
        result.status = DeviceCredentialRotationStatus::notFound;
        return result;
    }
    const auto actor = identities_.findActor(device->actorId);
    if (!actor || actor->type != ActorType::Service) {
        result.status = DeviceCredentialRotationStatus::notFound;
        return result;
    }
    if (!device->active || device->revoked || !actor->active || actor->revoked ||
        previous->expired || !previous->active || previous->revoked) {
        result.status = DeviceCredentialRotationStatus::stateConflict;
        return result;
    }

    // Old credential can win this race only once. Existing actor grants
    // remain attached to the same service actor; no new grants are minted.
    if (!identities_.rotateCredentialInActiveTransaction(
            request.previousCredentialId, issued.credentialId,
            actor->actorId, "device-app") ||
        !verifiers_.insertInActiveTransaction(
            issued.credentialId, request.deviceId, verifier)) return result;

    const auto replaced = identities_.findCredential(issued.credentialId);
    const auto old = identities_.findCredential(request.previousCredentialId);
    if (!replaced || !old || !old->revoked ||
        replaced->rotatedFromCredentialId != request.previousCredentialId ||
        !replaced->active || replaced->revoked) return result;

    AccountabilityEvent event;
    event.eventId = "ace_" + hex(auditBytes);
    event.classes = "audit,security,identity";
    event.eventType = "operation.succeeded";
    event.severity = "info";
    event.occurredAt = occurred;
    event.actorId = request.administratorActorId;
    event.actorType = "user";
    event.deviceId = request.deviceId;
    event.authenticationState = "authenticated";
    event.permission = "devices.credentials.rotate";
    event.backendId = "*";
    event.operationId = "device-credential-rotation:" + request.deviceId;
    event.requestId = request.requestId;
    event.correlationId = request.correlationId;
    event.action = "device.credential.rotate";
    event.decision = "allow";
    event.reasonCode = "device_credential_rotated";
    event.outcome = "success";
    if (!accountability_.append(event) || !transaction.commit()) return result;

    issued.deviceId = request.deviceId;
    issued.actorId = actor->actorId;
    result.issued.emplace(std::move(issued));
    result.status = DeviceCredentialRotationStatus::rotated;
    return result;
}

#pragma once

#include <optional>
#include <string>

class Database;
class SecurityIdentityRepository;
class DeviceCredentialVerifierRepository;
class AccountabilityEventRepository;

enum class DeviceCredentialRotationStatus {
    rotated, invalid, notFound, stateConflict, revisionConflict, unavailable
};

struct RotatedDeviceCredential {
    std::string deviceId;
    std::string actorId;
    std::string credentialId;
    std::string credentialSecret;

    RotatedDeviceCredential() = default;
    ~RotatedDeviceCredential();
    RotatedDeviceCredential(const RotatedDeviceCredential&) = delete;
    RotatedDeviceCredential& operator=(const RotatedDeviceCredential&) = delete;
    RotatedDeviceCredential(RotatedDeviceCredential&&) noexcept;
    RotatedDeviceCredential& operator=(RotatedDeviceCredential&&) noexcept;
    void clearSecret() noexcept;
};

struct DeviceCredentialRotationResult {
    DeviceCredentialRotationStatus status = DeviceCredentialRotationStatus::unavailable;
    std::optional<RotatedDeviceCredential> issued;
};

struct DeviceCredentialRotationRequest {
    std::string administratorActorId;
    std::string deviceId;
    std::string previousCredentialId;
    std::string expectedResourceRevision;
    std::string requestId;
    std::string correlationId;
};

// Caller MUST enforce administrator permission and browser CSRF at HTTP gate.
// One-shot result is never persisted or replayed. Lost response requires a
// subsequent explicit rotation of the currently active credential.
class DeviceCredentialRotationService {
public:
    DeviceCredentialRotationService(Database& database,
        SecurityIdentityRepository& identities,
        DeviceCredentialVerifierRepository& verifiers,
        AccountabilityEventRepository& accountability);

    DeviceCredentialRotationResult rotate(const DeviceCredentialRotationRequest& request);

private:
    Database& database_;
    SecurityIdentityRepository& identities_;
    DeviceCredentialVerifierRepository& verifiers_;
    AccountabilityEventRepository& accountability_;
};

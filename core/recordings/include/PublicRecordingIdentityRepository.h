#pragma once

#include <optional>
#include <string>

class Database;

// Suite-owned opaque identity, never the backend's recording pathname or
// mutable list position. A verified move must explicitly rebind the identity.
class PublicRecordingIdentityRepository final
{
public:
    explicit PublicRecordingIdentityRepository(Database& database);

    bool ensureSchema();
    std::optional<std::string> find(
        const std::string& backendId,
        const std::string& backendNativeId) const;

    // Server-internal reverse mapping only. The caller must separately
    // authorize backendId and recheck the live recording snapshot.
    // Never return a native recording identifier to Public-v1 clients.
    std::optional<std::string> findNativeForPublicId(
        const std::string& backendId,
        const std::string& publicRecordingId) const;
    std::optional<std::string> resolveOrCreate(
        const std::string& backendId,
        const std::string& backendNativeId);
    bool removeAfterVerifiedDeletion(
        const std::string& backendId,
        const std::string& backendNativeId);
    bool rebindAfterVerifiedMove(
        const std::string& backendId,
        const std::string& previousNativeId,
        const std::string& nextNativeId);

private:
    Database& database_;
};

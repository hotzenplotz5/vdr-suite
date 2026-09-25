#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <string>

class BackendRuntimeGenerationRepository;
class Database;

struct EmbeddedBackendLifecycleState
{
    bool present = false;
    bool online = false;
    std::uint64_t backendGeneration = 0;
    std::uint64_t heartbeatSequence = 0;
    std::int64_t lastHeartbeatAt = 0;
    std::int64_t leaseExpiresAt = 0;
};

class EmbeddedBackendLifecycleService
{
public:
    explicit EmbeddedBackendLifecycleService(Database& database);

    bool ensureSchema();

    bool startBackend(
        const std::string& backendId,
        std::int64_t now);

    bool heartbeatBackend(
        const std::string& backendId,
        bool healthy,
        std::int64_t now);

    bool maintainBackend(
        const std::string& backendId,
        bool healthy,
        std::int64_t now);

    bool stopBackend(
        const std::string& backendId,
        std::int64_t now);

    EmbeddedBackendLifecycleState statusForBackend(
        const std::string& backendId,
        std::int64_t now) const;

private:
    struct RuntimeIdentity
    {
        std::string instanceId;
        std::uint64_t backendGeneration = 0;
    };

    Database& database_;
    mutable std::mutex mutex_;
    std::map<std::string, RuntimeIdentity> runtimes_;
};

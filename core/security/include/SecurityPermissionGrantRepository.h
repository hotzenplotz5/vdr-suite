#pragma once

#include "SecurityIdentity.h"

#include <string>
#include <vector>

class Database;

struct SecurityPermissionGrantResolution
{
    bool available = false;
    std::vector<PermissionGrant> grants;
};

class SecurityPermissionGrantRepository
{
public:
    explicit SecurityPermissionGrantRepository(Database& database);

    bool ensureSchema();

    SecurityPermissionGrantResolution findActiveGrantsForActor(
        const std::string& actorId) const;

    bool ensureGrant(
        const std::string& actorId,
        const std::string& permission,
        const std::string& backendId);

    bool revokeGrant(
        const std::string& actorId,
        const std::string& permission,
        const std::string& backendId);

private:
    Database& database_;
};

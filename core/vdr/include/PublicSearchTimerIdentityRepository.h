#pragma once

#include <optional>
#include <string>

class Database;

// Durable Suite-owned SearchTimer identity. Native provider IDs never leave
// the server as public resource identities.
class PublicSearchTimerIdentityRepository final
{
public:
    explicit PublicSearchTimerIdentityRepository(Database& database);
    bool ensureSchema();
    std::optional<std::string> resolveOrCreate(
        const std::string& backendId, const std::string& nativeId);

private:
    Database& database_;
};

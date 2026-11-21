#pragma once

#include <optional>
#include <string>

class Database;

struct ActiveMediaRouteLease
{
    std::string sessionId;
    std::string routeId;
    long long routeEpoch = 0;
    std::string backendId;
    std::string providerId;
    std::string leaseId;
    std::string workspaceId;
    std::string presentationProfileId;
};

class MediaRouteLeaseRepository
{
public:
    explicit MediaRouteLeaseRepository(Database& database);

    std::optional<ActiveMediaRouteLease> findActive(
        const std::string& sessionId,
        const std::string& routeId,
        long long routeEpoch) const;

private:
    Database& database_;
};
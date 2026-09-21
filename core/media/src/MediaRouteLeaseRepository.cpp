#include "MediaRouteLeaseRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>

namespace
{

bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    return std::all_of(
        value.begin(), value.end(),
        [](unsigned char character) {
            return std::isalnum(character) || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int index)
{
    const unsigned char* value = sqlite3_column_text(statement, index);
    return value == nullptr
        ? std::string{}
        : std::string(reinterpret_cast<const char*>(value));
}

} // namespace

MediaRouteLeaseRepository::MediaRouteLeaseRepository(Database& database)
    : database_(database)
{
}

std::optional<ActiveMediaRouteLease> MediaRouteLeaseRepository::findActive(
    const std::string& sessionId,
    const std::string& routeId,
    long long routeEpoch) const
{
    if (!safeIdentifier(sessionId) || !safeIdentifier(routeId) || routeEpoch <= 0) {
        return std::nullopt;
    }

    auto transactionLease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT s.session_id, r.route_id, r.route_epoch, r.backend_id, "
        "r.provider_id, l.lease_id, l.workspace_id, s.presentation_profile_id "
        "FROM media_sessions s "
        "JOIN media_routes r ON r.session_id=s.session_id "
        "JOIN media_provider_stream_leases l ON l.route_id=r.route_id "
        "WHERE s.session_id=? AND r.route_id=? AND r.route_epoch=? "
        "AND s.state IN ('ready','active') "
        "AND r.state IN ('ready','active') "
        "AND l.state='active' LIMIT 1;";

    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    const bool bound =
        bindText(statement, 1, sessionId) &&
        bindText(statement, 2, routeId) &&
        sqlite3_bind_int64(statement, 3, routeEpoch) == SQLITE_OK;

    if (!bound || sqlite3_step(statement) != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    ActiveMediaRouteLease result;
    result.sessionId = columnText(statement, 0);
    result.routeId = columnText(statement, 1);
    result.routeEpoch = sqlite3_column_int64(statement, 2);
    result.backendId = columnText(statement, 3);
    result.providerId = columnText(statement, 4);
    result.leaseId = columnText(statement, 5);
    result.workspaceId = columnText(statement, 6);
    result.presentationProfileId = columnText(statement, 7);
    sqlite3_finalize(statement);

    if (!safeIdentifier(result.sessionId) || !safeIdentifier(result.routeId) ||
        result.routeEpoch != routeEpoch || !safeIdentifier(result.backendId) ||
        !safeIdentifier(result.providerId) || !safeIdentifier(result.leaseId) ||
        !safeIdentifier(result.workspaceId) ||
        !safeIdentifier(result.presentationProfileId)) {
        return std::nullopt;
    }

    return result;
}
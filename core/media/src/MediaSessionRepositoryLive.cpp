#include "MediaSessionRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace
{
bool safeIdentifier(const std::string& value)
{
    return !value.empty() && value.size() <= 128 &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}
}

bool MediaSessionRepository::updateProvisioningPresentationProfile(
    const std::string& sessionId,
    const std::string& presentationProfileId)
{
    if (!safeIdentifier(sessionId) || !safeIdentifier(presentationProfileId))
        return false;

    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE media_sessions SET presentation_profile_id=?, updated_at=CURRENT_TIMESTAMP "
        "WHERE session_id=? AND state='provisioning';";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;
    const bool bound =
        sqlite3_bind_text(statement, 1, presentationProfileId.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_text(statement, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
    const int stepped = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    const int changed = sqlite3_changes(database_.handle());
    sqlite3_finalize(statement);
    return stepped == SQLITE_DONE && changed == 1;
}

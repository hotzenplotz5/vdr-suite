#include "BrowserSessionCredentialRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <limits>

namespace
{
constexpr std::size_t MaximumRetentionBatchSize = 256;

bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128)
    {
        return false;
    }

    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return std::isalnum(character) ||
                character == '-' ||
                character == '_' ||
                character == '.' ||
                character == ':';
        });
}

bool safeTimestamp(const std::string& value)
{
    if (value.empty() || value.size() > 64)
    {
        return false;
    }

    return std::none_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return character == '\0' ||
                character == '\r' ||
                character == '\n';
        });
}

bool safeCandidate(const TerminalBrowserSessionCandidate& candidate)
{
    return safeIdentifier(candidate.tokenId) &&
        safeIdentifier(candidate.sessionId) &&
        safeIdentifier(candidate.actorId) &&
        safeIdentifier(candidate.deviceId) &&
        safeIdentifier(candidate.credentialId) &&
        safeTimestamp(candidate.terminalAt);
}

bool bindText(
    sqlite3_stmt* statement,
    int index,
    const std::string& value)
{
    return sqlite3_bind_text(
               statement,
               index,
               value.c_str(),
               -1,
               SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* text = sqlite3_column_text(statement, column);
    return text == nullptr
        ? std::string()
        : std::string(reinterpret_cast<const char*>(text));
}

bool validPolicy(
    int retentionSeconds,
    int idleTimeoutSeconds,
    std::size_t limit)
{
    return retentionSeconds > 0 &&
        idleTimeoutSeconds >= 0 &&
        limit > 0 &&
        limit <= MaximumRetentionBatchSize &&
        limit <= static_cast<std::size_t>(
            std::numeric_limits<int>::max());
}
}

std::optional<std::vector<TerminalBrowserSessionCandidate>>
BrowserSessionCredentialRepository::findTerminalRetentionCandidates(
    int retentionSeconds,
    int idleTimeoutSeconds,
    std::size_t limit) const
{
    if (!validPolicy(retentionSeconds, idleTimeoutSeconds, limit))
    {
        return std::nullopt;
    }

    const char* sql =
        "SELECT token_id, session_id, actor_id, device_id, credential_id, "
        "min("
        "CASE WHEN revoked_at <> '' AND revoked_at <= "
        "datetime(CURRENT_TIMESTAMP, '-' || ?1 || ' seconds') "
        "THEN revoked_at ELSE '9999-12-31 23:59:59' END, "
        "CASE WHEN expires_at <= "
        "datetime(CURRENT_TIMESTAMP, '-' || ?1 || ' seconds') "
        "THEN expires_at ELSE '9999-12-31 23:59:59' END, "
        "CASE WHEN ?2 > 0 AND last_seen_at <= "
        "datetime(CURRENT_TIMESTAMP, '-' || (?1 + ?2) || ' seconds') "
        "THEN datetime(last_seen_at, '+' || ?2 || ' seconds') "
        "ELSE '9999-12-31 23:59:59' END"
        ") AS terminal_at "
        "FROM security_browser_session_credentials "
        "WHERE (revoked_at <> '' AND revoked_at <= "
        "datetime(CURRENT_TIMESTAMP, '-' || ?1 || ' seconds')) "
        "OR expires_at <= "
        "datetime(CURRENT_TIMESTAMP, '-' || ?1 || ' seconds') "
        "OR (?2 > 0 AND last_seen_at <= "
        "datetime(CURRENT_TIMESTAMP, '-' || (?1 + ?2) || ' seconds')) "
        "ORDER BY terminal_at, token_id "
        "LIMIT ?3;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    const bool bound =
        sqlite3_bind_int(statement, 1, retentionSeconds) == SQLITE_OK &&
        sqlite3_bind_int(statement, 2, idleTimeoutSeconds) == SQLITE_OK &&
        sqlite3_bind_int(
            statement,
            3,
            static_cast<int>(limit)) == SQLITE_OK;
    if (!bound)
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::vector<TerminalBrowserSessionCandidate> candidates;
    int step = SQLITE_ROW;
    while ((step = sqlite3_step(statement)) == SQLITE_ROW)
    {
        TerminalBrowserSessionCandidate candidate;
        candidate.tokenId = columnText(statement, 0);
        candidate.sessionId = columnText(statement, 1);
        candidate.actorId = columnText(statement, 2);
        candidate.deviceId = columnText(statement, 3);
        candidate.credentialId = columnText(statement, 4);
        candidate.terminalAt = columnText(statement, 5);
        if (!safeCandidate(candidate))
        {
            sqlite3_finalize(statement);
            return std::nullopt;
        }
        candidates.push_back(std::move(candidate));
    }

    const int finalizeResult = sqlite3_finalize(statement);
    if (step != SQLITE_DONE || finalizeResult != SQLITE_OK)
    {
        return std::nullopt;
    }
    return candidates;
}

std::optional<bool>
BrowserSessionCredentialRepository::remainsTerminalRetentionCandidate(
    const TerminalBrowserSessionCandidate& candidate,
    int retentionSeconds,
    int idleTimeoutSeconds) const
{
    if (!safeCandidate(candidate) ||
        !validPolicy(retentionSeconds, idleTimeoutSeconds, 1))
    {
        return std::nullopt;
    }

    const char* sql =
        "SELECT 1 FROM security_browser_session_credentials "
        "WHERE token_id = ?1 AND session_id = ?2 AND actor_id = ?3 "
        "AND device_id = ?4 AND credential_id = ?5 AND ("
        "(revoked_at <> '' AND revoked_at <= "
        "datetime(CURRENT_TIMESTAMP, '-' || ?6 || ' seconds')) "
        "OR expires_at <= "
        "datetime(CURRENT_TIMESTAMP, '-' || ?6 || ' seconds') "
        "OR (?7 > 0 AND last_seen_at <= "
        "datetime(CURRENT_TIMESTAMP, '-' || (?6 + ?7) || ' seconds'))"
        ") LIMIT 1;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    const bool bound =
        bindText(statement, 1, candidate.tokenId) &&
        bindText(statement, 2, candidate.sessionId) &&
        bindText(statement, 3, candidate.actorId) &&
        bindText(statement, 4, candidate.deviceId) &&
        bindText(statement, 5, candidate.credentialId) &&
        sqlite3_bind_int(statement, 6, retentionSeconds) == SQLITE_OK &&
        sqlite3_bind_int(statement, 7, idleTimeoutSeconds) == SQLITE_OK;
    if (!bound)
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    const int step = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (step == SQLITE_ROW)
    {
        return true;
    }
    if (step == SQLITE_DONE)
    {
        return false;
    }
    return std::nullopt;
}

bool BrowserSessionCredentialRepository::deleteTerminalRetentionCandidate(
    const TerminalBrowserSessionCandidate& candidate)
{
    if (!safeCandidate(candidate))
    {
        return false;
    }

    const char* sql =
        "DELETE FROM security_browser_session_credentials "
        "WHERE token_id = ?1 AND session_id = ?2 AND actor_id = ?3 "
        "AND device_id = ?4 AND credential_id = ?5;";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound =
        bindText(statement, 1, candidate.tokenId) &&
        bindText(statement, 2, candidate.sessionId) &&
        bindText(statement, 3, candidate.actorId) &&
        bindText(statement, 4, candidate.deviceId) &&
        bindText(statement, 5, candidate.credentialId);
    const int step = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    const int changed = sqlite3_changes(database_.handle());
    const int finalizeResult = sqlite3_finalize(statement);
    return step == SQLITE_DONE &&
        finalizeResult == SQLITE_OK &&
        changed == 1;
}
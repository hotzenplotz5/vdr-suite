#include "HumanAccountCredentialSessionReadRepository.h"

#include "Database.h"

#include <sqlite3.h>
#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

namespace
{
bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128)
        return false;

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

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
               statement, index, value.c_str(), -1, SQLITE_TRANSIENT) ==
        SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* text = sqlite3_column_text(statement, column);
    return text == nullptr
        ? std::string()
        : std::string(reinterpret_cast<const char*>(text));
}

std::string hexEncode(const unsigned char* bytes, std::size_t size)
{
    static constexpr char Hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(size * 2U);
    for (std::size_t index = 0; index < size; ++index)
    {
        result.push_back(Hex[(bytes[index] >> 4U) & 0x0fU]);
        result.push_back(Hex[bytes[index] & 0x0fU]);
    }
    return result;
}

std::string credentialLifecycleRevision(
    const std::string& credentialId,
    const std::string& credentialType,
    bool active,
    bool expired,
    bool revoked,
    const std::string& expiresAt,
    const std::string& createdAt)
{
    std::string normalized = "credential-lifecycle/1\n";
    normalized += credentialId + "\n";
    normalized += credentialType + "\n";
    normalized += active ? "1\n" : "0\n";
    normalized += expired ? "1\n" : "0\n";
    normalized += revoked ? "1\n" : "0\n";
    normalized += expiresAt + "\n";
    normalized += createdAt + "\n";

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digestLength = 0U;
    if (EVP_Digest(
            normalized.data(),
            normalized.size(),
            digest.data(),
            &digestLength,
            EVP_sha256(),
            nullptr) != 1 ||
        digestLength == 0U)
    {
        return {};
    }

    return "credential-lifecycle:" +
        hexEncode(digest.data(), digestLength);
}

std::string sessionLifecycleRevision(
    sqlite3_stmt* statement,
    const std::string& sessionId,
    const std::string& deviceId,
    const std::string& issuedFromCredentialId,
    const std::string& browserCredentialId)
{
    std::string normalized = "session-lifecycle/1\n";
    normalized += sessionId + "\n";
    normalized += deviceId + "\n";
    normalized += issuedFromCredentialId + "\n";
    normalized += browserCredentialId + "\n";

    for (int column = 10; column <= 21; ++column)
    {
        if (column == 10 || column == 13 ||
            column == 16 || column == 19)
        {
            normalized += sqlite3_column_int(statement, column) != 0
                ? "1"
                : "0";
        }
        else
        {
            normalized += columnText(statement, column);
        }
        normalized += "\n";
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digestLength = 0U;
    if (EVP_Digest(
            normalized.data(),
            normalized.size(),
            digest.data(),
            &digestLength,
            EVP_sha256(),
            nullptr) != 1 ||
        digestLength == 0U)
    {
        return {};
    }

    return "session-lifecycle:" +
        hexEncode(digest.data(), digestLength);
}
}

HumanAccountCredentialSessionReadRepository::
HumanAccountCredentialSessionReadRepository(Database& database)
    : database_(database)
{
}

std::optional<std::vector<HumanAccountCredentialMetadata>>
HumanAccountCredentialSessionReadRepository::
listCredentialsByActorId(const std::string& actorId) const
{
    if (!safeIdentifier(actorId))
        return std::nullopt;

    const char* sql =
        "SELECT credential_id, credential_type, active, "
        "(expires_at <> '' AND expires_at <= CURRENT_TIMESTAMP), "
        "revoked_at <> '', expires_at, created_at "
        "FROM security_credentials "
        "WHERE actor_id = ? "
        "AND credential_type <> 'browser-session' "
        "ORDER BY credential_id ASC;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return std::nullopt;
    if (!bindText(statement, 1, actorId))
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::vector<HumanAccountCredentialMetadata> result;
    for (;;)
    {
        const int step = sqlite3_step(statement);
        if (step == SQLITE_DONE)
            break;
        if (step != SQLITE_ROW)
        {
            sqlite3_finalize(statement);
            return std::nullopt;
        }

        HumanAccountCredentialMetadata credential;
        credential.credentialId = columnText(statement, 0);
        credential.credentialType = columnText(statement, 1);
        credential.active = sqlite3_column_int(statement, 2) != 0;
        credential.expired = sqlite3_column_int(statement, 3) != 0;
        credential.revoked = sqlite3_column_int(statement, 4) != 0;
        credential.expiresAt = columnText(statement, 5);
        credential.createdAt = columnText(statement, 6);
        credential.resourceRevision =
            credentialLifecycleRevision(
                credential.credentialId,
                credential.credentialType,
                credential.active,
                credential.expired,
                credential.revoked,
                credential.expiresAt,
                credential.createdAt);
        if (credential.resourceRevision.empty())
        {
            sqlite3_finalize(statement);
            return std::nullopt;
        }
        result.push_back(std::move(credential));
    }

    sqlite3_finalize(statement);
    return result;
}

std::optional<std::vector<HumanAccountSessionMetadata>>
HumanAccountCredentialSessionReadRepository::
listSessionsByActorId(const std::string& actorId) const
{
    if (!safeIdentifier(actorId))
        return std::nullopt;

    const char* sql =
        "SELECT browser.session_id, browser.device_id, "
        "browser.issued_from_credential_id, browser.credential_id, "
        "(browser.active <> 0 AND session.active <> 0 AND "
        " browser_credential.active <> 0 AND issuing_credential.active <> 0), "
        "((browser.expires_at <> '' AND browser.expires_at <= CURRENT_TIMESTAMP) OR "
        " (session.expires_at <> '' AND session.expires_at <= CURRENT_TIMESTAMP) OR "
        " (browser_credential.expires_at <> '' AND browser_credential.expires_at <= CURRENT_TIMESTAMP) OR "
        " (issuing_credential.expires_at <> '' AND issuing_credential.expires_at <= CURRENT_TIMESTAMP)), "
        "((browser.revoked_at <> '') OR (session.revoked_at <> '') OR "
        " (browser_credential.revoked_at <> '') OR (issuing_credential.revoked_at <> '')), "
        "browser.expires_at, browser.last_seen_at, browser.created_at, "
        "browser.active, browser.expires_at, browser.revoked_at, "
        "session.active, session.expires_at, session.revoked_at, "
        "browser_credential.active, browser_credential.expires_at, "
        "browser_credential.revoked_at, issuing_credential.active, "
        "issuing_credential.expires_at, issuing_credential.revoked_at "
        "FROM security_browser_session_credentials AS browser "
        "JOIN security_sessions AS session "
        "ON session.session_id = browser.session_id "
        "AND session.actor_id = browser.actor_id "
        "AND session.device_id = browser.device_id "
        "JOIN security_credentials AS browser_credential "
        "ON browser_credential.credential_id = browser.credential_id "
        "AND browser_credential.actor_id = browser.actor_id "
        "AND browser_credential.credential_type = 'browser-session' "
        "JOIN security_credentials AS issuing_credential "
        "ON issuing_credential.credential_id = browser.issued_from_credential_id "
        "AND issuing_credential.actor_id = browser.actor_id "
        "WHERE browser.actor_id = ? "
        "ORDER BY browser.session_id ASC;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return std::nullopt;
    if (!bindText(statement, 1, actorId))
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::vector<HumanAccountSessionMetadata> result;
    for (;;)
    {
        const int step = sqlite3_step(statement);
        if (step == SQLITE_DONE)
            break;
        if (step != SQLITE_ROW)
        {
            sqlite3_finalize(statement);
            return std::nullopt;
        }

        HumanAccountSessionMetadata session;
        session.sessionId = columnText(statement, 0);
        session.deviceId = columnText(statement, 1);
        session.issuedFromCredentialId = columnText(statement, 2);
        session.browserCredentialId = columnText(statement, 3);
        session.active = sqlite3_column_int(statement, 4) != 0;
        session.expired = sqlite3_column_int(statement, 5) != 0;
        session.revoked = sqlite3_column_int(statement, 6) != 0;
        session.expiresAt = columnText(statement, 7);
        session.lastSeenAt = columnText(statement, 8);
        session.createdAt = columnText(statement, 9);
        session.resourceRevision = sessionLifecycleRevision(
            statement,
            session.sessionId,
            session.deviceId,
            session.issuedFromCredentialId,
            session.browserCredentialId);
        if (session.browserCredentialId.empty() ||
            session.resourceRevision.empty())
        {
            sqlite3_finalize(statement);
            return std::nullopt;
        }
        result.push_back(std::move(session));
    }

    sqlite3_finalize(statement);
    return result;
}

#include "CredentialVerifierRepository.h"

#include "Database.h"

#include <sqlite3.h>

namespace
{
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
}

CredentialVerifierRepository::CredentialVerifierRepository(Database& database)
    : database_(database)
{
}

bool CredentialVerifierRepository::ensureSchema()
{
    return database_.execute(
               "CREATE TABLE IF NOT EXISTS "
               "security_basic_credential_verifiers ("
               "credential_id TEXT PRIMARY KEY,"
               "login_name TEXT NOT NULL UNIQUE,"
               "password_hash TEXT NOT NULL,"
               "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
               "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
               "FOREIGN KEY(credential_id) "
               "REFERENCES security_credentials(credential_id)"
               ");") &&
        database_.execute(
               "CREATE UNIQUE INDEX IF NOT EXISTS "
               "idx_security_basic_verifiers_login "
               "ON security_basic_credential_verifiers(login_name);");
}

bool CredentialVerifierRepository::ensureVerifier(
    const std::string& credentialId,
    const std::string& loginName,
    const std::string& passwordHash)
{
    if (credentialId.empty() || loginName.empty() || passwordHash.empty())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT OR IGNORE INTO security_basic_credential_verifiers "
        "(credential_id, login_name, password_hash) VALUES (?, ?, ?);";

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
        bindText(statement, 1, credentialId) &&
        bindText(statement, 2, loginName) &&
        bindText(statement, 3, passwordHash);
    const int result = bound
        ? sqlite3_step(statement)
        : SQLITE_ERROR;
    sqlite3_finalize(statement);

    if (result != SQLITE_DONE)
    {
        return false;
    }

    const auto stored = findByLogin(loginName);
    return stored.has_value() &&
        stored->credentialId == credentialId &&
        stored->passwordHash == passwordHash;
}

std::optional<StoredBasicCredentialVerifier>
CredentialVerifierRepository::findByLogin(
    const std::string& loginName) const
{
    if (loginName.empty())
    {
        return std::nullopt;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT credential_id, login_name, password_hash "
        "FROM security_basic_credential_verifiers "
        "WHERE login_name = ?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    if (!bindText(statement, 1, loginName))
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::optional<StoredBasicCredentialVerifier> result;
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        StoredBasicCredentialVerifier verifier;
        verifier.credentialId = columnText(statement, 0);
        verifier.loginName = columnText(statement, 1);
        verifier.passwordHash = columnText(statement, 2);
        result = verifier;
    }

    sqlite3_finalize(statement);
    return result;
}

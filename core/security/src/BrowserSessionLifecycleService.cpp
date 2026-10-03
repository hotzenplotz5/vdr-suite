#include "BrowserSessionLifecycleService.h"

#include "BrowserSessionCredentialRepository.h"
#include "Database.h"
#include "SecurityIdentityRepository.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>

namespace
{
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

class DatabaseTransaction
{
public:
    explicit DatabaseTransaction(Database& database)
        : database_(database),
          active_(database_.execute("BEGIN IMMEDIATE;"))
    {
    }

    ~DatabaseTransaction()
    {
        if (active_)
        {
            database_.execute("ROLLBACK;");
        }
    }

    bool active() const noexcept
    {
        return active_;
    }

    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;"))
        {
            return false;
        }
        active_ = false;
        return true;
    }

private:
    Database& database_;
    bool active_ = false;
};
}

BrowserSessionLifecycleService::BrowserSessionLifecycleService(
    Database& database,
    SecurityIdentityRepository& identityRepository,
    BrowserSessionCredentialRepository& credentialRepository)
    : database_(database),
      identityRepository_(identityRepository),
      credentialRepository_(credentialRepository)
{
}

bool BrowserSessionLifecycleService::revokeInActiveTransaction(
    const std::string& sessionId,
    const std::string& credentialId)
{
    if (!database_.transactionActive() ||
        !safeIdentifier(sessionId) ||
        !safeIdentifier(credentialId))
    {
        return false;
    }

    const auto browserCredential =
        credentialRepository_.findBySessionId(sessionId);
    const auto session = identityRepository_.findSession(sessionId);
    const auto credential = identityRepository_.findCredential(credentialId);

    if (!browserCredential.has_value() ||
        browserCredential->credentialId != credentialId ||
        !browserCredential->active ||
        browserCredential->revoked ||
        !session.has_value() ||
        session->sessionId != sessionId ||
        !credential.has_value() ||
        credential->credentialId != credentialId ||
        credential->credentialType != "browser-session")
    {
        return false;
    }

    if (session->active && !session->revoked &&
        !identityRepository_.revokeSession(sessionId))
    {
        return false;
    }

    if (credential->active && !credential->revoked &&
        !identityRepository_.revokeCredential(credentialId))
    {
        return false;
    }

    return credentialRepository_.revokeBySessionId(sessionId);
}

bool BrowserSessionLifecycleService::revoke(
    const std::string& sessionId,
    const std::string& credentialId)
{
    if (!safeIdentifier(sessionId) || !safeIdentifier(credentialId))
    {
        return false;
    }

    auto transactionLease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
    {
        return false;
    }

    return revokeInActiveTransaction(sessionId, credentialId) &&
        transaction.commit();
}

std::optional<std::size_t>
BrowserSessionLifecycleService::revokeAllForActorInActiveTransaction(
    const std::string& actorId)
{
    if (!database_.transactionActive() || !safeIdentifier(actorId))
    {
        return std::nullopt;
    }

    const auto sessions =
        credentialRepository_.findActiveByActorId(actorId);
    if (!sessions.has_value())
    {
        return std::nullopt;
    }

    std::size_t revoked = 0;
    for (const auto& session : *sessions)
    {
        if (session.actorId != actorId ||
            !revokeInActiveTransaction(
                session.sessionId,
                session.credentialId))
        {
            return std::nullopt;
        }
        ++revoked;
    }

    return revoked;
}


std::optional<std::size_t>
BrowserSessionLifecycleService::
revokeIssuedFromCredentialInActiveTransaction(
    const std::string& issuingCredentialId)
{
    if (!database_.transactionActive() ||
        !safeIdentifier(issuingCredentialId))
    {
        return std::nullopt;
    }

    const auto sessions =
        credentialRepository_.findByIssuedFromCredentialId(
            issuingCredentialId);
    if (!sessions.has_value())
        return std::nullopt;

    std::size_t revoked = 0U;
    for (const auto& session : *sessions)
    {
        if (session.issuedFromCredentialId != issuingCredentialId ||
            !revokeInActiveTransaction(
                session.sessionId,
                session.credentialId))
        {
            return std::nullopt;
        }
        ++revoked;
    }

    return revoked;
}

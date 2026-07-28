#include "BrowserSessionLifecycleService.h"

#include "BrowserSessionCredentialRepository.h"
#include "Database.h"
#include "SecurityIdentityRepository.h"

#include <algorithm>
#include <cctype>
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
        !session->active ||
        session->revoked ||
        !credential.has_value() ||
        credential->credentialId != credentialId ||
        credential->credentialType != "browser-session" ||
        !credential->active ||
        credential->revoked)
    {
        return false;
    }

    return credentialRepository_.revokeBySessionId(sessionId) &&
        identityRepository_.revokeSession(sessionId) &&
        identityRepository_.revokeCredential(credentialId) &&
        transaction.commit();
}

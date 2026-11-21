#include "BrowserSessionCredentialRepository.h"
#include "Database.h"
#include "HumanAccountCredentialSessionReadRepository.h"
#include "HumanAccountCredentialSessionReadService.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityRepository.h"

#include <cassert>
#include <cstdio>
#include <string>

int main()
{
    const std::string path =
        "/tmp/vdr-suite-mu8a-credential-session-read-test.db";
    std::remove(path.c_str());

    Database database;
    assert(database.open(path));
    SecurityIdentityRepository identityRepository(database);
    assert(identityRepository.ensureSchema());
    BrowserSessionCredentialRepository browserRepository(database);
    assert(browserRepository.ensureSchema());
    HumanAccountRepository accountRepository(database);
    assert(accountRepository.ensureSchema());

    HumanAccountCredentialSessionReadRepository metadataRepository(database);
    HumanAccountCredentialSessionReadService service(
        accountRepository, identityRepository, metadataRepository);

    assert(database.execute(
        "INSERT INTO security_actors "
        "(actor_id, actor_type, display_name) VALUES "
        "('actor-a', 'user', 'Account A');"));
    assert(database.execute(
        "INSERT INTO security_human_accounts "
        "(account_id, actor_id, display_name) VALUES "
        "('account-a', 'actor-a', 'Account A');"));
    assert(database.execute(
        "INSERT INTO security_credentials "
        "(credential_id, actor_id, credential_type) VALUES "
        "('credential-human', 'actor-a', 'human-password');"));
    assert(database.execute(
        "INSERT INTO security_devices "
        "(device_id, actor_id, display_name) VALUES "
        "('device-a', 'actor-a', 'Browser A');"));
    assert(database.execute(
        "INSERT INTO security_sessions "
        "(session_id, actor_id, device_id, expires_at) VALUES "
        "('session-a', 'actor-a', 'device-a', '2099-01-01 00:00:00');"));
    assert(database.execute(
        "INSERT INTO security_credentials "
        "(credential_id, actor_id, credential_type, expires_at) VALUES "
        "('credential-browser', 'actor-a', 'browser-session', "
        "'2099-01-01 00:00:00');"));

    BrowserSessionCredentialRegistration registration;
    registration.tokenId = "token-a";
    registration.sessionId = "session-a";
    registration.actorId = "actor-a";
    registration.deviceId = "device-a";
    registration.credentialId = "credential-browser";
    registration.issuedFromCredentialId = "credential-human";
    registration.sessionSecretHash =
        "$6$sessionsalt$8tf7lGjGVFN700ih.GaNBFsDQaVkLgsffOM/4VS9ODoyxeEikzL9jMMbsfS2Lu2/A7U.ypuQ1g38ub5YckfEe/";
    registration.csrfSecretHash =
        "$6$csrfsalt$Zht7CPii63YntnxlS0UUgPTs6wcCD7WfThN91jWT8Ub0CzhKDP8nhTYAC13VefMKEyYMpUPZUG7AzYtSuFKSM1";
    registration.expiresAt = "2099-01-01 00:00:00";
    assert(browserRepository.insert(registration));

    const auto credentials = service.readCredentials("account-a");
    assert(credentials.status ==
        HumanAccountCredentialSessionReadStatus::success);
    assert(credentials.credentials.size() == 1U);
    assert(credentials.credentials.front().credentialId ==
        "credential-human");
    assert(credentials.credentials.front().credentialType ==
        "human-password");

    const auto sessions = service.readSessions("account-a");
    assert(sessions.status ==
        HumanAccountCredentialSessionReadStatus::success);
    assert(sessions.sessions.size() == 1U);
    assert(sessions.sessions.front().sessionId == "session-a");
    assert(sessions.sessions.front().issuedFromCredentialId ==
        "credential-human");
    assert(sessions.sessions.front().active);

    assert(identityRepository.revokeCredential("credential-human"));
    const auto fencedSessions = service.readSessions("account-a");
    assert(fencedSessions.sessions.size() == 1U);
    assert(!fencedSessions.sessions.front().active);
    assert(fencedSessions.sessions.front().revoked);

    assert(service.readCredentials("").status ==
        HumanAccountCredentialSessionReadStatus::invalidRequest);
    assert(service.readSessions("missing").status ==
        HumanAccountCredentialSessionReadStatus::accountNotFound);

    std::remove(path.c_str());
    return 0;
}

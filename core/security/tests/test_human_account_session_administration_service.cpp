#include "AccountabilityEventRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionLifecycleService.h"
#include "Database.h"
#include "HumanAccountCredentialSessionReadRepository.h"
#include "HumanAccountCredentialSessionReadService.h"
#include "HumanAccountRepository.h"
#include "HumanAccountSessionAdministrationService.h"
#include "SecurityIdentityRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
bool deterministicEntropy(unsigned char* output, std::size_t size)
{
    if (output == nullptr || size == 0U) return false;

    static unsigned char generation = 0x40;
    ++generation;
    std::memset(output, generation, size);
    return true;
}
}

int main()
{
    const std::string path =
        "/tmp/vdr-suite-mu8b-session-administration-test.db";
    std::remove(path.c_str());

    Database database;
    assert(database.open(path));

    SecurityIdentityRepository identityRepository(database);
    assert(identityRepository.ensureSchema());
    BrowserSessionCredentialRepository browserRepository(database);
    assert(browserRepository.ensureSchema());
    HumanAccountRepository accountRepository(database);
    assert(accountRepository.ensureSchema());
    AccountabilityEventRepository accountabilityRepository(database);
    assert(accountabilityRepository.ensureSchema());

    HumanAccountCredentialSessionReadRepository metadataRepository(database);
    HumanAccountCredentialSessionReadService readService(
        accountRepository,
        identityRepository,
        metadataRepository);
    BrowserSessionLifecycleService lifecycleService(
        database,
        identityRepository,
        browserRepository);
    HumanAccountSessionAdministrationService administrationService(
        database,
        readService,
        lifecycleService,
        accountabilityRepository,
        deterministicEntropy,
        []
        {
            return std::chrono::system_clock::from_time_t(1700000000);
        });

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

    const auto initial =
        readService.readSession("account-a", "session-a");
    assert(initial.status ==
        HumanAccountCredentialSessionReadStatus::success);
    assert(initial.session.active);
    assert(!initial.session.revoked);
    assert(initial.session.browserCredentialId ==
        "credential-browser");
    assert(initial.session.resourceRevision.rfind(
        "session-lifecycle:", 0U) == 0U);

    const std::string initialRevision =
        initial.session.resourceRevision;

    assert(database.execute(
        "UPDATE security_browser_session_credentials "
        "SET last_seen_at = '2098-01-01 00:00:00' "
        "WHERE session_id = 'session-a';"));
    const auto afterLastSeen =
        readService.readSession("account-a", "session-a");
    assert(afterLastSeen.status ==
        HumanAccountCredentialSessionReadStatus::success);
    assert(afterLastSeen.session.resourceRevision ==
        initialRevision);

    HumanAccountSessionAdministrationContext context;
    context.actorId = "actor-admin";
    context.requestId = "request-mu8b";
    context.correlationId = "correlation-mu8b";

    const auto stale = administrationService.revoke(
        context,
        "account-a",
        "session-a",
        "session-lifecycle:stale");
    assert(stale.status ==
        HumanAccountSessionAdministrationStatus::revisionConflict);

    const auto stillActive =
        readService.readSession("account-a", "session-a");
    assert(stillActive.session.active);
    assert(!stillActive.session.revoked);
    assert(stillActive.session.resourceRevision ==
        initialRevision);

    const auto revoked = administrationService.revoke(
        context,
        "account-a",
        "session-a",
        initialRevision);
    assert(revoked.status ==
        HumanAccountSessionAdministrationStatus::success);
    assert(!revoked.session.active);
    assert(revoked.session.revoked);
    assert(revoked.session.resourceRevision != initialRevision);

    const auto browserAfter =
        browserRepository.findBySessionId("session-a");
    assert(browserAfter.has_value());
    assert(!browserAfter->active);
    assert(browserAfter->revoked);

    const auto sessionAfter =
        identityRepository.findSession("session-a");
    assert(sessionAfter.has_value());
    assert(!sessionAfter->active);
    assert(sessionAfter->revoked);

    const auto credentialAfter =
        identityRepository.findCredential("credential-browser");
    assert(credentialAfter.has_value());
    assert(!credentialAfter->active);
    assert(credentialAfter->revoked);

    const auto replay = administrationService.revoke(
        context,
        "account-a",
        "session-a",
        initialRevision);
    assert(replay.status ==
        HumanAccountSessionAdministrationStatus::success);
    assert(replay.session.resourceRevision ==
        revoked.session.resourceRevision);
    assert(replay.session.revoked);

    const auto events = accountabilityRepository.listAll();
    assert(events.size() == 3U);
    assert(std::any_of(
        events.begin(),
        events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.permission == "accounts.sessions.revoke" &&
                event.reasonCode == "session_revision_conflict" &&
                event.decision == "deny";
        }));
    assert(std::any_of(
        events.begin(),
        events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode == "session_revoked" &&
                event.decision == "allow";
        }));
    assert(std::any_of(
        events.begin(),
        events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode == "session_already_terminal" &&
                event.decision == "allow";
        }));

    assert(administrationService.revoke(
        context,
        "missing",
        "session-a",
        initialRevision).status ==
        HumanAccountSessionAdministrationStatus::accountNotFound);
    assert(administrationService.revoke(
        context,
        "account-a",
        "missing",
        initialRevision).status ==
        HumanAccountSessionAdministrationStatus::sessionNotFound);

    std::remove(path.c_str());
    return 0;
}

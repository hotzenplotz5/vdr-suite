#include "AccountabilityEventRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionLifecycleService.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "HumanAccountAdministrationRepository.h"
#include "HumanAccountCredentialAdministrationService.h"
#include "HumanAccountCredentialSessionReadRepository.h"
#include "HumanAccountCredentialSessionReadService.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

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
    static unsigned char generation = 0x42;
    ++generation;
    std::memset(output, generation, size);
    return true;
}
}

int main()
{
    const std::string path =
        "/tmp/vdr-suite-mu8c-credential-administration-test.db";
    std::remove(path.c_str());

    Database database;
    assert(database.open(path));

    SecurityIdentityRepository identityRepository(database);
    assert(identityRepository.ensureSchema());
    BrowserSessionCredentialRepository browserRepository(database);
    assert(browserRepository.ensureSchema());
    HumanAccountRepository accountRepository(database);
    assert(accountRepository.ensureSchema());
    SecurityPermissionGrantRepository grantRepository(database);
    assert(grantRepository.ensureSchema());
    CredentialVerifierRepository verifierRepository(database);
    assert(verifierRepository.ensureSchema());
    AccountabilityEventRepository accountabilityRepository(database);
    assert(accountabilityRepository.ensureSchema());
    HumanAccountAdministrationRepository administrationRepository(
        database);

    HumanAccountCredentialSessionReadRepository metadataRepository(database);
    HumanAccountCredentialSessionReadService readService(
        accountRepository,
        identityRepository,
        metadataRepository);
    BrowserSessionLifecycleService lifecycleService(
        database,
        identityRepository,
        browserRepository);
    HumanAccountCredentialAdministrationService administrationService(
        database,
        readService,
        administrationRepository,
        identityRepository,
        browserRepository,
        lifecycleService,
        accountabilityRepository,
        deterministicEntropy,
        []
        {
            return std::chrono::system_clock::from_time_t(1700000100);
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
        "('credential-human-a', 'actor-a', 'human-password');"));
    assert(verifierRepository.ensureVerifier(
        "credential-human-a",
        "admin-a",
        "$6$testsalt$abcdefghijklmnopqrstuvwx"));
    assert(grantRepository.ensureGrant(
        "actor-a",
        "role.admin",
        "*"));

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
        "('credential-browser-a', 'actor-a', 'browser-session', "
        "'2099-01-01 00:00:00');"));

    BrowserSessionCredentialRegistration registration;
    registration.tokenId = "token-a";
    registration.sessionId = "session-a";
    registration.actorId = "actor-a";
    registration.deviceId = "device-a";
    registration.credentialId = "credential-browser-a";
    registration.issuedFromCredentialId = "credential-human-a";
    registration.sessionSecretHash =
        "$6$sessionsalt$8tf7lGjGVFN700ih.GaNBFsDQaVkLgsffOM/4VS9ODoyxeEikzL9jMMbsfS2Lu2/A7U.ypuQ1g38ub5YckfEe/";
    registration.csrfSecretHash =
        "$6$csrfsalt$Zht7CPii63YntnxlS0UUgPTs6wcCD7WfThN91jWT8Ub0CzhKDP8nhTYAC13VefMKEyYMpUPZUG7AzYtSuFKSM1";
    registration.expiresAt = "2099-01-01 00:00:00";
    assert(browserRepository.insert(registration));

    const auto initial =
        readService.readCredential(
            "account-a",
            "credential-human-a");
    assert(initial.status ==
        HumanAccountCredentialSessionReadStatus::success);
    assert(initial.credential.active);
    assert(!initial.credential.revoked);
    assert(initial.credential.resourceRevision.rfind(
        "credential-lifecycle:", 0U) == 0U);

    HumanAccountCredentialAdministrationContext context;
    context.actorId = "actor-admin";
    context.requestId = "request-mu8c";
    context.correlationId = "correlation-mu8c";

    const auto stale = administrationService.revoke(
        context,
        "account-a",
        "credential-human-a",
        "credential-lifecycle:stale");
    assert(stale.status ==
        HumanAccountCredentialAdministrationStatus::revisionConflict);

    const auto finalBlocked = administrationService.revoke(
        context,
        "account-a",
        "credential-human-a",
        initial.credential.resourceRevision);
    assert(finalBlocked.status ==
        HumanAccountCredentialAdministrationStatus::finalAdministrator);

    const auto stillActive =
        readService.readCredential(
            "account-a",
            "credential-human-a");
    assert(stillActive.credential.active);
    assert(!stillActive.credential.revoked);

    assert(database.execute(
        "INSERT INTO security_credentials "
        "(credential_id, actor_id, credential_type) VALUES "
        "('credential-human-b', 'actor-a', 'human-password');"));
    assert(verifierRepository.ensureVerifier(
        "credential-human-b",
        "admin-b",
        "$6$testsalt2$abcdefghijklmnopqrstuvwx"));

    const auto finalAfterAlternative =
        administrationRepository.
            wouldRevokeFinalUsableAdministrator(
                "credential-human-a");
    assert(finalAfterAlternative.has_value());
    assert(!*finalAfterAlternative);

    const auto revoked = administrationService.revoke(
        context,
        "account-a",
        "credential-human-a",
        initial.credential.resourceRevision);
    assert(revoked.status ==
        HumanAccountCredentialAdministrationStatus::success);
    assert(!revoked.credential.active);
    assert(revoked.credential.revoked);
    assert(revoked.credential.resourceRevision !=
        initial.credential.resourceRevision);
    assert(revoked.revokedBrowserSessions == 1U);

    const auto sourceAfter =
        identityRepository.findCredential(
            "credential-human-a");
    assert(sourceAfter.has_value());
    assert(!sourceAfter->active);
    assert(sourceAfter->revoked);

    const auto alternativeAfter =
        identityRepository.findCredential(
            "credential-human-b");
    assert(alternativeAfter.has_value());
    assert(alternativeAfter->active);
    assert(!alternativeAfter->revoked);

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

    const auto browserCredentialAfter =
        identityRepository.findCredential(
            "credential-browser-a");
    assert(browserCredentialAfter.has_value());
    assert(!browserCredentialAfter->active);
    assert(browserCredentialAfter->revoked);

    const auto replay = administrationService.revoke(
        context,
        "account-a",
        "credential-human-a",
        initial.credential.resourceRevision);
    assert(replay.status ==
        HumanAccountCredentialAdministrationStatus::success);
    assert(replay.credential.revoked);

    const auto events = accountabilityRepository.listAll();
    assert(std::any_of(
        events.begin(),
        events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.permission == "accounts.credentials.revoke" &&
                event.reasonCode == "credential_revision_conflict" &&
                event.decision == "deny";
        }));
    assert(std::any_of(
        events.begin(),
        events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode == "final_usable_administrator" &&
                event.decision == "deny";
        }));
    assert(std::any_of(
        events.begin(),
        events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode == "credential_revoked" &&
                event.decision == "allow";
        }));
    assert(std::any_of(
        events.begin(),
        events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode == "credential_already_terminal" &&
                event.decision == "allow";
        }));

    std::remove(path.c_str());
    return 0;
}

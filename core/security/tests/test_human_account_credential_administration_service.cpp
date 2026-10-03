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
    static unsigned char generation = 0x50;
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
    CredentialVerifierRepository verifierRepository(database);
    assert(verifierRepository.ensureSchema());
    SecurityPermissionGrantRepository grantRepository(database);
    assert(grantRepository.ensureSchema());
    AccountabilityEventRepository accountabilityRepository(database);
    assert(accountabilityRepository.ensureSchema());

    HumanAccountAdministrationRepository administrationRepository(database);
    HumanAccountCredentialSessionReadRepository metadataRepository(database);
    HumanAccountCredentialSessionReadService readService(
        accountRepository,
        identityRepository,
        metadataRepository);
    BrowserSessionLifecycleService lifecycleService(
        database,
        identityRepository,
        browserRepository);
    HumanAccountCredentialAdministrationService service(
        database,
        readService,
        administrationRepository,
        identityRepository,
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
        "('actor-a', 'user', 'Administrator');"));
    assert(database.execute(
        "INSERT INTO security_human_accounts "
        "(account_id, actor_id, display_name) VALUES "
        "('account-a', 'actor-a', 'Administrator');"));
    assert(database.execute(
        "INSERT INTO security_credentials "
        "(credential_id, actor_id, credential_type) VALUES "
        "('credential-human', 'actor-a', 'human-password');"));
    assert(verifierRepository.ensureVerifier(
        "credential-human", "admin", "verifier-a"));
    assert(grantRepository.ensureGrant(
        "actor-a", "role.admin", "*"));

    assert(database.execute(
        "INSERT INTO security_devices "
        "(device_id, actor_id, display_name) VALUES "
        "('device-a', 'actor-a', 'Browser A');"));
    assert(database.execute(
        "INSERT INTO security_sessions "
        "(session_id, actor_id, device_id, expires_at) VALUES "
        "('session-a', 'actor-a', 'device-a', "
        "'2099-01-01 00:00:00');"));
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
        readService.readCredential(
            "account-a", "credential-human");
    assert(initial.status ==
        HumanAccountCredentialSessionReadStatus::success);
    assert(initial.credential.active);
    assert(!initial.credential.revoked);
    assert(initial.credential.resourceRevision.rfind(
        "credential-lifecycle:", 0U) == 0U);
    const std::string initialRevision =
        initial.credential.resourceRevision;

    HumanAccountCredentialAdministrationContext context;
    context.actorId = "actor-operator";
    context.requestId = "request-mu8c";
    context.correlationId = "correlation-mu8c";

    const auto stale = service.revoke(
        context,
        "account-a",
        "credential-human",
        "credential-lifecycle:stale");
    assert(stale.status ==
        HumanAccountCredentialAdministrationStatus::revisionConflict);
    assert(readService.readCredential(
        "account-a", "credential-human").credential.active);

    const auto finalAdmin = service.revoke(
        context,
        "account-a",
        "credential-human",
        initialRevision);
    assert(finalAdmin.status ==
        HumanAccountCredentialAdministrationStatus::finalAdministrator);
    assert(readService.readCredential(
        "account-a", "credential-human").credential.active);
    assert(browserRepository.findBySessionId("session-a")->active);

    assert(database.execute(
        "INSERT INTO security_credentials "
        "(credential_id, actor_id, credential_type) VALUES "
        "('credential-human-alt', 'actor-a', 'human-password');"));
    assert(verifierRepository.ensureVerifier(
        "credential-human-alt", "admin-alt", "verifier-b"));

    const auto beforeRevoke =
        administrationRepository.
            countUsableAdministratorsExcludingCredential("");
    const auto afterTargetRevoke =
        administrationRepository.
            countUsableAdministratorsExcludingCredential(
                "credential-human");
    assert(beforeRevoke.has_value() && *beforeRevoke == 1U);
    assert(afterTargetRevoke.has_value() &&
        *afterTargetRevoke == 1U);

    const auto revoked = service.revoke(
        context,
        "account-a",
        "credential-human",
        initialRevision);
    assert(revoked.status ==
        HumanAccountCredentialAdministrationStatus::success);
    assert(!revoked.credential.active);
    assert(revoked.credential.revoked);
    assert(revoked.credential.resourceRevision != initialRevision);
    assert(revoked.revokedBrowserSessions == 1U);

    const auto sourceCredential =
        identityRepository.findCredential("credential-human");
    assert(sourceCredential.has_value());
    assert(!sourceCredential->active);
    assert(sourceCredential->revoked);

    const auto alternateCredential =
        identityRepository.findCredential("credential-human-alt");
    assert(alternateCredential.has_value());
    assert(alternateCredential->active);
    assert(!alternateCredential->revoked);

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
        identityRepository.findCredential("credential-browser");
    assert(browserCredentialAfter.has_value());
    assert(!browserCredentialAfter->active);
    assert(browserCredentialAfter->revoked);

    const auto replay = service.revoke(
        context,
        "account-a",
        "credential-human",
        initialRevision);
    assert(replay.status ==
        HumanAccountCredentialAdministrationStatus::success);
    assert(replay.credential.revoked);
    assert(replay.credential.resourceRevision ==
        revoked.credential.resourceRevision);
    assert(replay.revokedBrowserSessions == 0U);

    assert(database.execute(
        "INSERT INTO security_credentials "
        "(credential_id, actor_id, credential_type) VALUES "
        "('credential-other', 'actor-a', 'api-key');"));
    const auto other =
        readService.readCredential(
            "account-a", "credential-other");
    assert(other.status ==
        HumanAccountCredentialSessionReadStatus::success);
    const auto unsupported = service.revoke(
        context,
        "account-a",
        "credential-other",
        other.credential.resourceRevision);
    assert(unsupported.status ==
        HumanAccountCredentialAdministrationStatus::
            unsupportedCredentialType);
    assert(identityRepository.findCredential(
        "credential-other")->active);

    const auto events = accountabilityRepository.listAll();
    assert(events.size() == 5U);
    assert(std::any_of(
        events.begin(), events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.permission ==
                    "accounts.credentials.revoke" &&
                event.reasonCode ==
                    "credential_revision_conflict" &&
                event.decision == "deny";
        }));
    assert(std::any_of(
        events.begin(), events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode ==
                    "final_usable_administrator" &&
                event.decision == "deny";
        }));
    assert(std::any_of(
        events.begin(), events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode ==
                    "credential_revoked" &&
                event.decision == "allow";
        }));
    assert(std::any_of(
        events.begin(), events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode ==
                    "credential_already_terminal" &&
                event.decision == "allow";
        }));
    assert(std::any_of(
        events.begin(), events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode ==
                    "credential_type_not_revokeable" &&
                event.decision == "deny";
        }));

    std::remove(path.c_str());
    return 0;
}

#include "AccountabilityEventRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionLifecycleService.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "HumanAccountAdministrationRepository.h"
#include "HumanAccountAdministrationService.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>

namespace
{
const std::string SessionSecretHash =
    "$6$sessionsalt$8tf7lGjGVFN700ih.GaNBFsDQaVkLgsffOM/4VS9ODoyxeEikzL9jMMbsfS2Lu2/A7U.ypuQ1g38ub5YckfEe/";
const std::string CsrfSecretHash =
    "$6$csrfsalt$Zht7CPii63YntnxlS0UUgPTs6wcCD7WfThN91jWT8Ub0CzhKDP8nhTYAC13VefMKEyYMpUPZUG7AzYtSuFKSM1";

struct Fixture
{
    Database database;
    AccountabilityEventRepository accountability;
    SecurityIdentityRepository identities;
    SecurityIdentityProvisioningRepository provisioning;
    HumanAccountRepository accounts;
    HumanAccountAdministrationRepository administration;
    CredentialVerifierRepository verifiers;
    SecurityPermissionGrantRepository grants;
    BrowserSessionCredentialRepository browserCredentials;
    BrowserSessionLifecycleService browserLifecycle;
    unsigned char entropyCounter = 1;
    HumanAccountAdministrationService service;

    Fixture()
        : accountability(database),
          identities(database),
          provisioning(database),
          accounts(database),
          administration(database),
          verifiers(database),
          grants(database),
          browserCredentials(database),
          browserLifecycle(
              database,
              identities,
              browserCredentials),
          service(
              database,
              accounts,
              administration,
              identities,
              browserLifecycle,
              accountability,
              [this](unsigned char* output, std::size_t size)
              {
                  for (std::size_t index = 0; index < size; ++index)
                  {
                      output[index] = entropyCounter++;
                  }
                  return true;
              },
              []
              {
                  return std::chrono::system_clock::time_point(
                      std::chrono::seconds(1700000000));
              })
    {
        assert(database.open(":memory:"));
        assert(accountability.ensureSchema());
        assert(identities.ensureSchema());
        assert(accounts.ensureSchema());
        assert(verifiers.ensureSchema());
        assert(grants.ensureSchema());
        assert(browserCredentials.ensureSchema());
    }

    void createHumanAccount(
        const std::string& accountId,
        const std::string& actorId,
        const std::string& displayName,
        const std::string& credentialId,
        const std::string& loginName,
        bool administrator)
    {
        auto lease = database.acquireTransactionLease();
        assert(database.execute("BEGIN IMMEDIATE;"));
        assert(provisioning.ensureHumanCredentialInActiveTransaction(
            actorId,
            displayName,
            credentialId,
            "human-password"));
        assert(accounts.ensureAccountInActiveTransaction(
            accountId,
            actorId,
            displayName));
        assert(database.execute("COMMIT;"));

        assert(verifiers.ensureVerifier(
            credentialId,
            loginName,
            "$6$test$not-a-login-test"));
        if (administrator)
        {
            assert(grants.ensureGrant(
                actorId,
                "role.admin",
                "*"));
        }
    }

    void createBrowserSession(
        const std::string& actorId,
        const std::string& issuingCredentialId,
        const std::string& deviceId,
        const std::string& sessionId,
        const std::string& browserCredentialId,
        const std::string& tokenId)
    {
        assert(provisioning.ensureHumanBrowserDevice(
            actorId,
            deviceId,
            "Human Account browser"));
        assert(identities.createSessionCredential(
            sessionId,
            actorId,
            deviceId,
            browserCredentialId,
            "browser-session",
            "2099-01-01 00:00:00",
            issuingCredentialId));

        BrowserSessionCredentialRegistration registration;
        registration.tokenId = tokenId;
        registration.sessionId = sessionId;
        registration.actorId = actorId;
        registration.deviceId = deviceId;
        registration.credentialId = browserCredentialId;
        registration.issuedFromCredentialId = issuingCredentialId;
        registration.sessionSecretHash = SessionSecretHash;
        registration.csrfSecretHash = CsrfSecretHash;
        registration.expiresAt = "2099-01-01 00:00:00";
        assert(browserCredentials.insert(registration));
    }
};

HumanAccountAdministrationContext context(
    const std::string& requestId)
{
    HumanAccountAdministrationContext value;
    value.actorId = "actor-admin-1";
    value.actorType = ActorType::User;
    value.requestId = requestId;
    value.correlationId = "correlation-mu6";
    return value;
}
}

int main()
{
    Fixture fixture;
    fixture.createHumanAccount(
        "account-admin-1",
        "actor-admin-1",
        "Administrator One",
        "credential-human-1",
        "admin-one",
        true);

    const auto initial =
        fixture.accounts.findByAccountId("account-admin-1");
    assert(initial.status == HumanAccountRepositoryStatus::ok);
    assert(initial.account.revision == 1U);
    assert(initial.account.active);

    const auto rename = fixture.service.modifyDisplayName(
        context("request-rename"),
        "account-admin-1",
        initial.account.revision,
        "Administrator Eins");
    assert(rename.status ==
        HumanAccountAdministrationStatus::success);
    assert(rename.account.displayName == "Administrator Eins");
    assert(rename.account.revision == 2U);

    const auto actorAfterRename =
        fixture.identities.findActor("actor-admin-1");
    assert(actorAfterRename.has_value());
    assert(actorAfterRename->displayName == "Administrator Eins");

    const auto staleRename = fixture.service.modifyDisplayName(
        context("request-stale"),
        "account-admin-1",
        1U,
        "Must Not Win");
    assert(staleRename.status ==
        HumanAccountAdministrationStatus::revisionConflict);
    const auto afterStale =
        fixture.accounts.findByAccountId("account-admin-1");
    assert(afterStale.status == HumanAccountRepositoryStatus::ok);
    assert(afterStale.account.displayName == "Administrator Eins");
    assert(afterStale.account.revision == 2U);

    const auto finalAdminDenied = fixture.service.setActive(
        context("request-final-admin"),
        "account-admin-1",
        2U,
        false);
    assert(finalAdminDenied.status ==
        HumanAccountAdministrationStatus::finalAdministrator);
    const auto stillActive =
        fixture.accounts.findByAccountId("account-admin-1");
    assert(stillActive.status == HumanAccountRepositoryStatus::ok);
    assert(stillActive.account.active);
    assert(stillActive.account.revision == 2U);

    fixture.createHumanAccount(
        "account-admin-2",
        "actor-admin-2",
        "Administrator Two",
        "credential-human-2",
        "admin-two",
        true);

    fixture.createBrowserSession(
        "actor-admin-1",
        "credential-human-1",
        "device-admin-1",
        "session-admin-1",
        "credential-browser-1",
        "token-admin-1");

    const auto browserBefore =
        fixture.browserCredentials.findBySessionId(
            "session-admin-1");
    assert(browserBefore.has_value());
    assert(browserBefore->active);
    assert(!browserBefore->revoked);

    const auto deactivate = fixture.service.setActive(
        context("request-deactivate"),
        "account-admin-1",
        2U,
        false);
    assert(deactivate.status ==
        HumanAccountAdministrationStatus::success);
    assert(!deactivate.account.active);
    assert(deactivate.account.revision == 3U);
    assert(deactivate.revokedBrowserSessions == 1U);

    const auto browserAfter =
        fixture.browserCredentials.findBySessionId(
            "session-admin-1");
    assert(browserAfter.has_value());
    assert(!browserAfter->active);
    assert(browserAfter->revoked);

    const auto sessionAfter =
        fixture.identities.findSession("session-admin-1");
    assert(sessionAfter.has_value());
    assert(!sessionAfter->active);
    assert(sessionAfter->revoked);

    const auto browserCredentialAfter =
        fixture.identities.findCredential(
            "credential-browser-1");
    assert(browserCredentialAfter.has_value());
    assert(!browserCredentialAfter->active);
    assert(browserCredentialAfter->revoked);

    const auto reactivate = fixture.service.setActive(
        context("request-reactivate"),
        "account-admin-1",
        3U,
        true);
    assert(reactivate.status ==
        HumanAccountAdministrationStatus::success);
    assert(reactivate.account.active);
    assert(reactivate.account.revision == 4U);
    assert(reactivate.revokedBrowserSessions == 0U);

    const auto browserAfterReactivate =
        fixture.browserCredentials.findBySessionId(
            "session-admin-1");
    assert(browserAfterReactivate.has_value());
    assert(!browserAfterReactivate->active);
    assert(browserAfterReactivate->revoked);

    const auto noOpActive = fixture.service.setActive(
        context("request-already-active"),
        "account-admin-1",
        4U,
        true);
    assert(noOpActive.status ==
        HumanAccountAdministrationStatus::success);
    assert(noOpActive.account.revision == 4U);

    const auto events = fixture.accountability.listAll();
    assert(events.size() == 6U);
    assert(std::count_if(
        events.begin(),
        events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode ==
                "final_usable_administrator";
        }) == 1);
    assert(std::count_if(
        events.begin(),
        events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode ==
                "account_revision_conflict";
        }) == 1);
    assert(std::count_if(
        events.begin(),
        events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode ==
                "account_deactivated";
        }) == 1);

    return 0;
}

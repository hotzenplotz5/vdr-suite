#include "AccountabilityEventRepository.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "HumanAccountAdministrationRepository.h"
#include "HumanAccountGrantAdministrationService.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>

namespace
{
struct Fixture
{
    Database database;
    AccountabilityEventRepository accountability;
    SecurityIdentityRepository identities;
    SecurityIdentityProvisioningRepository provisioning;
    HumanAccountRepository accounts;
    HumanAccountAdministrationRepository administration;
    SecurityPermissionGrantRepository grants;
    CredentialVerifierRepository verifiers;
    unsigned char entropyCounter = 1;
    HumanAccountGrantAdministrationService service;

    Fixture(bool secondAdmin = false)
        : accountability(database),
          identities(database),
          provisioning(database),
          accounts(database),
          administration(database),
          grants(database),
          verifiers(database),
          service(
              database,
              accounts,
              identities,
              grants,
              administration,
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
        assert(grants.ensureSchema());
        assert(verifiers.ensureSchema());

        createHuman(
            "account-admin",
            "actor-admin",
            "credential-admin",
            "admin",
            "Administrator");
        createHuman(
            "account-viewer",
            "actor-viewer",
            "credential-viewer",
            "viewer",
            "Viewer");

        assert(grants.ensureGrant(
            "actor-admin",
            "role.admin",
            "*"));

        if (secondAdmin)
        {
            assert(grants.ensureGrant(
                "actor-viewer",
                "role.admin",
                "*"));
        }
    }

    void createHuman(
        const std::string& accountId,
        const std::string& actorId,
        const std::string& credentialId,
        const std::string& loginName,
        const std::string& displayName)
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
            "$6$test$not-used-for-login"));
    }
};

HumanAccountGrantAdministrationContext context()
{
    HumanAccountGrantAdministrationContext value;
    value.actorId = "actor-admin";
    value.requestId = "mu7-grant-test";
    value.correlationId = "mu7-correlation";
    return value;
}

bool hasGrant(
    const HumanAccountGrantSet& set,
    const std::string& permission,
    const std::string& backendId)
{
    for (const PermissionGrant& grant : set.grants)
    {
        if (grant.permission == permission &&
            grant.backendId == backendId)
        {
            return true;
        }
    }
    return false;
}
}

int main()
{
    {
        Fixture fixture;

        const HumanAccountLookupResult beforeAccount =
            fixture.accounts.findByAccountId("account-viewer");
        assert(beforeAccount.status ==
            HumanAccountRepositoryStatus::ok);

        const HumanAccountGrantAdministrationResult initial =
            fixture.service.read("account-viewer");
        assert(initial.status ==
            HumanAccountGrantAdministrationStatus::success);
        assert(initial.grantSet.accountId == "account-viewer");
        assert(initial.grantSet.actorId == "actor-viewer");
        assert(initial.grantSet.grants.empty());
        assert(initial.grantSet.resourceRevision.find(
            "grant-set:") == 0U);

        // Internal/unsupported low-level grants are deliberately invisible
        // to the MU.7 product grant set and do not perturb its revision.
        assert(fixture.grants.ensureGrant(
            "actor-viewer",
            "authentication.access",
            "*"));
        const HumanAccountGrantAdministrationResult internalOnly =
            fixture.service.read("account-viewer");
        assert(internalOnly.status ==
            HumanAccountGrantAdministrationStatus::success);
        assert(internalOnly.grantSet.grants.empty());
        assert(internalOnly.grantSet.resourceRevision ==
            initial.grantSet.resourceRevision);

        const HumanAccountGrantAdministrationResult ensured =
            fixture.service.setGrant(
                context(),
                "account-viewer",
                initial.grantSet.resourceRevision,
                "channels.view",
                "default",
                true);
        assert(ensured.status ==
            HumanAccountGrantAdministrationStatus::success);
        assert(hasGrant(
            ensured.grantSet,
            "channels.view",
            "default"));
        assert(ensured.grantSet.resourceRevision !=
            initial.grantSet.resourceRevision);

        // Repeating the same desired state with a stale observed revision is
        // terminal success and must not change the grant-set revision.
        const HumanAccountGrantAdministrationResult replayEnsure =
            fixture.service.setGrant(
                context(),
                "account-viewer",
                initial.grantSet.resourceRevision,
                "channels.view",
                "default",
                true);
        assert(replayEnsure.status ==
            HumanAccountGrantAdministrationStatus::success);
        assert(replayEnsure.grantSet.resourceRevision ==
            ensured.grantSet.resourceRevision);

        // The same stale revision cannot change a different tuple.
        const HumanAccountGrantAdministrationResult conflict =
            fixture.service.setGrant(
                context(),
                "account-viewer",
                initial.grantSet.resourceRevision,
                "timers.view",
                "default",
                true);
        assert(conflict.status ==
            HumanAccountGrantAdministrationStatus::revisionConflict);
        assert(!hasGrant(
            conflict.grantSet,
            "timers.view",
            "default"));

        // Revoke of an already-absent tuple is likewise terminal success,
        // even from stale observed state.
        const HumanAccountGrantAdministrationResult absentRevoke =
            fixture.service.setGrant(
                context(),
                "account-viewer",
                initial.grantSet.resourceRevision,
                "timers.view",
                "default",
                false);
        assert(absentRevoke.status ==
            HumanAccountGrantAdministrationStatus::success);
        assert(absentRevoke.grantSet.resourceRevision ==
            ensured.grantSet.resourceRevision);

        const HumanAccountGrantAdministrationResult revoked =
            fixture.service.setGrant(
                context(),
                "account-viewer",
                ensured.grantSet.resourceRevision,
                "channels.view",
                "default",
                false);
        assert(revoked.status ==
            HumanAccountGrantAdministrationStatus::success);
        assert(!hasGrant(
            revoked.grantSet,
            "channels.view",
            "default"));
        assert(revoked.grantSet.resourceRevision ==
            initial.grantSet.resourceRevision);

        // Account-owned revision is independent from grant-set revision.
        const HumanAccountLookupResult afterAccount =
            fixture.accounts.findByAccountId("account-viewer");
        assert(afterAccount.status ==
            HumanAccountRepositoryStatus::ok);
        assert(afterAccount.account.revision ==
            beforeAccount.account.revision);

        const HumanAccountGrantAdministrationResult invalidInternal =
            fixture.service.setGrant(
                context(),
                "account-viewer",
                revoked.grantSet.resourceRevision,
                "authentication.access",
                "*",
                true);
        assert(invalidInternal.status ==
            HumanAccountGrantAdministrationStatus::invalidGrant);

        const HumanAccountGrantAdministrationResult invalidScope =
            fixture.service.setGrant(
                context(),
                "account-viewer",
                revoked.grantSet.resourceRevision,
                "channels.view",
                "unsafe/scope",
                true);
        assert(invalidScope.status ==
            HumanAccountGrantAdministrationStatus::invalidGrant);

        const HumanAccountGrantAdministrationResult missing =
            fixture.service.read("does-not-exist");
        assert(missing.status ==
            HumanAccountGrantAdministrationStatus::accountNotFound);
    }

    {
        // The sole usable administrator cannot lose role.admin@*.
        Fixture fixture;
        const HumanAccountGrantAdministrationResult adminSet =
            fixture.service.read("account-admin");
        assert(adminSet.status ==
            HumanAccountGrantAdministrationStatus::success);
        assert(hasGrant(
            adminSet.grantSet,
            "role.admin",
            "*"));

        const HumanAccountGrantAdministrationResult blocked =
            fixture.service.setGrant(
                context(),
                "account-admin",
                adminSet.grantSet.resourceRevision,
                "role.admin",
                "*",
                false);
        assert(blocked.status ==
            HumanAccountGrantAdministrationStatus::finalAdministrator);

        const HumanAccountGrantAdministrationResult stillAdmin =
            fixture.service.read("account-admin");
        assert(stillAdmin.status ==
            HumanAccountGrantAdministrationStatus::success);
        assert(hasGrant(
            stillAdmin.grantSet,
            "role.admin",
            "*"));
    }

    {
        // With another usable administrator, role.admin@* can be revoked.
        Fixture fixture(true);
        const HumanAccountGrantAdministrationResult adminSet =
            fixture.service.read("account-admin");
        assert(adminSet.status ==
            HumanAccountGrantAdministrationStatus::success);

        const HumanAccountGrantAdministrationResult revokedAdmin =
            fixture.service.setGrant(
                context(),
                "account-admin",
                adminSet.grantSet.resourceRevision,
                "role.admin",
                "*",
                false);
        assert(revokedAdmin.status ==
            HumanAccountGrantAdministrationStatus::success);
        assert(!hasGrant(
            revokedAdmin.grantSet,
            "role.admin",
            "*"));
    }

    return 0;
}

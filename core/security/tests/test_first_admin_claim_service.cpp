#include "AccountabilityEventRepository.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "FirstAdminBootstrapRepository.h"
#include "FirstAdminClaimService.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <crypt.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace
{
const std::string SetupSecret =
    "bootstrap-secret-0123456789abcdef";
const std::string Password =
    "first-admin-password";

std::string bootstrapVerifier(const std::string& secret)
{
    crypt_data data{};
    char* encoded = crypt_r(
        secret.c_str(),
        "$6$rounds=10000$claimtest$",
        &data);
    assert(encoded != nullptr);
    return encoded;
}

std::vector<unsigned char> bytes(
    unsigned char start,
    std::size_t size)
{
    std::vector<unsigned char> result(size);
    for (std::size_t index = 0; index < size; ++index)
    {
        result[index] =
            static_cast<unsigned char>(start + index);
    }
    return result;
}

FirstAdminClaimService::EntropySource claimEntropy(
    unsigned char start)
{
    std::vector<std::vector<unsigned char>> chunks{
        bytes(start, 16),
        bytes(static_cast<unsigned char>(start + 0x10), 16),
        bytes(static_cast<unsigned char>(start + 0x20), 16),
        bytes(static_cast<unsigned char>(start + 0x30), 16),
        bytes(static_cast<unsigned char>(start + 0x40), 32),
    };

    return [chunks = std::move(chunks), index = std::size_t{0}](
               unsigned char* output,
               std::size_t size) mutable
    {
        if (output == nullptr ||
            index >= chunks.size() ||
            chunks[index].size() != size)
        {
            return false;
        }

        std::copy(
            chunks[index].begin(),
            chunks[index].end(),
            output);
        ++index;
        return true;
    };
}

FirstAdminClaimRequest requestFor(
    const std::string& bootstrapId,
    const std::string& setupSecret = SetupSecret)
{
    FirstAdminClaimRequest request;
    request.bootstrapId = bootstrapId;
    request.setupSecret = setupSecret;
    request.loginName = "admin";
    request.password = Password;
    request.displayName = "First administrator";
    request.requestId = "request-first-admin-claim";
    request.correlationId = "correlation-first-admin-claim";
    return request;
}

struct Fixture
{
    Database database;
    SecurityIdentityRepository identities;
    SecurityIdentityProvisioningRepository provisioning;
    HumanAccountRepository accounts;
    CredentialVerifierRepository verifiers;
    SecurityPermissionGrantRepository grants;
    AccountabilityEventRepository accountability;
    FirstAdminBootstrapRepository bootstraps;

    Fixture()
        : identities(database),
          provisioning(database),
          accounts(database),
          verifiers(database),
          grants(database),
          accountability(database),
          bootstraps(database)
    {
        assert(database.open(":memory:"));
        assert(identities.ensureSchema());
        assert(accounts.ensureSchema());
        assert(verifiers.ensureSchema());
        assert(grants.ensureSchema());
        assert(accountability.ensureSchema());
        assert(bootstraps.ensureSchema());
    }

    void registerBootstrap(const std::string& bootstrapId)
    {
        FirstAdminBootstrapRegistration registration;
        registration.bootstrapId = bootstrapId;
        registration.verifierHash =
            bootstrapVerifier(SetupSecret);
        registration.expiresAt = "2099-01-01 00:00:00";
        assert(
            bootstraps.registerBootstrap(registration) ==
            FirstAdminBootstrapStatus::ok);
    }

    FirstAdminClaimService service(unsigned char entropyStart)
    {
        return FirstAdminClaimService(
            database,
            bootstraps,
            provisioning,
            accounts,
            verifiers,
            grants,
            accountability,
            claimEntropy(entropyStart),
            []
            {
                return std::chrono::system_clock::time_point(
                    std::chrono::seconds(4070908800));
            });
    }
};


}

int main()
{
    {
        Fixture fixture;
        fixture.registerBootstrap("bootstrap-success");

        FirstAdminClaimService service =
            fixture.service(0x10);
        const FirstAdminClaimResult result =
            service.claim(
                requestFor("bootstrap-success"));

        assert(result.status == FirstAdminClaimStatus::success);
        assert(!result.accountId.empty());
        assert(!result.actorId.empty());
        assert(!result.credentialId.empty());

        const HumanAccountLookupResult account =
            fixture.accounts.findByAccountId(
                result.accountId);
        assert(account.status == HumanAccountRepositoryStatus::ok);
        assert(account.account.actorId == result.actorId);
        assert(account.account.displayName == "First administrator");
        assert(account.account.active);

        const auto actor =
            fixture.identities.findActor(result.actorId);
        assert(actor.has_value());
        assert(actor->type == ActorType::User);
        assert(actor->active);
        assert(!actor->revoked);

        const auto credential =
            fixture.identities.findCredential(
                result.credentialId);
        assert(credential.has_value());
        assert(credential->actorId == result.actorId);
        assert(
            credential->credentialType ==
            "human-password");

        const auto verifier =
            fixture.verifiers.findByLogin("admin");
        assert(verifier.has_value());
        assert(
            verifier->credentialId ==
            result.credentialId);
        assert(verifier->passwordHash != Password);
        assert(
            verifier->passwordHash.rfind("$y$", 0) == 0);

        const SecurityPermissionGrantResolution grants =
            fixture.grants.findActiveGrantsForActor(
                result.actorId);
        assert(grants.available);
        assert(grants.grants.size() == 1);
        assert(grants.grants[0].permission == "role.admin");
        assert(grants.grants[0].backendId == "*");

        const auto events = fixture.accountability.listAll();
        assert(events.size() == 1);
        assert(
            events[0].eventType ==
            "security.first-admin.claim");
        assert(events[0].actorId == result.actorId);
        assert(events[0].actorType == "user");
        assert(events[0].authenticationState == "bootstrap");
        assert(events[0].permission == "role.admin");
        assert(events[0].backendId == "*");
        assert(events[0].decision == "allow");
        assert(events[0].outcome == "success");

        assert(
            fixture.bootstraps.claimState() ==
            FirstAdminClaimState::claimed);

        FirstAdminClaimService replayService =
            fixture.service(0x70);
        assert(
            replayService.claim(
                requestFor("bootstrap-success")).status ==
            FirstAdminClaimStatus::claimed);
    }

    {
        Fixture fixture;
        fixture.registerBootstrap("bootstrap-rejected");

        FirstAdminClaimService service =
            fixture.service(0x20);
        const FirstAdminClaimResult result =
            service.claim(
                requestFor(
                    "bootstrap-rejected",
                    "wrong-bootstrap-secret-0123456789"));

        assert(
            result.status ==
            FirstAdminClaimStatus::bootstrapRejected);
        assert(
            fixture.bootstraps.findById(
                "bootstrap-rejected").status ==
            FirstAdminBootstrapStatus::ok);
        assert(fixture.accounts.listAll().accounts.empty());
        assert(
            fixture.bootstraps.claimState() ==
            FirstAdminClaimState::unclaimed);
    }

    {
        Fixture fixture;
        fixture.registerBootstrap("bootstrap-rollback");

        AccountabilityEvent duplicateEvent;
        duplicateEvent.eventId =
            "ace_606162636465666768696a6b6c6d6e6f";
        duplicateEvent.eventType = "test.duplicate";
        duplicateEvent.requestId = "preexisting-accountability-event";
        assert(fixture.accountability.append(duplicateEvent));

        FirstAdminClaimService service =
            fixture.service(0x30);
        const FirstAdminClaimResult result =
            service.claim(
                requestFor("bootstrap-rollback"));

        assert(
            result.status ==
            FirstAdminClaimStatus::storageError);
        assert(
            fixture.bootstraps.findById(
                "bootstrap-rollback").status ==
            FirstAdminBootstrapStatus::ok);
        assert(fixture.accounts.listAll().accounts.empty());

        const std::string rolledBackActorId =
            "actor_404142434445464748494a4b4c4d4e4f";
        const std::string rolledBackCredentialId =
            "credential_505152535455565758595a5b5c5d5e5f";
        assert(
            !fixture.identities.findActor(
                rolledBackActorId).has_value());
        assert(
            !fixture.identities.findCredential(
                rolledBackCredentialId).has_value());
        assert(
            !fixture.verifiers.findByLogin(
                "admin").has_value());

        const SecurityPermissionGrantResolution rolledBackGrants =
            fixture.grants.findActiveGrantsForActor(
                rolledBackActorId);
        assert(rolledBackGrants.available);
        assert(rolledBackGrants.grants.empty());

        const auto events = fixture.accountability.listAll();
        assert(events.size() == 1);
        assert(events[0].eventType == "test.duplicate");

        assert(
            fixture.bootstraps.claimState() ==
            FirstAdminClaimState::unclaimed);
    }

    return 0;
}

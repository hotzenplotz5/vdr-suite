#include "AccountabilityEventRepository.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "HumanAccountCreationRepository.h"
#include "HumanAccountCreationService.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>
#include <utility>

namespace
{
struct Fixture
{
    Database database;
    AccountabilityEventRepository accountability;
    SecurityIdentityRepository identities;
    SecurityIdentityProvisioningRepository provisioning;
    HumanAccountRepository accounts;
    HumanAccountCreationRepository creation;
    CredentialVerifierRepository verifiers;
    SecurityPermissionGrantRepository grants;
    unsigned char entropyCounter = 1;
    HumanAccountCreationService service;

    Fixture()
        : accountability(database),
          identities(database),
          provisioning(database),
          accounts(database),
          creation(database),
          verifiers(database),
          grants(database),
          service(
              database,
              provisioning,
              accounts,
              creation,
              verifiers,
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
        assert(creation.ensureSchema());
        assert(verifiers.ensureSchema());
        assert(grants.ensureSchema());

        auto lease = database.acquireTransactionLease();
        assert(database.execute("BEGIN IMMEDIATE;"));
        assert(provisioning.ensureHumanCredentialInActiveTransaction(
            "actor-admin",
            "Administrator",
            "credential-admin",
            "human-password"));
        assert(accounts.ensureAccountInActiveTransaction(
            "account-admin",
            "actor-admin",
            "Administrator"));
        assert(database.execute("COMMIT;"));
        assert(verifiers.ensureVerifier(
            "credential-admin",
            "admin",
            "$6$test$not-a-login-test"));
        assert(grants.ensureGrant(
            "actor-admin",
            "role.admin",
            "*"));
    }
};

HumanAccountCreationRequest request(
    const std::string& key,
    const std::string& login = "viewer",
    const std::string& displayName = "Viewer")
{
    HumanAccountCreationRequest value;
    value.actorId = "actor-admin";
    value.loginName = login;
    value.displayName = displayName;
    value.password = "initial-password";
    value.idempotencyKey = key;
    value.requestId = "request-" + key;
    value.correlationId = "correlation-mu6d";
    return value;
}
}

int main()
{
    Fixture fixture;

    const HumanAccountCreationResult created =
        fixture.service.create(request("idem-1"));
    assert(created.status == HumanAccountCreationStatus::success);
    assert(!created.account.accountId.empty());
    assert(!created.account.actorId.empty());
    assert(created.account.displayName == "Viewer");
    assert(created.account.active);
    assert(created.account.revision == 1U);

    const auto actor =
        fixture.identities.findActor(created.account.actorId);
    assert(actor.has_value());
    assert(actor->type == ActorType::User);
    assert(actor->displayName == "Viewer");

    const auto verifier = fixture.verifiers.findByLogin("viewer");
    assert(verifier.has_value());
    const auto credential =
        fixture.identities.findCredential(verifier->credentialId);
    assert(credential.has_value());
    assert(credential->actorId == created.account.actorId);
    assert(credential->credentialType == "human-password");
    assert(credential->active);
    assert(!credential->revoked);

    const SecurityPermissionGrantResolution createdGrants =
        fixture.grants.findActiveGrantsForActor(created.account.actorId);
    assert(createdGrants.available);
    assert(createdGrants.grants.empty());

    const unsigned char entropyAfterCreate =
        fixture.entropyCounter;
    HumanAccountCreationRequest replayRequest =
        request("idem-1");
    replayRequest.password = "different-retry-password";
    const HumanAccountCreationResult replayed =
        fixture.service.create(std::move(replayRequest));
    assert(replayed.status == HumanAccountCreationStatus::replayed);
    assert(replayed.account.accountId == created.account.accountId);
    assert(fixture.entropyCounter == entropyAfterCreate);
    assert(fixture.accounts.listAll().accounts.size() == 2U);

    const HumanAccountCreationResult idempotencyConflict =
        fixture.service.create(
            request("idem-1", "viewer-two", "Viewer Two"));
    assert(idempotencyConflict.status ==
        HumanAccountCreationStatus::idempotencyConflict);
    assert(fixture.accounts.listAll().accounts.size() == 2U);

    const HumanAccountCreationResult loginConflict =
        fixture.service.create(
            request("idem-2", "viewer", "Another Viewer"));
    assert(loginConflict.status ==
        HumanAccountCreationStatus::loginConflict);
    assert(fixture.accounts.listAll().accounts.size() == 2U);

    const auto binding = fixture.creation.find(
        "actor-admin",
        "idem-1");
    assert(binding.has_value());
    assert(binding->accountId == created.account.accountId);
    assert(binding->loginName == "viewer");
    assert(binding->displayName == "Viewer");

    const auto events = fixture.accountability.listAll();
    bool sawCreate = false;
    for (const AccountabilityEvent& event : events)
    {
        if (event.eventType == "security.human-account.create" &&
            event.permission == "accounts.create" &&
            event.actorId == "actor-admin" &&
            event.reasonCode == "account_created")
        {
            sawCreate = true;
        }
    }
    assert(sawCreate);

    return 0;
}

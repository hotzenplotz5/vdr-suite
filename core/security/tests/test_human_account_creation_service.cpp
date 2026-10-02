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

namespace
{
struct Fixture
{
    Database database;
    SecurityIdentityRepository identities;
    SecurityIdentityProvisioningRepository provisioning;
    HumanAccountRepository accounts;
    HumanAccountCreationRepository creation;
    CredentialVerifierRepository verifiers;
    SecurityPermissionGrantRepository grants;
    AccountabilityEventRepository accountability;
    unsigned char entropyCounter = 1;
    HumanAccountCreationService service;

    Fixture()
        : identities(database),
          provisioning(database),
          accounts(database),
          creation(database),
          verifiers(database),
          grants(database),
          accountability(database),
          service(
              database,
              provisioning,
              accounts,
              creation,
              verifiers,
              accountability,
              [this](unsigned char* output, std::size_t size)
              {
                  if (output == nullptr || size == 0U)
                  {
                      return false;
                  }
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
        assert(identities.ensureSchema());
        assert(accounts.ensureSchema());
        assert(creation.ensureSchema());
        assert(verifiers.ensureSchema());
        assert(grants.ensureSchema());
        assert(accountability.ensureSchema());

        auto lease = database.acquireTransactionLease();
        assert(database.execute("BEGIN IMMEDIATE;"));
        assert(provisioning.ensureHumanCredentialInActiveTransaction(
            "actor-request-admin",
            "Request Administrator",
            "credential-request-admin",
            "human-password"));
        assert(database.execute("COMMIT;"));
    }
};

HumanAccountCreationRequest requestFor(
    const std::string& key,
    const std::string& loginName,
    const std::string& password,
    const std::string& displayName)
{
    HumanAccountCreationRequest request;
    request.actorId = "actor-request-admin";
    request.actorType = ActorType::User;
    request.idempotencyKey = key;
    request.loginName = loginName;
    request.password = password;
    request.displayName = displayName;
    request.requestId = "request-mu6d-create";
    request.correlationId = "correlation-mu6d-create";
    return request;
}
}

int main()
{
    Fixture fixture;

    const HumanAccountCreationResult created =
        fixture.service.create(requestFor(
            "idem-account-create-1",
            "alice",
            "initial-password-one",
            "Alice Viewer"));

    assert(created.status == HumanAccountCreationStatus::success);
    assert(!created.account.accountId.empty());
    assert(!created.account.actorId.empty());
    assert(created.account.displayName == "Alice Viewer");
    assert(created.account.active);
    assert(created.account.revision == 1U);

    const auto actor =
        fixture.identities.findActor(created.account.actorId);
    assert(actor.has_value());
    assert(actor->type == ActorType::User);
    assert(actor->displayName == "Alice Viewer");
    assert(actor->active);
    assert(!actor->revoked);

    const auto verifier = fixture.verifiers.findByLogin("alice");
    assert(verifier.has_value());
    assert(verifier->passwordHash != "initial-password-one");
    assert(verifier->passwordHash.rfind("$y$", 0) == 0);

    const auto credential =
        fixture.identities.findCredential(verifier->credentialId);
    assert(credential.has_value());
    assert(credential->actorId == created.account.actorId);
    assert(credential->credentialType == "human-password");
    assert(credential->active);
    assert(!credential->expired);
    assert(!credential->revoked);

    const SecurityPermissionGrantResolution createdGrants =
        fixture.grants.findActiveGrantsForActor(
            created.account.actorId);
    assert(createdGrants.available);
    assert(createdGrants.grants.empty());

    const HumanAccountCreateIdempotencyLookupResult binding =
        fixture.creation.find(
            "actor-request-admin",
            "idem-account-create-1");
    assert(binding.status ==
        HumanAccountCreateIdempotencyStatus::ok);
    assert(binding.record.accountId == created.account.accountId);
    assert(binding.record.loginName == "alice");
    assert(binding.record.displayName == "Alice Viewer");

    const std::string originalVerifierHash = verifier->passwordHash;

    const HumanAccountCreationResult replayed =
        fixture.service.create(requestFor(
            "idem-account-create-1",
            "alice",
            "a-different-password-must-not-rotate",
            "Alice Viewer"));

    assert(replayed.status ==
        HumanAccountCreationStatus::replayed);
    assert(replayed.account.accountId == created.account.accountId);
    assert(replayed.account.actorId == created.account.actorId);
    assert(fixture.accounts.listAll().accounts.size() == 1U);

    const auto verifierAfterReplay =
        fixture.verifiers.findByLogin("alice");
    assert(verifierAfterReplay.has_value());
    assert(
        verifierAfterReplay->passwordHash ==
        originalVerifierHash);

    const HumanAccountCreationResult idemConflict =
        fixture.service.create(requestFor(
            "idem-account-create-1",
            "alice",
            "ignored-password",
            "Alice Renamed"));

    assert(idemConflict.status ==
        HumanAccountCreationStatus::idempotencyConflict);
    const auto afterIdemConflict =
        fixture.accounts.findByAccountId(created.account.accountId);
    assert(afterIdemConflict.status ==
        HumanAccountRepositoryStatus::ok);
    assert(afterIdemConflict.account.displayName == "Alice Viewer");
    assert(afterIdemConflict.account.revision == 1U);

    const HumanAccountCreationResult loginConflict =
        fixture.service.create(requestFor(
            "idem-account-create-2",
            "alice",
            "another-password",
            "Second Alice"));

    assert(loginConflict.status ==
        HumanAccountCreationStatus::loginConflict);
    assert(fixture.accounts.listAll().accounts.size() == 1U);
    assert(
        fixture.creation.find(
            "actor-request-admin",
            "idem-account-create-2").status ==
        HumanAccountCreateIdempotencyStatus::notFound);

    const HumanAccountCreationResult invalid =
        fixture.service.create(requestFor(
            "",
            "bob",
            "password",
            "Bob"));
    assert(invalid.status ==
        HumanAccountCreationStatus::invalidRequest);
    assert(!fixture.verifiers.findByLogin("bob").has_value());

    const auto events = fixture.accountability.listAll();
    assert(events.size() == 4U);

    std::size_t createdEvents = 0U;
    std::size_t replayEvents = 0U;
    std::size_t idempotencyConflictEvents = 0U;
    std::size_t loginConflictEvents = 0U;

    for (const AccountabilityEvent& event : events)
    {
        assert(event.actorId == "actor-request-admin");
        assert(event.permission == "accounts.create");
        assert(event.backendId == "*");
        assert(event.action == "human-account.create");
        assert(event.eventType == "security.human-account.create");

        if (event.reasonCode == "account_created")
            ++createdEvents;
        else if (event.reasonCode == "account_create_replayed")
            ++replayEvents;
        else if (event.reasonCode == "idempotency_conflict")
            ++idempotencyConflictEvents;
        else if (event.reasonCode == "login_name_conflict")
            ++loginConflictEvents;
    }

    assert(createdEvents == 1U);
    assert(replayEvents == 1U);
    assert(idempotencyConflictEvents == 1U);
    assert(loginConflictEvents == 1U);

    return 0;
}

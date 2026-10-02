#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "HumanAccountGrantAdministrationService.h"
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
struct Fixture
{
    Database database;
    AccountabilityEventRepository accountability;
    SecurityIdentityRepository identities;
    SecurityIdentityProvisioningRepository provisioning;
    HumanAccountRepository accounts;
    SecurityPermissionGrantRepository grants;
    unsigned char entropyCounter = 1U;
    HumanAccountGrantAdministrationService service;

    Fixture()
        : accountability(database),
          identities(database),
          provisioning(database),
          accounts(database),
          grants(database),
          service(
              database,
              accounts,
              identities,
              grants,
              accountability,
              [this](unsigned char* output, std::size_t size)
              {
                  for (std::size_t index = 0U; index < size; ++index)
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

        auto lease = database.acquireTransactionLease();
        assert(database.execute("BEGIN IMMEDIATE;"));
        assert(provisioning.ensureHumanCredentialInActiveTransaction(
            "actor-user-1",
            "User One",
            "credential-user-1",
            "human-password"));
        assert(accounts.ensureAccountInActiveTransaction(
            "account-user-1",
            "actor-user-1",
            "User One"));
        assert(database.execute("COMMIT;"));
    }
};

HumanAccountGrantAdministrationContext context(
    const std::string& requestId)
{
    HumanAccountGrantAdministrationContext value;
    value.actorId = "actor-admin-1";
    value.actorType = ActorType::User;
    value.requestId = requestId;
    value.correlationId = "correlation-mu7";
    return value;
}

bool contains(
    const HumanAccountGrantResource& resource,
    const std::string& backendId)
{
    return std::any_of(
        resource.grants.begin(),
        resource.grants.end(),
        [&](const PermissionGrant& grant)
        {
            return grant.permission == "channels.view" &&
                grant.backendId == backendId;
        });
}
}

int main()
{
    Fixture fixture;

    const auto initial =
        fixture.service.read("account-user-1");
    assert(initial.status ==
        HumanAccountGrantAdministrationStatus::success);
    assert(initial.resource.accountId == "account-user-1");
    assert(initial.resource.actorId == "actor-user-1");
    assert(initial.resource.grants.empty());
    assert(!initial.resource.resourceRevision.empty());

    const auto unsupportedWildcard =
        fixture.service.setGrant(
            context("request-wildcard"),
            "account-user-1",
            initial.resource.resourceRevision,
            "channels.view",
            "*",
            true);
    assert(unsupportedWildcard.status ==
        HumanAccountGrantAdministrationStatus::invalidRequest);

    const auto unsupportedPermission =
        fixture.service.setGrant(
            context("request-unsupported"),
            "account-user-1",
            initial.resource.resourceRevision,
            "timers.create",
            "default",
            true);
    assert(unsupportedPermission.status ==
        HumanAccountGrantAdministrationStatus::invalidRequest);

    const auto ensured =
        fixture.service.setGrant(
            context("request-ensure"),
            "account-user-1",
            initial.resource.resourceRevision,
            "channels.view",
            "default",
            true);
    assert(ensured.status ==
        HumanAccountGrantAdministrationStatus::success);
    assert(contains(ensured.resource, "default"));
    assert(ensured.resource.resourceRevision !=
        initial.resource.resourceRevision);

    const auto stored =
        fixture.grants.findActiveGrantsForActor("actor-user-1");
    assert(stored.available);
    assert(std::count_if(
        stored.grants.begin(),
        stored.grants.end(),
        [](const PermissionGrant& grant)
        {
            return grant.permission == "channels.view" &&
                grant.backendId == "default";
        }) == 1);

    const auto replayEnsureWithStaleRevision =
        fixture.service.setGrant(
            context("request-replay-ensure"),
            "account-user-1",
            initial.resource.resourceRevision,
            "channels.view",
            "default",
            true);
    assert(replayEnsureWithStaleRevision.status ==
        HumanAccountGrantAdministrationStatus::success);
    assert(replayEnsureWithStaleRevision.resource.resourceRevision ==
        ensured.resource.resourceRevision);

    const auto staleDifferentTuple =
        fixture.service.setGrant(
            context("request-stale-other"),
            "account-user-1",
            initial.resource.resourceRevision,
            "channels.view",
            "secondary",
            true);
    assert(staleDifferentTuple.status ==
        HumanAccountGrantAdministrationStatus::revisionConflict);
    assert(!contains(staleDifferentTuple.resource, "secondary"));

    const auto ensuredSecondary =
        fixture.service.setGrant(
            context("request-ensure-secondary"),
            "account-user-1",
            ensured.resource.resourceRevision,
            "channels.view",
            "secondary",
            true);
    assert(ensuredSecondary.status ==
        HumanAccountGrantAdministrationStatus::success);
    assert(contains(ensuredSecondary.resource, "default"));
    assert(contains(ensuredSecondary.resource, "secondary"));

    const auto revoked =
        fixture.service.setGrant(
            context("request-revoke"),
            "account-user-1",
            ensuredSecondary.resource.resourceRevision,
            "channels.view",
            "default",
            false);
    assert(revoked.status ==
        HumanAccountGrantAdministrationStatus::success);
    assert(!contains(revoked.resource, "default"));
    assert(contains(revoked.resource, "secondary"));

    const auto replayRevokeWithStaleRevision =
        fixture.service.setGrant(
            context("request-replay-revoke"),
            "account-user-1",
            ensuredSecondary.resource.resourceRevision,
            "channels.view",
            "default",
            false);
    assert(replayRevokeWithStaleRevision.status ==
        HumanAccountGrantAdministrationStatus::success);
    assert(replayRevokeWithStaleRevision.resource.resourceRevision ==
        revoked.resource.resourceRevision);

    const auto events = fixture.accountability.listAll();
    assert(std::count_if(
        events.begin(),
        events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.eventType ==
                "security.human-account.grant-administration" &&
                event.permission == "accounts.grants.modify" &&
                event.reasonCode == "grant_set_revision_conflict";
        }) == 1);
    assert(std::count_if(
        events.begin(),
        events.end(),
        [](const AccountabilityEvent& event)
        {
            return event.reasonCode == "grant_ensured" ||
                event.reasonCode == "grant_revoked" ||
                event.reasonCode == "grant_already_active" ||
                event.reasonCode == "grant_already_revoked";
        }) == 5);

    return 0;
}

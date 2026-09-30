#include "Database.h"
#include "FirstAdminBootstrapRepository.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <string>

namespace
{
const std::string VerifierHash =
    "$6$testsalt$qzmynZ3SU0S5D.QBAsFplf6HVa.jpeEdx88KlHvhGfddFSPHoEWMArwiVQ1PLzZDrJJ9Vs/zKBgHPMSwmFddx.";

FirstAdminBootstrapRegistration registration(
    const std::string& bootstrapId,
    const std::string& expiresAt)
{
    FirstAdminBootstrapRegistration value;
    value.bootstrapId = bootstrapId;
    value.verifierHash = VerifierHash;
    value.expiresAt = expiresAt;
    return value;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    SecurityIdentityRepository identityRepository(database);
    HumanAccountRepository accountRepository(database);
    SecurityPermissionGrantRepository grantRepository(database);
    FirstAdminBootstrapRepository bootstrapRepository(database);

    assert(identityRepository.ensureSchema());
    assert(accountRepository.ensureSchema());
    assert(grantRepository.ensureSchema());
    assert(bootstrapRepository.ensureSchema());

    assert(
        bootstrapRepository.claimState() ==
        FirstAdminClaimState::unclaimed);

    FirstAdminBootstrapRegistration invalid =
        registration("bootstrap-invalid", "2099-01-01 00:00:00");
    invalid.verifierHash = "plain-text-bootstrap-secret";
    assert(
        bootstrapRepository.registerBootstrap(invalid) ==
        FirstAdminBootstrapStatus::invalid);

    assert(
        bootstrapRepository.registerBootstrap(
            registration(
                "bootstrap-expired",
                "2000-01-01 00:00:00")) ==
        FirstAdminBootstrapStatus::expired);

    assert(
        bootstrapRepository.registerBootstrap(
            registration(
                "bootstrap-first",
                "2099-01-01 00:00:00")) ==
        FirstAdminBootstrapStatus::ok);

    assert(
        bootstrapRepository.registerBootstrap(
            registration(
                "bootstrap-conflict",
                "2099-01-02 00:00:00")) ==
        FirstAdminBootstrapStatus::conflict);

    auto first =
        bootstrapRepository.findById("bootstrap-first");
    assert(first.status == FirstAdminBootstrapStatus::ok);
    assert(first.bootstrap.bootstrapId == "bootstrap-first");
    assert(first.bootstrap.verifierHash == VerifierHash);
    assert(first.bootstrap.expiresAt == "2099-01-01 00:00:00");
    assert(!first.bootstrap.expired);
    assert(!first.bootstrap.consumed);
    assert(!first.bootstrap.invalidated);

    assert(
        bootstrapRepository.consumeInActiveTransaction(
            "bootstrap-first") ==
        FirstAdminBootstrapStatus::transactionRequired);

    assert(database.execute("BEGIN IMMEDIATE;"));
    assert(
        bootstrapRepository.consumeInActiveTransaction(
            "bootstrap-first") ==
        FirstAdminBootstrapStatus::ok);
    assert(database.execute("ROLLBACK;"));

    first = bootstrapRepository.findById("bootstrap-first");
    assert(first.status == FirstAdminBootstrapStatus::ok);
    assert(!first.bootstrap.consumed);

    assert(database.execute("BEGIN IMMEDIATE;"));
    assert(
        bootstrapRepository.consumeInActiveTransaction(
            "bootstrap-first") ==
        FirstAdminBootstrapStatus::ok);
    assert(database.execute("COMMIT;"));

    first = bootstrapRepository.findById("bootstrap-first");
    assert(first.status == FirstAdminBootstrapStatus::consumed);
    assert(first.bootstrap.consumed);
    assert(!first.bootstrap.consumedAt.empty());

    assert(
        bootstrapRepository.registerBootstrap(
            registration(
                "bootstrap-second",
                "2099-01-02 00:00:00")) ==
        FirstAdminBootstrapStatus::ok);

    assert(database.execute("BEGIN IMMEDIATE;"));
    assert(
        bootstrapRepository.invalidateInActiveTransaction(
            "bootstrap-second") ==
        FirstAdminBootstrapStatus::ok);
    assert(database.execute("COMMIT;"));

    const auto second =
        bootstrapRepository.findById("bootstrap-second");
    assert(second.status == FirstAdminBootstrapStatus::invalidated);
    assert(second.bootstrap.invalidated);
    assert(!second.bootstrap.invalidatedAt.empty());

    assert(database.execute(
        "INSERT INTO security_actors "
        "(actor_id, actor_type, display_name) "
        "VALUES ('first-admin-actor', 'user', 'First administrator');"));
    assert(database.execute(
        "INSERT INTO security_human_accounts "
        "(account_id, actor_id, display_name) "
        "VALUES ('first-admin-account', 'first-admin-actor', "
        "'First administrator');"));
    assert(grantRepository.ensureGrant(
        "first-admin-actor",
        "role.admin",
        "*"));

    assert(
        bootstrapRepository.claimState() ==
        FirstAdminClaimState::claimed);

    assert(
        bootstrapRepository.registerBootstrap(
            registration(
                "bootstrap-after-claim",
                "2099-01-03 00:00:00")) ==
        FirstAdminBootstrapStatus::claimed);

    const auto fenced =
        bootstrapRepository.findById("bootstrap-first");
    assert(fenced.status == FirstAdminBootstrapStatus::claimed);
    assert(fenced.bootstrap.verifierHash.empty());

    assert(database.execute("BEGIN IMMEDIATE;"));
    assert(
        bootstrapRepository.consumeInActiveTransaction(
            "bootstrap-first") ==
        FirstAdminBootstrapStatus::claimed);
    assert(database.execute("ROLLBACK;"));

    return 0;
}

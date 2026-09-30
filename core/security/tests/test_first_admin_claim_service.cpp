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
#include <sqlite3.h>

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

std::string bootstrapConsumedAt(
    Database& database,
    const std::string& bootstrapId)
{
    sqlite3_stmt* statement = nullptr;
    assert(
        sqlite3_prepare_v2(
            database.handle(),
            "SELECT consumed_at "
            "FROM security_first_admin_bootstrap_issuances "
            "WHERE bootstrap_id = ?;",
            -1,
            &statement,
            nullptr) == SQLITE_OK);
    assert(
        sqlite3_bind_text(
            statement,
            1,
            bootstrapId.c_str(),
            -1,
            SQLITE_TRANSIENT) == SQLITE_OK);

    std::string result;
    assert(sqlite3_step(statement) == SQLITE_ROW);
    const unsigned char* text =
        sqlite3_column_text(statement, 0);
    if (text != nullptr)
    {
        result =
            reinterpret_cast<const char*>(text);
    }
    sqlite3_finalize(statement);
    return result;
}

int tableCount(Database& database, const char* table)
{
    const std::string sql =
        std::string("SELECT COUNT(*) FROM ") +
        table +
        ";";
    sqlite3_stmt* statement = nullptr;
    assert(
        sqlite3_prepare_v2(
            database.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_ROW);
    const int count = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    return count;
}
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
            !bootstrapConsumedAt(
                fixture.database,
                "bootstrap-success").empty());
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
        assert(
            bootstrapConsumedAt(
                fixture.database,
                "bootstrap-rejected").empty());
        assert(fixture.accounts.listAll().accounts.empty());
        assert(
            fixture.bootstraps.claimState() ==
            FirstAdminClaimState::unclaimed);
    }

    {
        Fixture fixture;
        fixture.registerBootstrap("bootstrap-rollback");
        assert(
            fixture.database.execute(
                "DROP TABLE accountability_events;"));

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
        assert(
            bootstrapConsumedAt(
                fixture.database,
                "bootstrap-rollback").empty());
        assert(fixture.accounts.listAll().accounts.empty());
        assert(
            tableCount(
                fixture.database,
                "security_actors") == 0);
        assert(
            tableCount(
                fixture.database,
                "security_credentials") == 0);
        assert(
            tableCount(
                fixture.database,
                "security_basic_credential_verifiers") == 0);
        assert(
            tableCount(
                fixture.database,
                "security_actor_permission_grants") == 0);
        assert(
            fixture.bootstraps.claimState() ==
            FirstAdminClaimState::unclaimed);
    }

    return 0;
}

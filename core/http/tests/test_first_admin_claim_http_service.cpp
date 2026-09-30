#include "AccountabilityEventRepository.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "FirstAdminBootstrapRepository.h"
#include "FirstAdminClaimHttpService.h"
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
#include <memory>
#include <string>

namespace
{
const std::string SetupSecret =
    "bootstrap-secret-0123456789abcdef";
const std::string Password =
    "first-admin-browser-password";

std::string bootstrapVerifier(const std::string& secret)
{
    crypt_data data{};
    char* encoded = crypt_r(
        secret.c_str(),
        "$6$rounds=10000$browserclaim$",
        &data);
    assert(encoded != nullptr);
    return encoded;
}

FirstAdminClaimService::EntropySource entropy(
    unsigned char start)
{
    return [value = start](
               unsigned char* output,
               std::size_t size) mutable
    {
        if (output == nullptr || size == 0U)
        {
            return false;
        }
        for (std::size_t index = 0; index < size; ++index)
        {
            output[index] = value++;
        }
        return true;
    };
}

std::string body(
    const std::string& bootstrapId,
    const std::string& setupSecret = SetupSecret)
{
    return
        "{\"bootstrapId\":\"" + bootstrapId +
        "\",\"setupSecret\":\"" + setupSecret +
        "\",\"loginName\":\"admin\","
        "\"password\":\"" + Password +
        "\",\"displayName\":\"First administrator\"}";
}

HttpServerRequest request(
    const std::string& bootstrapId,
    const std::string& setupSecret = SetupSecret)
{
    HttpServerRequest result;
    result.method = "POST";
    result.path = "/api/security/first-admin/claim";
    result.headers["Content-Type"] =
        "application/json; charset=utf-8";
    result.headers["X-Request-ID"] =
        "request-first-admin-browser-claim";
    result.body = body(bootstrapId, setupSecret);
    return result;
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
    std::unique_ptr<FirstAdminClaimService> claimService;
    std::unique_ptr<FirstAdminClaimHttpService> http;

    explicit Fixture(unsigned char entropyStart = 0x10)
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

        claimService =
            std::make_unique<FirstAdminClaimService>(
                database,
                bootstraps,
                provisioning,
                accounts,
                verifiers,
                grants,
                accountability,
                entropy(entropyStart),
                []
                {
                    return std::chrono::system_clock::time_point(
                        std::chrono::seconds(4070908800));
                });
        http =
            std::make_unique<FirstAdminClaimHttpService>(
                *claimService);
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

    void consumeBootstrap(const std::string& bootstrapId)
    {
        auto lease = database.acquireTransactionLease();
        assert(database.execute("BEGIN IMMEDIATE;"));
        assert(
            bootstraps.consumeInActiveTransaction(bootstrapId) ==
            FirstAdminBootstrapStatus::ok);
        assert(database.execute("COMMIT;"));
    }

    void invalidateBootstrap(const std::string& bootstrapId)
    {
        auto lease = database.acquireTransactionLease();
        assert(database.execute("BEGIN IMMEDIATE;"));
        assert(
            bootstraps.invalidateInActiveTransaction(bootstrapId) ==
            FirstAdminBootstrapStatus::ok);
        assert(database.execute("COMMIT;"));
    }
};

void assertNoSecrets(
    const HttpServerResponse& response)
{
    assert(response.body.find(SetupSecret) == std::string::npos);
    assert(response.body.find(Password) == std::string::npos);
    for (const auto& header : response.headers)
    {
        assert(header.second.find(SetupSecret) == std::string::npos);
        assert(header.second.find(Password) == std::string::npos);
    }
    assert(response.headers.count("Set-Cookie") == 0U);
    assert(response.body.find("csrf") == std::string::npos);
}
}

int main()
{
    {
        Fixture fixture;
        fixture.registerBootstrap("bootstrap-browser-success");

        HttpServerRequest claim =
            request("bootstrap-browser-success");
        const HttpServerResponse response =
            fixture.http->handle(claim);

        assert(response.statusCode == 201);
        assert(
            response.headers.at("X-Request-ID") ==
            "request-first-admin-browser-claim");
        assert(
            response.headers.count("X-Correlation-ID") == 0U);
        assert(
            response.headers.at("Cache-Control") ==
            "no-store");
        assert(
            response.headers.at("X-Content-Type-Options") ==
            "nosniff");
        assert(
            response.body.find("\"status\":\"claimed\"") !=
            std::string::npos);
        assertNoSecrets(response);

        const HumanAccountListResult accounts =
            fixture.accounts.listAll();
        assert(accounts.status == HumanAccountRepositoryStatus::ok);
        assert(accounts.accounts.size() == 1U);
        assert(accounts.accounts[0].active);
        assert(
            accounts.accounts[0].displayName ==
            "First administrator");

        const auto actor =
            fixture.identities.findActor(
                accounts.accounts[0].actorId);
        assert(actor.has_value());
        assert(actor->type == ActorType::User);
        assert(actor->active);
        assert(!actor->revoked);

        const auto verifier =
            fixture.verifiers.findByLogin("admin");
        assert(verifier.has_value());
        assert(verifier->passwordHash != Password);
        assert(
            verifier->passwordHash.rfind("$y$", 0) == 0);

        const auto grants =
            fixture.grants.findActiveGrantsForActor(
                accounts.accounts[0].actorId);
        assert(grants.available);
        assert(grants.grants.size() == 1U);
        assert(grants.grants[0].permission == "role.admin");
        assert(grants.grants[0].backendId == "*");

        const auto events = fixture.accountability.listAll();
        assert(events.size() == 1U);
        assert(
            events[0].eventType ==
            "security.first-admin.claim");

        const HttpServerResponse replay =
            fixture.http->handle(claim);
        assert(replay.statusCode == 409);
        assert(
            replay.body.find("server_already_claimed") !=
            std::string::npos);
        assertNoSecrets(replay);

        const HumanAccountListResult replayAccounts =
            fixture.accounts.listAll();
        assert(replayAccounts.accounts.size() == 1U);
    }

    {
        Fixture fixture(0x30);
        fixture.registerBootstrap("bootstrap-browser-wrong-secret");
        const HttpServerResponse response =
            fixture.http->handle(
                request(
                    "bootstrap-browser-wrong-secret",
                    "wrong-bootstrap-secret-0123456789"));
        assert(response.statusCode == 401);
        assert(
            response.body.find("invalid_bootstrap_proof") !=
            std::string::npos);
        assert(fixture.accounts.listAll().accounts.empty());
        assertNoSecrets(response);
    }

    {
        Fixture fixture(0x40);
        fixture.registerBootstrap("bootstrap-browser-expired");
        assert(
            fixture.database.execute(
                "UPDATE security_first_admin_bootstrap_issuances "
                "SET expires_at = '2000-01-01 00:00:00' "
                "WHERE bootstrap_id = 'bootstrap-browser-expired';"));
        const HttpServerResponse response =
            fixture.http->handle(
                request("bootstrap-browser-expired"));
        assert(response.statusCode == 410);
        assert(
            response.body.find("bootstrap_expired") !=
            std::string::npos);
        assert(fixture.accounts.listAll().accounts.empty());
        assertNoSecrets(response);
    }

    {
        Fixture fixture(0x50);
        fixture.registerBootstrap("bootstrap-browser-consumed");
        fixture.consumeBootstrap("bootstrap-browser-consumed");
        const HttpServerResponse response =
            fixture.http->handle(
                request("bootstrap-browser-consumed"));
        assert(response.statusCode == 409);
        assert(
            response.body.find("bootstrap_consumed") !=
            std::string::npos);
        assert(fixture.accounts.listAll().accounts.empty());
        assertNoSecrets(response);
    }

    {
        Fixture fixture(0x60);
        fixture.registerBootstrap("bootstrap-browser-invalidated");
        fixture.invalidateBootstrap(
            "bootstrap-browser-invalidated");
        const HttpServerResponse response =
            fixture.http->handle(
                request("bootstrap-browser-invalidated"));
        assert(response.statusCode == 410);
        assert(
            response.body.find("bootstrap_invalidated") !=
            std::string::npos);
        assert(fixture.accounts.listAll().accounts.empty());
        assertNoSecrets(response);
    }

    {
        Fixture fixture(0x70);
        fixture.registerBootstrap("bootstrap-browser-missing-proof");
        HttpServerRequest missing =
            request("bootstrap-browser-missing-proof");
        missing.body =
            "{\"bootstrapId\":\"bootstrap-browser-missing-proof\","
            "\"loginName\":\"admin\","
            "\"password\":\"first-admin-browser-password\","
            "\"displayName\":\"First administrator\"}";
        const HttpServerResponse response =
            fixture.http->handle(missing);
        assert(response.statusCode == 400);
        assert(fixture.accounts.listAll().accounts.empty());
        assertNoSecrets(response);
    }

    {
        Fixture fixture(0x80);
        fixture.registerBootstrap("bootstrap-browser-boundary");

        HttpServerRequest other =
            request("bootstrap-browser-boundary");
        other.path = "/api/v1/accounts";
        other.headers["Authorization"] =
            "Bootstrap " + SetupSecret;
        assert(!fixture.http->handles(other));

        HttpServerRequest session =
            request("bootstrap-browser-boundary");
        session.path = "/api/security/browser-sessions";
        assert(!fixture.http->handles(session));

        HttpServerRequest wrongMethod =
            request("bootstrap-browser-boundary");
        wrongMethod.method = "GET";
        const HttpServerResponse methodResponse =
            fixture.http->handle(wrongMethod);
        assert(methodResponse.statusCode == 405);
        assert(
            methodResponse.headers.at("Allow") == "POST");
        assertNoSecrets(methodResponse);

        HttpServerRequest wrongType =
            request("bootstrap-browser-boundary");
        wrongType.headers["Content-Type"] = "text/plain";
        const HttpServerResponse typeResponse =
            fixture.http->handle(wrongType);
        assert(typeResponse.statusCode == 415);
        assertNoSecrets(typeResponse);
    }

    return 0;
}

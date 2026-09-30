#include "AccountabilityEventRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionHttpGate.h"
#include "BrowserSessionHttpService.h"
#include "BrowserSessionIssuanceService.h"
#include "BrowserSessionLifecycleService.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "HumanAccountRepository.h"
#include "HumanPasswordBrowserAuthenticator.h"
#include "PersistentIdentityResolver.h"
#include "SecurityConfiguration.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <crypt.h>

#include <cassert>
#include <string>

namespace
{
const std::string HumanPassword = "human-password";
const std::string HumanAuthorization =
    "Basic YWRtaW46aHVtYW4tcGFzc3dvcmQ=";
const std::string LegacyAuthorization =
    "Basic YWRtaW46dmRyLXN1aXRl";

std::string passwordHash(const std::string& password)
{
    crypt_data data{};
    char* encoded = crypt_r(
        password.c_str(),
        "$6$humanpassword$",
        &data);
    assert(encoded != nullptr);
    return encoded;
}

HttpServerRequest loginRequest(
    const std::string& authorization,
    const std::string& requestId)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = "/api/security/browser-sessions";
    request.headers["Authorization"] = authorization;
    request.headers["X-Request-ID"] = requestId;
    return request;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    AccountabilityEventRepository accountability(database);
    SecurityIdentityRepository identities(database);
    SecurityIdentityProvisioningRepository provisioning(database);
    HumanAccountRepository accounts(database);
    CredentialVerifierRepository verifiers(database);
    SecurityPermissionGrantRepository grants(database);
    BrowserSessionCredentialRepository browserCredentials(database);

    assert(accountability.ensureSchema());
    assert(identities.ensureSchema());
    assert(accounts.ensureSchema());
    assert(verifiers.ensureSchema());
    assert(grants.ensureSchema());
    assert(browserCredentials.ensureSchema());

    {
        auto lease = database.acquireTransactionLease();
        assert(database.execute("BEGIN IMMEDIATE;"));
        assert(provisioning.ensureHumanCredentialInActiveTransaction(
            "actor-human-admin",
            "First administrator",
            "credential-human-admin",
            "human-password"));
        assert(accounts.ensureAccountInActiveTransaction(
            "account-human-admin",
            "actor-human-admin",
            "First administrator"));
        assert(database.execute("COMMIT;"));
    }

    assert(verifiers.ensureVerifier(
        "credential-human-admin",
        "admin",
        passwordHash(HumanPassword)));

    const std::string loginDeviceId =
        HumanPasswordBrowserAuthenticator::loginDeviceId(
            "account-human-admin");
    assert(loginDeviceId == "human-browser-account-human-admin");
    assert(!identities.findDevice(loginDeviceId).has_value());

    HumanPasswordBrowserAuthenticator humanAuthenticator(
        verifiers,
        identities,
        accounts,
        provisioning);

    const auto wrongHumanPassword =
        humanAuthenticator.authenticate(
            {{"Authorization", LegacyAuthorization}},
            "request-human-wrong",
            "");
    assert(
        wrongHumanPassword.authenticationState ==
        AuthenticationState::Invalid);
    assert(!identities.findDevice(loginDeviceId).has_value());

    const auto humanContext =
        humanAuthenticator.authenticate(
            {{"Authorization", HumanAuthorization}},
            "request-human-correct",
            "");
    assert(humanContext.authenticated());
    assert(humanContext.actor.actorId == "actor-human-admin");
    assert(humanContext.actor.type == ActorType::User);
    assert(humanContext.device.has_value());
    assert(humanContext.device->deviceId == loginDeviceId);
    assert(humanContext.credential.has_value());
    assert(
        humanContext.credential->credentialId ==
        "credential-human-admin");

    const auto storedDevice =
        identities.findDevice(loginDeviceId);
    assert(storedDevice.has_value());
    assert(storedDevice->actorId == "actor-human-admin");
    assert(storedDevice->active);
    assert(!storedDevice->revoked);

    const auto repeatedContext =
        humanAuthenticator.authenticate(
            {{"Authorization", HumanAuthorization}},
            "request-human-repeat",
            "");
    assert(repeatedContext.authenticated());
    assert(repeatedContext.device.has_value());
    assert(
        repeatedContext.device->deviceId ==
        loginDeviceId);

    PersistentIdentityResolver resolver(identities);
    SecurityConfiguration configuration;

    BrowserSessionHttpGate gate(
        configuration,
        accountability,
        browserCredentials,
        grants,
        &resolver,
        nullptr,
        &humanAuthenticator);

    const BrowserSessionGateDecision legacyCollision =
        gate.evaluate(loginRequest(
            LegacyAuthorization,
            "request-human-strict-precedence"));
    assert(!legacyCollision.allowed);
    assert(legacyCollision.rejection.statusCode == 401);
    assert(
        legacyCollision.rejection.body.find(
            "invalid_credentials") != std::string::npos);

    const BrowserSessionGateDecision login =
        gate.evaluate(loginRequest(
            HumanAuthorization,
            "request-human-session"));
    assert(login.allowed);
    assert(login.login);
    assert(login.context.authenticated());
    assert(login.context.actor.actorId == "actor-human-admin");
    assert(login.context.device.has_value());
    assert(login.context.device->deviceId == loginDeviceId);
    assert(login.context.credential.has_value());
    assert(
        login.context.credential->credentialId ==
        "credential-human-admin");

    BrowserSessionIssuanceService issuance(
        database,
        identities,
        browserCredentials);
    BrowserSessionLifecycleService lifecycle(
        database,
        identities,
        browserCredentials);
    BrowserSessionHttpService http(
        issuance,
        lifecycle,
        accountability);

    const HttpServerResponse response =
        http.login(login.context);
    assert(response.statusCode == 200);
    assert(response.headers.count("Set-Cookie") == 1U);
    assert(
        response.headers.at("Set-Cookie").find(
            "HttpOnly") != std::string::npos);
    assert(
        response.headers.at("Set-Cookie").find(
            "Secure") != std::string::npos);
    assert(
        response.body.find(""csrfToken"") !=
        std::string::npos);
    assert(
        response.body.find(HumanPassword) ==
        std::string::npos);

    const std::string cookie =
        response.headers.at("Set-Cookie");
    const std::string prefix = "vdr_suite_session=";
    const std::size_t start = cookie.find(prefix);
    assert(start != std::string::npos);
    const std::size_t valueStart = start + prefix.size();
    const std::size_t dot = cookie.find('.', valueStart);
    assert(dot != std::string::npos);
    const std::string tokenId =
        cookie.substr(valueStart, dot - valueStart);

    const auto browser =
        browserCredentials.findByTokenId(tokenId);
    assert(browser.has_value());
    assert(browser->actorId == "actor-human-admin");
    assert(browser->deviceId == loginDeviceId);
    assert(
        browser->issuedFromCredentialId ==
        "credential-human-admin");

    const auto events = accountability.listAll();
    bool sawAllowed = false;
    bool sawSucceeded = false;
    for (const auto& event : events)
    {
        if (event.action == "browser.session.issue" &&
            event.actorId == "actor-human-admin" &&
            event.deviceId == loginDeviceId &&
            event.eventType == "authorization.allowed")
        {
            sawAllowed = true;
        }
        if (event.action == "browser.session.issue" &&
            event.actorId == "actor-human-admin" &&
            event.deviceId == loginDeviceId &&
            event.eventType == "operation.succeeded")
        {
            sawSucceeded = true;
        }
    }
    assert(sawAllowed);
    assert(sawSucceeded);

    assert(identities.revokeDevice(loginDeviceId));
    const auto revokedContext =
        humanAuthenticator.authenticate(
            {{"Authorization", HumanAuthorization}},
            "request-human-revoked-device",
            "");
    assert(
        revokedContext.authenticationState ==
        AuthenticationState::Revoked);
    assert(revokedContext.device.has_value());
    assert(!revokedContext.device->active);

    return 0;
}

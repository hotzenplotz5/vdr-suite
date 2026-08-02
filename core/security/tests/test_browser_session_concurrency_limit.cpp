#include "AccountabilityEventRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionHttpService.h"
#include "BrowserSessionIssuanceService.h"
#include "BrowserSessionLifecycleService.h"
#include "Database.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace
{
BrowserSessionIssuanceService::EntropySource deterministicEntropy()
{
    return [value = static_cast<unsigned int>(1)](
               unsigned char* output,
               std::size_t size) mutable
    {
        if (output == nullptr || size == 0)
        {
            return false;
        }
        for (std::size_t index = 0; index < size; ++index)
        {
            output[index] = static_cast<unsigned char>(value & 0xffU);
            ++value;
        }
        return true;
    };
}

BrowserSessionIssuanceService::Clock fixedClock()
{
    return []
    {
        return std::chrono::system_clock::time_point(
            std::chrono::seconds(4070908800));
    };
}

bool eventContains(
    const AccountabilityEvent& event,
    const std::string& value)
{
    if (value.empty())
    {
        return false;
    }

    const std::vector<std::string> fields = {
        event.eventId,
        event.classes,
        event.eventType,
        event.severity,
        event.occurredAt,
        event.actorId,
        event.actorType,
        event.deviceId,
        event.sessionId,
        event.authenticationState,
        event.permission,
        event.backendId,
        event.operationId,
        event.requestId,
        event.correlationId,
        event.action,
        event.decision,
        event.reasonCode,
        event.outcome,
    };

    return std::any_of(
        fields.begin(),
        fields.end(),
        [&](const std::string& field)
        {
            return field.find(value) != std::string::npos;
        });
}

bool hasOutcome(
    const std::vector<AccountabilityEvent>& events,
    const std::string& reasonCode,
    const std::string& requestId)
{
    return std::any_of(
        events.begin(),
        events.end(),
        [&](const AccountabilityEvent& event)
        {
            return event.eventType == "operation.failed" &&
                event.permission == "session.issue.self" &&
                event.action == "browser.session.issue" &&
                event.reasonCode == reasonCode &&
                event.outcome == "failed" &&
                event.decision == "allowed" &&
                event.requestId == requestId;
        });
}

void provisionActor(
    SecurityIdentityProvisioningRepository& repository,
    const std::string& suffix)
{
    assert(repository.ensureIdentity(
        "user-" + suffix,
        ActorType::User,
        "User " + suffix,
        "device-" + suffix,
        "Device " + suffix,
        "source-session-" + suffix,
        "source-credential-" + suffix,
        "managed-basic"));
}

RequestSecurityContext basicContext(
    const std::string& suffix,
    const std::string& requestId)
{
    RequestSecurityContext context;
    context.actor = ActorIdentity{
        "user-" + suffix,
        ActorType::User,
        "User " + suffix,
        true};
    context.device = DeviceIdentity{
        "device-" + suffix,
        true};
    context.session = SessionIdentity{
        "source-session-" + suffix,
        true,
        false,
        false};
    context.credential = CredentialIdentity{
        "source-credential-" + suffix,
        true,
        false,
        false};
    context.authenticationState = AuthenticationState::Authenticated;
    context.requestId = requestId;
    context.correlationId = "correlation-" + suffix;
    return context;
}
}

int main()
{
    {
        Database database;
        assert(database.open(":memory:"));

        SecurityIdentityRepository identityRepository(database);
        assert(identityRepository.ensureSchema());

        SecurityIdentityProvisioningRepository provisioningRepository(database);
        provisionActor(provisioningRepository, "alpha");
        provisionActor(provisioningRepository, "beta");

        BrowserSessionCredentialRepository credentialRepository(database);
        assert(credentialRepository.ensureSchema());

        BrowserSessionIssuanceService issuanceService(
            database,
            identityRepository,
            credentialRepository,
            deterministicEntropy(),
            fixedClock());
        BrowserSessionLifecycleService lifecycleService(
            database,
            identityRepository,
            credentialRepository);

        BrowserSessionIssuanceRequest alpha;
        alpha.actorId = "user-alpha";
        alpha.deviceId = "device-alpha";
        alpha.issuedFromCredentialId = "source-credential-alpha";
        alpha.lifetimeSeconds = 3600;
        alpha.maximumActivePerActor = 1;

        const auto initiallyActive =
            credentialRepository.countEffectiveActiveByActorId(alpha.actorId);
        assert(initiallyActive.has_value());
        assert(*initiallyActive == 0);

        BrowserSessionIssuanceResult first =
            issuanceService.issueWithPolicy(alpha);
        assert(first.status == BrowserSessionIssuanceStatus::Issued);
        assert(first.session.has_value());

        const std::string firstSessionId = first.session->sessionId;
        const std::string firstCredentialId = first.session->credentialId;
        const std::string firstTokenId = first.session->tokenId;

        const auto oneActive =
            credentialRepository.countEffectiveActiveByActorId(alpha.actorId);
        assert(oneActive.has_value());
        assert(*oneActive == 1);

        BrowserSessionIssuanceResult denied =
            issuanceService.issueWithPolicy(alpha);
        assert(denied.status == BrowserSessionIssuanceStatus::LimitReached);
        assert(!denied.session.has_value());

        const auto stillOne =
            credentialRepository.countEffectiveActiveByActorId(alpha.actorId);
        assert(stillOne.has_value());
        assert(*stillOne == 1);

        const auto firstRow =
            credentialRepository.findByTokenId(firstTokenId);
        const auto firstSession =
            identityRepository.findSession(firstSessionId);
        const auto firstCredential =
            identityRepository.findCredential(firstCredentialId);
        assert(firstRow.has_value());
        assert(firstRow->active && !firstRow->expired && !firstRow->revoked);
        assert(firstSession.has_value());
        assert(firstSession->active &&
            !firstSession->expired &&
            !firstSession->revoked);
        assert(firstCredential.has_value());
        assert(firstCredential->active &&
            !firstCredential->expired &&
            !firstCredential->revoked);

        assert(lifecycleService.revoke(
            firstSessionId,
            firstCredentialId));
        const auto afterRevoke =
            credentialRepository.countEffectiveActiveByActorId(alpha.actorId);
        assert(afterRevoke.has_value());
        assert(*afterRevoke == 0);

        BrowserSessionIssuanceResult replacement =
            issuanceService.issueWithPolicy(alpha);
        assert(replacement.status == BrowserSessionIssuanceStatus::Issued);
        assert(replacement.session.has_value());

        assert(credentialRepository.setExpiry(
            replacement.session->sessionId,
            "2000-01-01 00:00:00"));
        const auto afterBrowserExpiry =
            credentialRepository.countEffectiveActiveByActorId(alpha.actorId);
        assert(afterBrowserExpiry.has_value());
        assert(*afterBrowserExpiry == 0);

        BrowserSessionIssuanceResult afterExpired =
            issuanceService.issueWithPolicy(alpha);
        assert(afterExpired.status == BrowserSessionIssuanceStatus::Issued);
        assert(afterExpired.session.has_value());

        BrowserSessionIssuanceRequest beta;
        beta.actorId = "user-beta";
        beta.deviceId = "device-beta";
        beta.issuedFromCredentialId = "source-credential-beta";
        beta.lifetimeSeconds = 3600;
        beta.maximumActivePerActor = 1;

        BrowserSessionIssuanceResult betaIssued =
            issuanceService.issueWithPolicy(beta);
        assert(betaIssued.status == BrowserSessionIssuanceStatus::Issued);
        assert(betaIssued.session.has_value());

        const auto alphaActive =
            credentialRepository.countEffectiveActiveByActorId(alpha.actorId);
        const auto betaActive =
            credentialRepository.countEffectiveActiveByActorId(beta.actorId);
        assert(alphaActive.has_value() && *alphaActive == 1);
        assert(betaActive.has_value() && *betaActive == 1);

        assert(identityRepository.revokeCredential(
            alpha.issuedFromCredentialId));
        const auto afterIssuerRevoke =
            credentialRepository.countEffectiveActiveByActorId(alpha.actorId);
        assert(afterIssuerRevoke.has_value());
        assert(*afterIssuerRevoke == 0);

        const auto rawAfterIssuerRevoke =
            credentialRepository.findBySessionId(
                afterExpired.session->sessionId);
        assert(rawAfterIssuerRevoke.has_value());
        assert(rawAfterIssuerRevoke->active);
        assert(!rawAfterIssuerRevoke->revoked);

        BrowserSessionIssuanceRequest unlimited = beta;
        unlimited.maximumActivePerActor = 0;
        assert(issuanceService.issue(unlimited).has_value());
        assert(issuanceService.issue(unlimited).has_value());
    }

    {
        Database database;
        assert(database.open(":memory:"));

        AccountabilityEventRepository accountabilityRepository(database);
        assert(accountabilityRepository.ensureSchema());

        SecurityIdentityRepository identityRepository(database);
        assert(identityRepository.ensureSchema());

        SecurityIdentityProvisioningRepository provisioningRepository(database);
        provisionActor(provisioningRepository, "http");

        BrowserSessionCredentialRepository credentialRepository(database);
        assert(credentialRepository.ensureSchema());

        BrowserSessionIssuanceService issuanceService(
            database,
            identityRepository,
            credentialRepository,
            deterministicEntropy(),
            fixedClock());
        BrowserSessionLifecycleService lifecycleService(
            database,
            identityRepository,
            credentialRepository);

        BrowserSessionLifetimeConfiguration lifetime;
        lifetime.seconds = 900;
        BrowserSessionConcurrencyConfiguration concurrency;
        concurrency.maximumActivePerActor = 1;

        BrowserSessionHttpService httpService(
            issuanceService,
            lifecycleService,
            accountabilityRepository,
            lifetime,
            concurrency);

        RequestSecurityContext firstContext =
            basicContext("http", "request-limit-first");
        const HttpServerResponse firstLogin =
            httpService.login(firstContext);
        assert(firstLogin.statusCode == 200);
        assert(firstLogin.headers.find("Set-Cookie") !=
            firstLogin.headers.end());

        const std::string firstCookie =
            firstLogin.headers.at("Set-Cookie");
        const std::string csrfMarker = "\"csrfToken\":\"";
        const std::size_t csrfBegin =
            firstLogin.body.find(csrfMarker);
        assert(csrfBegin != std::string::npos);
        const std::size_t csrfValueBegin = csrfBegin + csrfMarker.size();
        const std::size_t csrfEnd =
            firstLogin.body.find('"', csrfValueBegin);
        assert(csrfEnd != std::string::npos);
        const std::string csrfToken = firstLogin.body.substr(
            csrfValueBegin,
            csrfEnd - csrfValueBegin);
        assert(!csrfToken.empty());

        RequestSecurityContext secondContext =
            basicContext("http", "request-limit-second");
        const HttpServerResponse secondLogin =
            httpService.login(secondContext);
        assert(secondLogin.statusCode == 409);
        assert(secondLogin.headers.find("Set-Cookie") ==
            secondLogin.headers.end());
        assert(secondLogin.body.find("browser_session_limit_reached") !=
            std::string::npos);
        assert(secondLogin.body.find("request-limit-second") !=
            std::string::npos);

        const auto count =
            credentialRepository.countEffectiveActiveByActorId("user-http");
        assert(count.has_value());
        assert(*count == 1);

        const auto events = accountabilityRepository.listAll();
        assert(hasOutcome(
            events,
            "browser_session_limit_reached",
            "request-limit-second"));
        for (const AccountabilityEvent& event : events)
        {
            assert(!eventContains(event, firstCookie));
            assert(!eventContains(event, csrfToken));
        }

        BrowserSessionConcurrencyConfiguration invalidConcurrency;
        invalidConcurrency.configuredValueValid = false;
        BrowserSessionHttpService invalidService(
            issuanceService,
            lifecycleService,
            accountabilityRepository,
            lifetime,
            invalidConcurrency);

        RequestSecurityContext invalidContext =
            basicContext("http", "request-limit-invalid");
        const HttpServerResponse invalidResponse =
            invalidService.login(invalidContext);
        assert(invalidResponse.statusCode == 503);
        assert(invalidResponse.headers.find("Set-Cookie") ==
            invalidResponse.headers.end());
        assert(invalidResponse.body.find(
            "browser_session_limit_configuration_invalid") !=
            std::string::npos);
        assert(hasOutcome(
            accountabilityRepository.listAll(),
            "browser_session_limit_configuration_invalid",
            "request-limit-invalid"));
    }

    return 0;
}

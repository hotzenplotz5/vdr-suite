#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionIssuanceService.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "HumanAccountRecoveryService.h"
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

namespace
{
const std::string AccountId = "account-recovery-test";
const std::string ActorId = "actor-recovery-test";
const std::string CredentialId = "credential-recovery-test";
const std::string LoginName = "admin";
const std::string OldPassword = "old-password";
const std::string NewPassword = "new-password";

std::string passwordHash(
    const std::string& password,
    const std::string& salt)
{
    crypt_data data{};
    char* encoded = crypt_r(
        password.c_str(),
        salt.c_str(),
        &data);
    assert(encoded != nullptr);
    return encoded;
}

bool passwordMatches(
    const std::string& password,
    const std::string& hash)
{
    crypt_data data{};
    char* encoded =
        crypt_r(password.c_str(), hash.c_str(), &data);
    return encoded != nullptr &&
        std::string(encoded) == hash;
}

HumanAccountRecoveryService::EntropySource entropy(
    unsigned char seed)
{
    return [next = seed](
               unsigned char* output,
               std::size_t size) mutable
    {
        if (output == nullptr || size == 0)
        {
            return false;
        }
        for (std::size_t index = 0; index < size; ++index)
        {
            output[index] = next++;
        }
        return true;
    };
}

BrowserSessionIssuanceService::EntropySource sessionEntropy(
    unsigned char seed)
{
    return [next = seed](
               unsigned char* output,
               std::size_t size) mutable
    {
        if (output == nullptr || size == 0)
        {
            return false;
        }
        for (std::size_t index = 0; index < size; ++index)
        {
            output[index] = next++;
        }
        return true;
    };
}

struct IssuedIds
{
    std::string sessionId;
    std::string credentialId;
};

struct Fixture
{
    Database database;
    SecurityIdentityRepository identities;
    SecurityIdentityProvisioningRepository provisioning;
    HumanAccountRepository accounts;
    CredentialVerifierRepository verifiers;
    SecurityPermissionGrantRepository grants;
    BrowserSessionCredentialRepository browserCredentials;
    AccountabilityEventRepository accountability;

    Fixture()
        : identities(database),
          provisioning(database),
          accounts(database),
          verifiers(database),
          grants(database),
          browserCredentials(database),
          accountability(database)
    {
        assert(database.open(":memory:"));
        assert(identities.ensureSchema());
        assert(accounts.ensureSchema());
        assert(verifiers.ensureSchema());
        assert(grants.ensureSchema());
        assert(browserCredentials.ensureSchema());
        assert(accountability.ensureSchema());

        auto lease = database.acquireTransactionLease();
        assert(database.execute("BEGIN IMMEDIATE;"));
        assert(
            provisioning.ensureHumanCredentialInActiveTransaction(
                ActorId,
                "Recovery administrator",
                CredentialId,
                "human-password"));
        assert(
            accounts.ensureAccountInActiveTransaction(
                AccountId,
                ActorId,
                "Recovery administrator"));
        assert(
            verifiers.ensureVerifier(
                CredentialId,
                LoginName,
                passwordHash(
                    OldPassword,
                    "$6$rounds=10000$recovery-old$")));
        assert(
            grants.ensureGrant(
                ActorId,
                "role.admin",
                "*"));
        assert(database.execute("COMMIT;"));
    }

    HumanAccountRecoveryService recovery(unsigned char seed)
    {
        return HumanAccountRecoveryService(
            database,
            accounts,
            verifiers,
            identities,
            browserCredentials,
            accountability,
            entropy(seed),
            []
            {
                return std::chrono::system_clock::time_point(
                    std::chrono::seconds(4070908800));
            });
    }

    IssuedIds issueSession(unsigned char seed)
    {
        const std::string deviceId =
            "human-browser-" + AccountId;
        assert(
            provisioning.ensureHumanBrowserDevice(
                ActorId,
                deviceId,
                "Human Account browser login"));

        BrowserSessionIssuanceService issuance(
            database,
            identities,
            browserCredentials,
            sessionEntropy(seed),
            []
            {
                return std::chrono::system_clock::time_point(
                    std::chrono::seconds(4070908800));
            });

        BrowserSessionIssuanceRequest request;
        request.actorId = ActorId;
        request.deviceId = deviceId;
        request.issuedFromCredentialId = CredentialId;
        request.lifetimeSeconds = 3600;

        auto issued = issuance.issue(request);
        assert(issued.has_value());
        return IssuedIds{
            issued->sessionId,
            issued->credentialId};
    }
};

HumanAccountRecoveryRequest recoveryRequest(
    const std::string& accountId = AccountId,
    const std::string& loginName = LoginName)
{
    HumanAccountRecoveryRequest request;
    request.accountId = accountId;
    request.loginName = loginName;
    request.newPassword = NewPassword;
    request.requestId = "request-human-account-recovery";
    request.correlationId = "correlation-human-account-recovery";
    return request;
}

std::string eventFields(const AccountabilityEvent& event)
{
    return event.eventId +
        event.classes +
        event.eventType +
        event.severity +
        event.occurredAt +
        event.actorId +
        event.actorType +
        event.deviceId +
        event.sessionId +
        event.authenticationState +
        event.permission +
        event.backendId +
        event.operationId +
        event.requestId +
        event.correlationId +
        event.action +
        event.decision +
        event.reasonCode +
        event.outcome;
}
}

int main()
{
    {
        Fixture fixture;
        const IssuedIds firstSession =
            fixture.issueSession(0x10);
        const IssuedIds secondSession =
            fixture.issueSession(0x40);

        const auto before =
            fixture.verifiers.findByLogin(LoginName);
        assert(before.has_value());
        assert(passwordMatches(OldPassword, before->passwordHash));

        HumanAccountRecoveryService recovery =
            fixture.recovery(0x70);
        const HumanAccountRecoveryResult result =
            recovery.recover(recoveryRequest());

        assert(
            result.status ==
            HumanAccountRecoveryStatus::success);
        assert(result.accountId == AccountId);
        assert(result.actorId == ActorId);
        assert(result.credentialId == CredentialId);
        assert(result.revokedBrowserSessions == 2);

        const auto account =
            fixture.accounts.findByAccountId(AccountId);
        assert(account.status == HumanAccountRepositoryStatus::ok);
        assert(account.account.actorId == ActorId);
        assert(fixture.accounts.listAll().accounts.size() == 1);

        const auto actor =
            fixture.identities.findActor(ActorId);
        assert(actor.has_value());
        assert(actor->type == ActorType::User);
        assert(actor->active);
        assert(!actor->revoked);

        const auto sourceCredential =
            fixture.identities.findCredential(CredentialId);
        assert(sourceCredential.has_value());
        assert(
            sourceCredential->credentialType ==
            "human-password");
        assert(sourceCredential->active);
        assert(!sourceCredential->expired);
        assert(!sourceCredential->revoked);

        const auto after =
            fixture.verifiers.findByLogin(LoginName);
        assert(after.has_value());
        assert(after->credentialId == CredentialId);
        assert(after->passwordHash.rfind("$y$", 0) == 0);
        assert(!passwordMatches(OldPassword, after->passwordHash));
        assert(passwordMatches(NewPassword, after->passwordHash));

        const IssuedIds sessions[] = {
            firstSession,
            secondSession,
        };
        for (const IssuedIds& issued : sessions)
        {
            const auto browser =
                fixture.browserCredentials.findBySessionId(
                    issued.sessionId);
            assert(browser.has_value());
            assert(!browser->active);
            assert(browser->revoked);

            const auto session =
                fixture.identities.findSession(
                    issued.sessionId);
            assert(session.has_value());
            assert(!session->active);
            assert(session->revoked);

            const auto browserCredential =
                fixture.identities.findCredential(
                    issued.credentialId);
            assert(browserCredential.has_value());
            assert(!browserCredential->active);
            assert(browserCredential->revoked);
        }

        const auto activeByIssuer =
            fixture.browserCredentials.
                findByIssuedFromCredentialId(
                    CredentialId);
        assert(activeByIssuer.has_value());
        assert(activeByIssuer->empty());

        const auto grantResolution =
            fixture.grants.findActiveGrantsForActor(ActorId);
        assert(grantResolution.available);
        assert(grantResolution.grants.size() == 1);
        assert(
            grantResolution.grants[0].permission ==
            "role.admin");
        assert(grantResolution.grants[0].backendId == "*");

        const auto events = fixture.accountability.listAll();
        assert(events.size() == 1);
        assert(
            events[0].eventType ==
            "security.human-account.recovery");
        assert(events[0].actorId == ActorId);
        assert(events[0].actorType == "user");
        assert(events[0].authenticationState == "local-root");
        assert(events[0].decision == "allow");
        assert(
            events[0].reasonCode ==
            "local_root_password_reset");
        assert(events[0].outcome == "success");
        assert(
            eventFields(events[0]).find(NewPassword) ==
            std::string::npos);
        assert(
            eventFields(events[0]).find(OldPassword) ==
            std::string::npos);
    }

    {
        Fixture fixture;
        HumanAccountRecoveryService recovery =
            fixture.recovery(0x20);
        const HumanAccountRecoveryResult result =
            recovery.recover(
                recoveryRequest(
                    "account-does-not-exist",
                    LoginName));

        assert(
            result.status ==
            HumanAccountRecoveryStatus::accountNotFound);
        const auto verifier =
            fixture.verifiers.findByLogin(LoginName);
        assert(verifier.has_value());
        assert(passwordMatches(OldPassword, verifier->passwordHash));

        const auto events = fixture.accountability.listAll();
        assert(events.size() == 1);
        assert(events[0].decision == "deny");
        assert(events[0].reasonCode == "account_not_found");
        assert(events[0].outcome == "failed");
        assert(
            eventFields(events[0]).find(NewPassword) ==
            std::string::npos);
    }

    {
        Fixture fixture;
        assert(fixture.identities.revokeActor(ActorId));

        HumanAccountRecoveryService recovery =
            fixture.recovery(0x30);
        const HumanAccountRecoveryResult result =
            recovery.recover(recoveryRequest());

        assert(
            result.status ==
            HumanAccountRecoveryStatus::accountInactive);
        const auto verifier =
            fixture.verifiers.findByLogin(LoginName);
        assert(verifier.has_value());
        assert(passwordMatches(OldPassword, verifier->passwordHash));

        const auto events = fixture.accountability.listAll();
        assert(events.size() == 1);
        assert(events[0].actorId == ActorId);
        assert(events[0].decision == "deny");
        assert(events[0].reasonCode == "account_inactive");
    }

    {
        Fixture fixture;
        assert(
            fixture.provisioning.ensureTechnicalIdentity(
                "actor-technical",
                ActorType::Service,
                "Technical principal",
                "device-technical",
                "Technical device",
                "credential-technical",
                "managed-basic"));
        assert(
            fixture.verifiers.ensureVerifier(
                "credential-technical",
                "technical",
                passwordHash(
                    "technical-password",
                    "$6$rounds=10000$recovery-tech$")));

        HumanAccountRecoveryService recovery =
            fixture.recovery(0x40);
        const HumanAccountRecoveryResult result =
            recovery.recover(
                recoveryRequest(
                    AccountId,
                    "technical"));

        assert(
            result.status ==
            HumanAccountRecoveryStatus::credentialInvalid);
        const auto humanVerifier =
            fixture.verifiers.findByLogin(LoginName);
        assert(humanVerifier.has_value());
        assert(
            passwordMatches(
                OldPassword,
                humanVerifier->passwordHash));
        const auto technicalVerifier =
            fixture.verifiers.findByLogin("technical");
        assert(technicalVerifier.has_value());
        assert(
            passwordMatches(
                "technical-password",
                technicalVerifier->passwordHash));

        const auto events = fixture.accountability.listAll();
        assert(events.size() == 1);
        assert(events[0].decision == "deny");
        assert(events[0].reasonCode == "credential_invalid");
    }

    {
        Fixture fixture;
        const IssuedIds issued =
            fixture.issueSession(0x60);

        AccountabilityEvent duplicate;
        duplicate.eventId =
            "ace_707172737475767778797a7b7c7d7e7f";
        duplicate.eventType = "test.duplicate";
        duplicate.requestId = "preexisting-accountability-event";
        assert(fixture.accountability.append(duplicate));

        HumanAccountRecoveryService recovery =
            fixture.recovery(0x70);
        const HumanAccountRecoveryResult result =
            recovery.recover(recoveryRequest());

        assert(
            result.status ==
            HumanAccountRecoveryStatus::storageError);

        const auto verifier =
            fixture.verifiers.findByLogin(LoginName);
        assert(verifier.has_value());
        assert(passwordMatches(OldPassword, verifier->passwordHash));
        assert(!passwordMatches(NewPassword, verifier->passwordHash));

        const auto browser =
            fixture.browserCredentials.findBySessionId(
                issued.sessionId);
        assert(browser.has_value());
        assert(browser->active);
        assert(!browser->revoked);

        const auto session =
            fixture.identities.findSession(
                issued.sessionId);
        assert(session.has_value());
        assert(session->active);
        assert(!session->revoked);

        const auto browserCredential =
            fixture.identities.findCredential(
                issued.credentialId);
        assert(browserCredential.has_value());
        assert(browserCredential->active);
        assert(!browserCredential->revoked);

        const auto events = fixture.accountability.listAll();
        assert(events.size() == 1);
        assert(events[0].eventType == "test.duplicate");
    }

    return 0;
}

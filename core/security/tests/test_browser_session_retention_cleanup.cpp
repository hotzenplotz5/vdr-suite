#include "AccountabilityEventRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionRetentionService.h"
#include "Database.h"
#include "SecurityConfiguration.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace
{
constexpr const char* ActorId = "user-retention";
constexpr const char* DeviceId = "device-retention";
constexpr const char* SourceSessionId = "source-session-retention";
constexpr const char* SourceCredentialId = "source-credential-retention";

struct Lifecycle
{
    std::string tokenId;
    std::string sessionId;
    std::string credentialId;
};

std::string sqlQuoted(const std::string& value)
{
    return "'" + value + "'";
}

std::string numbered(const std::string& prefix, int value)
{
    std::ostringstream output;
    output << prefix << std::setw(3) << std::setfill('0') << value;
    return output.str();
}

struct Fixture
{
    Database database;
    AccountabilityEventRepository accountabilityRepository;
    SecurityIdentityRepository identityRepository;
    SecurityIdentityProvisioningRepository provisioningRepository;
    BrowserSessionCredentialRepository credentialRepository;
    SecurityPermissionGrantRepository grantRepository;
    BrowserSessionRetentionService retentionService;

    Fixture()
        : accountabilityRepository(database),
          identityRepository(database),
          provisioningRepository(database),
          credentialRepository(database),
          grantRepository(database),
          retentionService(
              database,
              credentialRepository,
              identityRepository,
              accountabilityRepository)
    {
        assert(database.open(":memory:"));
        assert(accountabilityRepository.ensureSchema());
        assert(identityRepository.ensureSchema());
        assert(credentialRepository.ensureSchema());
        assert(grantRepository.ensureSchema());
        assert(provisioningRepository.ensureIdentity(
            ActorId,
            ActorType::User,
            "Retention user",
            DeviceId,
            "Retention device",
            SourceSessionId,
            SourceCredentialId,
            "managed-basic"));
        assert(grantRepository.ensureGrant(
            ActorId,
            "recordings.view",
            "default"));
    }

    Lifecycle createLifecycle(
        const std::string& suffix,
        const std::string& expiresExpression,
        const std::string& lastSeenExpression,
        const std::string& revokedExpression,
        const std::string& credentialType = "browser-session")
    {
        Lifecycle lifecycle;
        lifecycle.tokenId = "token-" + suffix;
        lifecycle.sessionId = "session-" + suffix;
        lifecycle.credentialId = "credential-" + suffix;

        assert(identityRepository.createSessionCredential(
            lifecycle.sessionId,
            ActorId,
            DeviceId,
            lifecycle.credentialId,
            credentialType,
            "2099-01-01 00:00:00",
            SourceCredentialId));

        BrowserSessionCredentialRegistration registration;
        registration.tokenId = lifecycle.tokenId;
        registration.sessionId = lifecycle.sessionId;
        registration.actorId = ActorId;
        registration.deviceId = DeviceId;
        registration.credentialId = lifecycle.credentialId;
        registration.issuedFromCredentialId = SourceCredentialId;
        registration.sessionSecretHash = "$6$session-secret";
        registration.csrfSecretHash = "$6$csrf-secret";
        registration.expiresAt = "2099-01-01 00:00:00";
        assert(credentialRepository.insert(registration));

        assert(database.execute(
            "UPDATE security_browser_session_credentials SET "
            "expires_at = " + expiresExpression + ", "
            "last_seen_at = " + lastSeenExpression + ", "
            "revoked_at = " + revokedExpression + " "
            "WHERE token_id = " + sqlQuoted(lifecycle.tokenId) + ";"));
        return lifecycle;
    }
};

BrowserSessionRetentionConfiguration enabledRetention()
{
    BrowserSessionRetentionConfiguration configuration;
    configuration.seconds = 86400;
    return configuration;
}

BrowserSessionIdleConfiguration disabledIdle()
{
    BrowserSessionIdleConfiguration configuration;
    configuration.timeoutSeconds = 0;
    return configuration;
}

BrowserSessionIdleConfiguration enabledIdle()
{
    BrowserSessionIdleConfiguration configuration;
    configuration.timeoutSeconds = 300;
    return configuration;
}

void assertPresent(Fixture& fixture, const Lifecycle& lifecycle)
{
    assert(fixture.credentialRepository.findByTokenId(
        lifecycle.tokenId).has_value());
    assert(fixture.identityRepository.findSession(
        lifecycle.sessionId).has_value());
    assert(fixture.identityRepository.findCredential(
        lifecycle.credentialId).has_value());
}

void assertDeleted(Fixture& fixture, const Lifecycle& lifecycle)
{
    assert(!fixture.credentialRepository.findByTokenId(
        lifecycle.tokenId).has_value());
    assert(!fixture.identityRepository.findSession(
        lifecycle.sessionId).has_value());
    assert(!fixture.identityRepository.findCredential(
        lifecycle.credentialId).has_value());
}

std::string eventText(const AccountabilityEvent& event)
{
    return event.eventId + event.classes + event.eventType + event.severity +
        event.occurredAt + event.actorId + event.actorType + event.deviceId +
        event.sessionId + event.authenticationState + event.permission +
        event.backendId + event.operationId + event.requestId +
        event.correlationId + event.action + event.decision +
        event.reasonCode + event.outcome;
}

void testConfiguration()
{
    unsetenv("VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS");
    SecurityConfiguration configuration =
        SecurityConfiguration::fromEnvironment();
    assert(configuration.browserSessionRetention.valid());
    assert(!configuration.browserSessionRetention.enabled());
    assert(configuration.browserSessionRetention.seconds == 0);
    assert(BrowserSessionRetentionConfiguration::BatchSize == 256);

    for (const char* valid : {"0", "86400", "31536000"})
    {
        setenv(
            "VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS",
            valid,
            1);
        configuration = SecurityConfiguration::fromEnvironment();
        assert(configuration.browserSessionRetention.valid());
        assert(configuration.browserSessionRetention.seconds ==
            std::stoi(valid));
    }

    for (const char* invalid : {
             "", "-1", "+86400", " 86400", "86400 ", "86400s",
             "1", "86399", "31536001",
             "999999999999999999999999999"})
    {
        setenv(
            "VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS",
            invalid,
            1);
        configuration = SecurityConfiguration::fromEnvironment();
        assert(!configuration.browserSessionRetention.valid());
    }
    unsetenv("VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS");
}

void testDisabledPolicy()
{
    Fixture fixture;
    const Lifecycle oldExpired = fixture.createLifecycle(
        "disabled",
        "datetime(CURRENT_TIMESTAMP, '-90000 seconds')",
        "CURRENT_TIMESTAMP",
        "''");

    BrowserSessionRetentionConfiguration disabled;
    assert(fixture.retentionService.cleanup(disabled, disabledIdle()));
    assertPresent(fixture, oldExpired);
    assert(fixture.accountabilityRepository.listAll().empty());

    BrowserSessionRetentionConfiguration invalid;
    invalid.configuredValueValid = false;
    assert(!fixture.retentionService.cleanup(invalid, disabledIdle()));
    assertPresent(fixture, oldExpired);
}

void testEligibilityAndPreservation()
{
    Fixture fixture;
    const Lifecycle active = fixture.createLifecycle(
        "active",
        "datetime(CURRENT_TIMESTAMP, '+3600 seconds')",
        "CURRENT_TIMESTAMP",
        "''");
    const Lifecycle revokedRecent = fixture.createLifecycle(
        "revoked-recent",
        "datetime(CURRENT_TIMESTAMP, '+3600 seconds')",
        "CURRENT_TIMESTAMP",
        "datetime(CURRENT_TIMESTAMP, '-86399 seconds')");
    const Lifecycle revokedOld = fixture.createLifecycle(
        "revoked-old",
        "datetime(CURRENT_TIMESTAMP, '+3600 seconds')",
        "CURRENT_TIMESTAMP",
        "datetime(CURRENT_TIMESTAMP, '-86401 seconds')");
    const Lifecycle expiredRecent = fixture.createLifecycle(
        "expired-recent",
        "datetime(CURRENT_TIMESTAMP, '-86399 seconds')",
        "CURRENT_TIMESTAMP",
        "''");
    const Lifecycle expiredOld = fixture.createLifecycle(
        "expired-old",
        "datetime(CURRENT_TIMESTAMP, '-86401 seconds')",
        "CURRENT_TIMESTAMP",
        "''");
    const Lifecycle idleOnly = fixture.createLifecycle(
        "idle-disabled",
        "datetime(CURRENT_TIMESTAMP, '+3600 seconds')",
        "datetime(CURRENT_TIMESTAMP, '-200000 seconds')",
        "''");
    const Lifecycle issuerOnly = fixture.createLifecycle(
        "issuer-only",
        "datetime(CURRENT_TIMESTAMP, '+3600 seconds')",
        "CURRENT_TIMESTAMP",
        "''");

    assert(fixture.identityRepository.revokeCredential(SourceCredentialId));
    assert(fixture.retentionService.cleanup(
        enabledRetention(),
        disabledIdle()));

    assertPresent(fixture, active);
    assertPresent(fixture, revokedRecent);
    assertDeleted(fixture, revokedOld);
    assertPresent(fixture, expiredRecent);
    assertDeleted(fixture, expiredOld);
    assertPresent(fixture, idleOnly);
    assertPresent(fixture, issuerOnly);

    const auto actor = fixture.identityRepository.findActor(ActorId);
    const auto device = fixture.identityRepository.findDevice(DeviceId);
    const auto issuer = fixture.identityRepository.findCredential(
        SourceCredentialId);
    assert(actor.has_value());
    assert(device.has_value());
    assert(issuer.has_value() && issuer->revoked);

    const auto grants = fixture.grantRepository.findActiveGrantsForActor(
        ActorId);
    assert(grants.available);
    assert(grants.grants.size() == 1);
    assert(grants.grants.front().permission == "recordings.view");

    const std::vector<AccountabilityEvent> events =
        fixture.accountabilityRepository.listAll();
    assert(events.size() == 2);
    for (const AccountabilityEvent& event : events)
    {
        assert(event.classes == "security,lifecycle,maintenance");
        assert(event.eventType == "operation.succeeded");
        assert(event.severity == "info");
        assert(event.actorId == ActorId);
        assert(event.actorType == "system");
        assert(event.deviceId == DeviceId);
        assert(event.authenticationState == "system-maintenance");
        assert(event.action == "browser.session.cleanup");
        assert(event.decision == "completed");
        assert(event.reasonCode ==
            "browser_session_retention_elapsed");
        assert(event.outcome == "deleted");
        assert(!event.requestId.empty());
        assert(eventText(event).find("$6$") == std::string::npos);
        assert(eventText(event).find("session-secret") ==
            std::string::npos);
        assert(eventText(event).find("csrf-secret") ==
            std::string::npos);
    }
}

void testIdleEligibility()
{
    Fixture fixture;
    const Lifecycle recent = fixture.createLifecycle(
        "idle-recent",
        "datetime(CURRENT_TIMESTAMP, '+3600 seconds')",
        "datetime(CURRENT_TIMESTAMP, '-86699 seconds')",
        "''");
    const Lifecycle old = fixture.createLifecycle(
        "idle-old",
        "datetime(CURRENT_TIMESTAMP, '+3600 seconds')",
        "datetime(CURRENT_TIMESTAMP, '-86701 seconds')",
        "''");

    assert(fixture.retentionService.cleanup(
        enabledRetention(),
        enabledIdle()));
    assertPresent(fixture, recent);
    assertDeleted(fixture, old);
}

void testOwnedIdentityDeletion()
{
    Fixture fixture;
    const Lifecycle nonBrowser = fixture.createLifecycle(
        "non-browser",
        "datetime(CURRENT_TIMESTAMP, '-90000 seconds')",
        "CURRENT_TIMESTAMP",
        "''",
        "api-key");
    const Lifecycle referenced = fixture.createLifecycle(
        "referenced",
        "datetime(CURRENT_TIMESTAMP, '-90000 seconds')",
        "CURRENT_TIMESTAMP",
        "''");

    assert(fixture.database.execute(
        "CREATE TRIGGER replace_retained_verifier "
        "AFTER DELETE ON security_browser_session_credentials "
        "WHEN OLD.token_id = 'token-referenced' BEGIN "
        "INSERT INTO security_browser_session_credentials ("
        "token_id, session_id, actor_id, device_id, credential_id, "
        "issued_from_credential_id, session_secret_hash, csrf_secret_hash, "
        "expires_at, last_seen_at) VALUES ("
        "'token-replacement', OLD.session_id, OLD.actor_id, OLD.device_id, "
        "OLD.credential_id, OLD.issued_from_credential_id, "
        "'$6$replacement-session', '$6$replacement-csrf', "
        "datetime(CURRENT_TIMESTAMP, '+3600 seconds'), CURRENT_TIMESTAMP); "
        "END;"));

    assert(fixture.retentionService.cleanup(
        enabledRetention(),
        disabledIdle()));

    assert(!fixture.credentialRepository.findByTokenId(
        nonBrowser.tokenId).has_value());
    assert(!fixture.identityRepository.findSession(
        nonBrowser.sessionId).has_value());
    const auto preservedNonBrowser = fixture.identityRepository.findCredential(
        nonBrowser.credentialId);
    assert(preservedNonBrowser.has_value());
    assert(preservedNonBrowser->credentialType == "api-key");

    assert(!fixture.credentialRepository.findByTokenId(
        referenced.tokenId).has_value());
    assert(fixture.credentialRepository.findByTokenId(
        "token-replacement").has_value());
    assert(fixture.identityRepository.findSession(
        referenced.sessionId).has_value());
    assert(fixture.identityRepository.findCredential(
        referenced.credentialId).has_value());
}

void testBoundedDeterministicBatch()
{
    Fixture fixture;
    std::vector<Lifecycle> lifecycles;
    for (int index = 0; index < 258; ++index)
    {
        lifecycles.push_back(fixture.createLifecycle(
            numbered("batch-", index),
            sqlQuoted("2020-01-01 00:00:00"),
            "CURRENT_TIMESTAMP",
            "''"));
    }

    assert(fixture.retentionService.cleanup(
        enabledRetention(),
        disabledIdle()));

    assert(!fixture.credentialRepository.findByTokenId(
        lifecycles[0].tokenId).has_value());
    assert(!fixture.credentialRepository.findByTokenId(
        lifecycles[255].tokenId).has_value());
    assertPresent(fixture, lifecycles[256]);
    assertPresent(fixture, lifecycles[257]);
    assert(fixture.accountabilityRepository.listAll().size() == 256);
}

void testAuditFailureRollback()
{
    Fixture fixture;
    const Lifecycle candidate = fixture.createLifecycle(
        "audit-failure",
        sqlQuoted("2020-01-01 00:00:00"),
        "CURRENT_TIMESTAMP",
        "''");
    assert(fixture.database.execute(
        "CREATE TRIGGER fail_cleanup_audit "
        "BEFORE INSERT ON accountability_events "
        "WHEN NEW.action = 'browser.session.cleanup' BEGIN "
        "SELECT RAISE(ABORT, 'forced cleanup audit failure'); END;"));

    assert(!fixture.retentionService.cleanup(
        enabledRetention(),
        disabledIdle()));
    assertPresent(fixture, candidate);
    assert(fixture.accountabilityRepository.listAll().empty());
}

void testSqlFailureRollsBackWholeBatch()
{
    Fixture fixture;
    const Lifecycle first = fixture.createLifecycle(
        "rollback-a",
        sqlQuoted("2019-01-01 00:00:00"),
        "CURRENT_TIMESTAMP",
        "''");
    const Lifecycle second = fixture.createLifecycle(
        "rollback-b",
        sqlQuoted("2020-01-01 00:00:00"),
        "CURRENT_TIMESTAMP",
        "''");
    assert(fixture.database.execute(
        "CREATE TRIGGER fail_second_cleanup "
        "BEFORE DELETE ON security_browser_session_credentials "
        "WHEN OLD.token_id = 'token-rollback-b' BEGIN "
        "SELECT RAISE(ABORT, 'forced cleanup delete failure'); END;"));

    assert(!fixture.retentionService.cleanup(
        enabledRetention(),
        disabledIdle()));
    assertPresent(fixture, first);
    assertPresent(fixture, second);
    assert(fixture.accountabilityRepository.listAll().empty());
}
}

int main()
{
    testConfiguration();
    testDisabledPolicy();
    testEligibilityAndPreservation();
    testIdleEligibility();
    testOwnedIdentityDeletion();
    testBoundedDeterministicBatch();
    testAuditFailureRollback();
    testSqlFailureRollsBackWholeBatch();
    return 0;
}
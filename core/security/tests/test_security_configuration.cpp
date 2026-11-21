#include "SecurityConfiguration.h"

#include <cassert>
#include <cstdlib>
#include <string>

namespace
{
void clearEnvironment()
{
    unsetenv("VDR_SUITE_SECURITY_MODE");
    unsetenv("VDR_SUITE_BASIC_AUTH");
    unsetenv("VDR_SUITE_LEGACY_BASIC_CREDENTIAL_ID");
    unsetenv("VDR_SUITE_LEGACY_BASIC_PERMISSIONS");
    unsetenv("VDR_SUITE_MANAGED_BASIC_USERNAME");
    unsetenv("VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH");
    unsetenv("VDR_SUITE_MANAGED_BASIC_ACTOR_ID");
    unsetenv("VDR_SUITE_MANAGED_BASIC_ACTOR_DISPLAY_NAME");
    unsetenv("VDR_SUITE_MANAGED_BASIC_DEVICE_ID");
    unsetenv("VDR_SUITE_MANAGED_BASIC_SESSION_ID");
    unsetenv("VDR_SUITE_MANAGED_BASIC_CREDENTIAL_ID");
    unsetenv("VDR_SUITE_MANAGED_BASIC_PERMISSIONS");
    unsetenv("VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS");
    unsetenv("VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR");
}
}

int main()
{
    clearEnvironment();

    const SecurityConfiguration defaults =
        SecurityConfiguration::fromEnvironment();
    assert(!defaults.managedBasic.hasAnyConfiguration());
    assert(!defaults.managedBasic.complete());
    assert(defaults.managedBasic.grants.empty());
    assert(defaults.browserSessionLifetime.valid());
    assert(defaults.browserSessionLifetime.seconds == 28800);
    assert(defaults.browserSessionConcurrency.valid());
    assert(defaults.browserSessionConcurrency.maximumActivePerActor == 0);

    setenv("VDR_SUITE_SECURITY_MODE", "legacy-basic", 1);
    setenv("VDR_SUITE_BASIC_AUTH", "Basic configured", 1);
    setenv(
        "VDR_SUITE_LEGACY_BASIC_CREDENTIAL_ID",
        "credential-configured",
        1);
    setenv(
        "VDR_SUITE_LEGACY_BASIC_PERMISSIONS",
        "remote.control@default, recordings.view@*",
        1);
    const SecurityConfiguration retiredLegacyInputs =
        SecurityConfiguration::fromEnvironment();
    assert(!retiredLegacyInputs.managedBasic.hasAnyConfiguration());
    assert(retiredLegacyInputs.managedBasic.grants.empty());

    clearEnvironment();
    setenv("VDR_SUITE_MANAGED_BASIC_USERNAME", "phase62-admin", 1);
    const SecurityConfiguration partialManaged =
        SecurityConfiguration::fromEnvironment();
    assert(partialManaged.managedBasic.hasAnyConfiguration());
    assert(!partialManaged.managedBasic.complete());

    setenv(
        "VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH",
        "$6$testsalt$hash",
        1);
    setenv(
        "VDR_SUITE_MANAGED_BASIC_ACTOR_ID",
        "user-admin",
        1);
    setenv(
        "VDR_SUITE_MANAGED_BASIC_ACTOR_DISPLAY_NAME",
        "Administrator",
        1);
    setenv(
        "VDR_SUITE_MANAGED_BASIC_DEVICE_ID",
        "device-admin",
        1);
    setenv(
        "VDR_SUITE_MANAGED_BASIC_SESSION_ID",
        "session-admin",
        1);
    setenv(
        "VDR_SUITE_MANAGED_BASIC_CREDENTIAL_ID",
        "credential-admin",
        1);
    setenv(
        "VDR_SUITE_MANAGED_BASIC_PERMISSIONS",
        "remote.control@default, audit.view@*",
        1);

    const SecurityConfiguration managed =
        SecurityConfiguration::fromEnvironment();
    assert(managed.managedBasic.complete());
    assert(managed.managedBasic.username == "phase62-admin");
    assert(managed.managedBasic.passwordHash == "$6$testsalt$hash");
    assert(managed.managedBasic.actorId == "user-admin");
    assert(managed.managedBasic.actorDisplayName == "Administrator");
    assert(managed.managedBasic.deviceId == "device-admin");
    assert(managed.managedBasic.sessionId == "session-admin");
    assert(managed.managedBasic.credentialId == "credential-admin");
    assert(managed.managedBasic.grants.size() == 2);
    assert(managed.managedBasic.grants[0].permission ==
        "remote.control");
    assert(managed.managedBasic.grants[0].backendId == "default");
    assert(managed.managedBasic.grants[1].permission == "audit.view");
    assert(managed.managedBasic.grants[1].backendId == "*");

    clearEnvironment();
    setenv(
        "VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS",
        "900",
        1);
    const SecurityConfiguration customLifetime =
        SecurityConfiguration::fromEnvironment();
    assert(customLifetime.browserSessionLifetime.valid());
    assert(customLifetime.browserSessionLifetime.seconds == 900);

    for (const std::string invalidValue : {
             "",
             "299",
             "86401",
             "+3600",
             " 3600",
             "3600 ",
             "3600x",
             "999999999999999999999999999999999999"})
    {
        clearEnvironment();
        setenv(
            "VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS",
            invalidValue.c_str(),
            1);
        const SecurityConfiguration invalidLifetime =
            SecurityConfiguration::fromEnvironment();
        assert(!invalidLifetime.browserSessionLifetime.valid());
        assert(invalidLifetime.browserSessionLifetime.seconds == 28800);
    }

    clearEnvironment();
    setenv(
        "VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS",
        "300",
        1);
    assert(SecurityConfiguration::fromEnvironment()
        .browserSessionLifetime.valid());

    clearEnvironment();
    setenv(
        "VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS",
        "86400",
        1);
    assert(SecurityConfiguration::fromEnvironment()
        .browserSessionLifetime.valid());

    for (const std::string validValue : {"0", "1", "64"})
    {
        clearEnvironment();
        setenv(
            "VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR",
            validValue.c_str(),
            1);
        const SecurityConfiguration concurrency =
            SecurityConfiguration::fromEnvironment();
        assert(concurrency.browserSessionConcurrency.valid());
        assert(
            concurrency.browserSessionConcurrency.maximumActivePerActor ==
            static_cast<std::size_t>(std::stoul(validValue)));
    }

    for (const std::string invalidValue : {
             "",
             "65",
             "+1",
             "-1",
             " 1",
             "1 ",
             "1x",
             "999999999999999999999999999999999999"})
    {
        clearEnvironment();
        setenv(
            "VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR",
            invalidValue.c_str(),
            1);
        const SecurityConfiguration concurrency =
            SecurityConfiguration::fromEnvironment();
        assert(!concurrency.browserSessionConcurrency.valid());
        assert(
            concurrency.browserSessionConcurrency.maximumActivePerActor == 0);
    }

    clearEnvironment();
    return 0;
}

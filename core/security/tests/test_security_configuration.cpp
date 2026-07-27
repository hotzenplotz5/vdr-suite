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
}
}

int main()
{
    clearEnvironment();

    const SecurityConfiguration compatibility =
        SecurityConfiguration::fromEnvironment();
    assert(compatibility.mode ==
        SecurityMode::LegacyBasicCompatibility);
    assert(!compatibility.expectedAuthorizationHeader.empty());
    assert(compatibility.credentialId ==
        "legacy-basic-credential");
    assert(compatibility.grants.size() == 1);
    assert(compatibility.grants.front().permission == "*");

    setenv("VDR_SUITE_SECURITY_MODE", "enforced", 1);
    const SecurityConfiguration failClosed =
        SecurityConfiguration::fromEnvironment();
    assert(failClosed.mode == SecurityMode::Enforced);
    assert(failClosed.expectedAuthorizationHeader.empty());
    assert(failClosed.grants.empty());

    setenv("VDR_SUITE_BASIC_AUTH", "Basic configured", 1);
    setenv(
        "VDR_SUITE_LEGACY_BASIC_CREDENTIAL_ID",
        "credential-configured",
        1);
    setenv(
        "VDR_SUITE_LEGACY_BASIC_PERMISSIONS",
        "remote.control@default, recordings.view@*",
        1);
    const SecurityConfiguration configured =
        SecurityConfiguration::fromEnvironment();
    assert(configured.expectedAuthorizationHeader ==
        "Basic configured");
    assert(configured.credentialId ==
        "credential-configured");
    assert(configured.grants.size() == 2);
    assert(configured.grants[0].permission == "remote.control");
    assert(configured.grants[0].backendId == "default");
    assert(configured.grants[1].permission == "recordings.view");
    assert(configured.grants[1].backendId == "*");

    clearEnvironment();
    return 0;
}

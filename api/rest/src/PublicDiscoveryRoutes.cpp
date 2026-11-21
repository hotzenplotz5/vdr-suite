#include "PublicDiscoveryRoutes.h"
#include "PublicApiRuntime.h"
#include "ServerBuildIdentity.h"

#include <string>

namespace {

std::string contractRootBody(
    const bool authenticated
)
{
    return (
        std::string("{\"apiVersion\":\"v1\",\"serverVersion\":\"")
        + VdrSuiteServerBuildIdentity::ServerVersion
        + "\",\"supportedApiMajors\":[\"v1\"],"
          "\"compatibilityPolicy\":{\"version\":1,"
          "\"responseEvolution\":\"additive\","
          "\"breakingChanges\":\"new-major\","
          "\"unknownRequestFields\":\"reject\","
          "\"legacyUnversioned\":\"transition\"},"
          "\"authentication\":{\"authenticated\":"
        + (authenticated ? "true" : "false")
        + "},\"links\":{\"self\":\"/api/v1\",\"capabilities\":\"/api/v1/capabilities\",\"backends\":\"/api/v1/backends\",\"accounts\":\"/api/v1/accounts\",\"devicePairings\":\"/api/v1/device-pairings\"}}"
        );
}

std::string platformCapabilitiesBody(
    const bool operationReadAvailable,
    const bool timerAssignmentReadAvailable,
    const bool timerCreateAdmissionAvailable,
    const bool searchTimerCollectionAvailable,
    const bool backendCollectionAvailable,
    const bool recordingCollectionAvailable,
    const bool genreCollectionAvailable,
    const bool epgGenreAvailable,
    const bool epgNowNextAvailable,
    const bool epgWindowAvailable,
    const bool epgEventDetailAvailable,
    const bool nativeTimersAvailable,
    const bool searchAvailable,
    const bool accountCollectionAvailable,
    const bool accountMutationAvailable,
    const bool accountCreateAvailable,
    const bool accountGrantAdministrationAvailable,
    const bool deviceGrantAdministrationAvailable,
    const bool deviceLifecycleAdministrationAvailable,
    const bool deviceCredentialRotationAvailable,
    const bool accountSecurityMetadataAvailable,
    const bool accountCredentialRevokeAvailable,
    const bool accountSessionRevokeAvailable,
    const bool devicePairingBootstrapAvailable,
    const bool devicePairingAdministrationAvailable,
    const bool deviceCredentialIssueAvailable
)
{
    return (
        "{\"apiVersion\":\"v1\",\"capabilities\":["
        "{\"id\":\"public-api.contract-root\",\"version\":1,\"availability\":\"available\"},"
        "{\"id\":\"public-api.durable-operations-read\",\"version\":1,\"availability\":\"" +
        std::string(operationReadAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.timer-assignments-read\",\"version\":1,\"availability\":\"" +
        std::string(timerAssignmentReadAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.search-timers-read\",\"version\":1,\"availability\":\"" +
        std::string(searchTimerCollectionAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.timer-create-admission\",\"version\":1,\"availability\":\"" +
        std::string(timerCreateAdmissionAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.backends-read\",\"version\":1,\"availability\":\"" +
        std::string(backendCollectionAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.recordings-read\",\"version\":1,\"availability\":\"" +
        std::string(recordingCollectionAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.recordings-browse\",\"version\":1,\"availability\":\"" +
        std::string(recordingCollectionAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.recording-genres-read\",\"version\":1,\"availability\":\"" +
        std::string(genreCollectionAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.epg-genres-read\",\"version\":1,\"availability\":\"" +
        std::string(epgGenreAvailable ? "available" : "unavailable") +
        "\"}," 
        "{\"id\":\"public-api.recording-search\",\"version\":1,\"availability\":\"" +
        std::string(searchAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.epg-search\",\"version\":1,\"availability\":\"" +
        std::string(searchAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.epg-now-next-read\",\"version\":1,\"availability\":\"" +
        std::string(epgNowNextAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.epg-window-read\",\"version\":1,\"availability\":\"" +
        std::string(epgWindowAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.epg-event-detail-read\",\"version\":1,\"availability\":\"" +
        std::string(epgEventDetailAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.native-timers-read\",\"version\":1,\"availability\":\"" +
        std::string(nativeTimersAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-read\",\"version\":1,\"availability\":\"" +
        std::string(accountCollectionAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-lifecycle-mutation\",\"version\":1,\"availability\":\"" +
        std::string(accountMutationAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-create\",\"version\":1,\"availability\":\"" +
        std::string(accountCreateAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-grants-administration\",\"version\":1,\"availability\":\"" +
        std::string(accountGrantAdministrationAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.devices-grants-administration\",\"version\":1,\"availability\":\"" +
        std::string(deviceGrantAdministrationAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.devices-lifecycle-administration\",\"version\":1,\"availability\":\"" +
        std::string(deviceLifecycleAdministrationAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.devices-credential-rotation\",\"version\":1,\"availability\":\"" +
        std::string(deviceCredentialRotationAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-credential-session-metadata\",\"version\":1,\"availability\":\"" +
        std::string(accountSecurityMetadataAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-credential-revoke\",\"version\":1,\"availability\":\"" +
        std::string(accountCredentialRevokeAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.accounts-session-revoke\",\"version\":1,\"availability\":\"" +
        std::string(accountSessionRevokeAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.device-pairing-bootstrap\",\"version\":1,\"availability\":\"" +
        std::string(devicePairingBootstrapAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.device-pairing-administration\",\"version\":1,\"availability\":\"" +
        std::string(devicePairingAdministrationAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.device-credential-issuance\",\"version\":1,\"availability\":\"" +
        std::string(deviceCredentialIssueAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.compatibility-policy\",\"version\":1,\"availability\":\"available\"},"
        "{\"id\":\"public-api.deprecation-metadata\",\"version\":1,\"availability\":\"available\"}"
        "],\"compatibility\":{\"policyVersion\":1,"
        "\"supportedApiMajors\":[\"v1\"],"
        "\"responseEvolution\":\"additive\","
        "\"breakingChanges\":\"new-major\","
        "\"unknownRequestFields\":\"reject\","
        "\"legacyUnversioned\":\"transition\","
        "\"lifecycle\":[\"supported\",\"deprecated\",\"sunset-announced\",\"removed\"],"
        "\"deprecationHeaders\":[\"Deprecation\",\"Sunset\",\"Link\"],"
        "\"deprecatedAliases\":[]},"
        "\"links\":{\"self\":\"/api/v1/capabilities\",\"root\":\"/api/v1\"}}"
        );
}

} // namespace

namespace vdrsuite {
namespace public_api {

std::string discoveryGetBody(
    const PublicApiRuntime& runtime,
    const std::string& path,
    bool authenticated)
{
    if (path == "/api/v1")
        return contractRootBody(authenticated);
    if (path == "/api/v1/capabilities")
        return platformCapabilitiesBody(

            runtime.operationLookupConfigured(),
            runtime.timerAssignmentLookupConfigured(),
            runtime.timerCreateAdmissionConfigured(),
            runtime.searchTimerCollectionLookupConfigured(),
            runtime.backendCollectionLookupConfigured(),
            runtime.recordingCollectionLookupConfigured(),
            runtime.genreCollectionLookupConfigured(),
            runtime.epgGenreLookupConfigured(),
            runtime.epgNowNextLookupConfigured(),
            runtime.epgWindowLookupConfigured(),
            runtime.epgEventDetailLookupConfigured(),
            runtime.nativeTimersLookupConfigured(),
            runtime.searchLookupConfigured(),
            runtime.accountCollectionLookupConfigured(),
            runtime.accountMutationConfigured(),
            runtime.accountCreateConfigured(),
            runtime.accountGrantLookupConfigured() &&
                runtime.accountGrantMutationConfigured(),
            runtime.deviceGrantAdministrationConfigured(),
            runtime.deviceLifecycleAdministrationConfigured(),
            runtime.deviceCredentialRotationConfigured(),
            runtime.accountCredentialLookupConfigured() &&
                runtime.accountSessionLookupConfigured(),
            runtime.accountCredentialItemLookupConfigured() &&
                runtime.accountCredentialMutationConfigured(),
            runtime.accountSessionItemLookupConfigured() &&
                runtime.accountSessionMutationConfigured(),
            runtime.devicePairingCreateConfigured() &&
                runtime.devicePairingLookupConfigured(),
            runtime.devicePairingAdministrationCollectionLookupConfigured() &&
                runtime.devicePairingAdministrationLookupConfigured() &&
                runtime.devicePairingDecisionConfigured(),
            runtime.deviceCredentialIssueConfigured());
    return {};
}

} // namespace public_api
} // namespace vdrsuite

#include "MediaAccessGrantAuthenticator.h"
#include "MediaRouteLeaseRepository.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"

#include "Database.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <string>

namespace
{

class DeterministicEntropy
{
public:
    bool fill(unsigned char* output, std::size_t size)
    {
        if (output == nullptr || size == 0) return false;
        for (std::size_t index = 0; index < size; ++index) {
            output[index] = next_++;
        }
        return true;
    }

private:
    unsigned char next_ = 1;
};

std::chrono::system_clock::time_point futureClock()
{
    std::tm utc{};
    utc.tm_year = 130; // 2030
    utc.tm_mon = 0;
    utc.tm_mday = 1;
    return std::chrono::system_clock::from_time_t(timegm(&utc));
}

MediaSessionIssuanceRequest request()
{
    MediaSessionIssuanceRequest value;
    value.actorId = "actor-1";
    value.backendId = "default";
    value.resourceKind = "recording";
    value.resourceId = "42";
    value.presentationProfileId = "hls-fmp4";
    value.providerId = "local-vdr-recording";
    value.lifetimeSeconds = 21600;
    return value;
}

} // namespace

int main()
{
    Database database;
    assert(database.open(":memory:"));

    MediaSessionRepository repository(database);
    assert(repository.ensureSchema());
    MediaRouteLeaseRepository routeLeaseRepository(database);

    DeterministicEntropy entropy;
    MediaSessionIssuanceService issuer(
        repository,
        [&entropy](unsigned char* output, std::size_t size) {
            return entropy.fill(output, size);
        },
        [] { return futureClock(); });

    auto issued = issuer.issue(request());
    assert(issued.issued);
    assert(issued.reasonCode.empty());
    assert(issued.session.sessionId.rfind("ms_", 0) == 0);
    assert(issued.session.routeId.rfind("mr_", 0) == 0);
    assert(issued.session.leaseId.rfind("pl_", 0) == 0);
    assert(issued.session.grantId.rfind("mg_", 0) == 0);
    assert(issued.session.workspaceId == issued.session.sessionId);
    assert(issued.session.accessCredential.rfind(issued.session.grantId + ".", 0) == 0);

    const auto provisioning = repository.findSession(issued.session.sessionId);
    assert(provisioning.has_value());
    assert(provisioning->state == "provisioning");
    assert(!routeLeaseRepository.findActive(
        issued.session.sessionId,
        issued.session.routeId,
        issued.session.routeEpoch).has_value());

    const auto storedGrant = repository.findResolvedGrant(issued.session.grantId, 300);
    assert(storedGrant.has_value());
    assert(!storedGrant->active);
    assert(MediaSessionRepository::supportsSecretHash(storedGrant->secretHash));
    assert(storedGrant->secretHash != issued.session.accessCredential);
    assert(storedGrant->secretHash.find(issued.session.accessCredential) == std::string::npos);

    MediaAccessGrantAuthenticator authenticator(repository, 300, 60);
    const auto beforeReady = authenticator.authenticate(
        issued.session.accessCredential,
        issued.session.sessionId);
    assert(!beforeReady.authenticated);
    assert(beforeReady.reasonCode == "media_access_inactive");

    assert(repository.activateBundle(issued.session.sessionId));
    const auto accepted = authenticator.authenticate(
        issued.session.accessCredential,
        issued.session.sessionId);
    assert(accepted.authenticated);
    assert(accepted.sessionId == issued.session.sessionId);
    assert(accepted.routeId == issued.session.routeId);
    assert(accepted.routeEpoch == 1);
    assert(accepted.actorId == "actor-1");

    const auto activeLease = routeLeaseRepository.findActive(
        accepted.sessionId,
        accepted.routeId,
        accepted.routeEpoch);
    assert(activeLease.has_value());
    assert(activeLease->sessionId == issued.session.sessionId);
    assert(activeLease->routeId == issued.session.routeId);
    assert(activeLease->routeEpoch == 1);
    assert(activeLease->providerId == "local-vdr-recording");
    assert(activeLease->leaseId == issued.session.leaseId);
    assert(activeLease->workspaceId == issued.session.workspaceId);
    assert(activeLease->presentationProfileId == "hls-fmp4");

    assert(!routeLeaseRepository.findActive(
        accepted.sessionId,
        accepted.routeId,
        accepted.routeEpoch + 1).has_value());

    std::string wrongCredential = issued.session.accessCredential;
    wrongCredential.back() = wrongCredential.back() == 'A' ? 'B' : 'A';
    const auto wrongSecret = authenticator.authenticate(
        wrongCredential,
        issued.session.sessionId);
    assert(!wrongSecret.authenticated);
    assert(wrongSecret.reasonCode == "media_access_denied");

    const auto wrongFence = authenticator.authenticate(
        issued.session.accessCredential,
        "ms_00000000000000000000000000000000");
    assert(!wrongFence.authenticated);
    assert(wrongFence.reasonCode == "media_access_fence_mismatch");

    assert(repository.endBundle(issued.session.sessionId, "client_stop"));
    const auto ended = repository.findSession(issued.session.sessionId);
    assert(ended.has_value());
    assert(ended->state == "ended");
    assert(ended->terminalReason == "client_stop");
    assert(!routeLeaseRepository.findActive(
        issued.session.sessionId,
        issued.session.routeId,
        issued.session.routeEpoch).has_value());

    const auto afterEnd = authenticator.authenticate(
        issued.session.accessCredential,
        issued.session.sessionId);
    assert(!afterEnd.authenticated);
    assert(afterEnd.reasonCode == "media_access_inactive");

    auto second = issuer.issue(request());
    assert(second.issued);
    assert(repository.activateBundle(second.session.sessionId));
    assert(repository.recoverNonTerminalBundles());

    const auto recovered = repository.findSession(second.session.sessionId);
    assert(recovered.has_value());
    assert(recovered->state == "failed");
    assert(recovered->terminalReason == "daemon_restart_ownership_lost");
    const auto afterRestart = authenticator.authenticate(
        second.session.accessCredential,
        second.session.sessionId);
    assert(!afterRestart.authenticated);
    assert(afterRestart.reasonCode == "media_access_inactive");

    issued.session.clearSecret();
    second.session.clearSecret();
    return 0;
}
#include "PublicDevicePlaybackRegistry.h"
#include "Database.h"
#include "MediaSessionIssuanceService.h"
#include "SecurityIdentityProvisioningRepository.h"

#include <cassert>
#include <chrono>
#include <ctime>
#include <string>

namespace {
RequestSecurityContext device() {
    RequestSecurityContext c;
    c.authenticationState = AuthenticationState::Authenticated;
    c.actor = {"actor-playback", ActorType::Service, "TV", true};
    c.device = DeviceIdentity{"device-playback",true};
    c.credential = CredentialIdentity{"credential-playback",true,false,false};
    c.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    c.grants = {{"media.recording.play","backend-a"}};
    return c;
}
}

int main() {
    Database db;
    assert(db.open(":memory:"));
    MediaSessionRepository sessions(db);
    SecurityIdentityRepository identities(db);
    SecurityPermissionGrantRepository grants(db);
    SecurityIdentityProvisioningRepository provisioning(db);
    assert(sessions.ensureSchema());
    assert(identities.ensureSchema());
    assert(grants.ensureSchema());
    assert(provisioning.ensureTechnicalIdentity(
        "actor-playback",ActorType::Service,"TV",
        "device-playback","TV","credential-playback","device-app"));
    assert(grants.ensureGrant("actor-playback","media.recording.play","backend-a"));
    MediaSessionIssuanceService issuer(sessions,{},[]{
        std::tm utc{};
        utc.tm_year=130;
        utc.tm_mday=1;
        return std::chrono::system_clock::from_time_t(timegm(&utc));
    });
    MediaSessionIssuanceRequest request;
    request.actorId="actor-playback";
    request.backendId="backend-a";
    request.resourceKind="recording";
    request.resourceId="native-opaque";
    request.presentationProfileId="hls-ts";
    request.providerId="local-vdr-recording";
    request.lifetimeSeconds=3600;
    auto issue=issuer.issue(request);
    assert(issue.issued);
    assert(sessions.activateBundle(issue.session.sessionId));
    const auto id=issue.session.sessionId;

    PublicDevicePlaybackRegistry registry(sessions,identities,grants);
    assert(!registry.authorized(id,"actor-playback","backend-a",true));
    assert(registry.authorized(id,"actor-playback","backend-a",false));
    // Device grants can be revoked while HLS provisioning is running.
    assert(grants.revokeGrant("actor-playback","media.recording.play","backend-a"));
    assert(!registry.add(id,"backend-a",device()));
    assert(grants.ensureGrant("actor-playback","media.recording.play","backend-a"));
    assert(registry.add(id,"backend-a",device()));
    assert(!registry.add(id,"backend-a",device()));
    assert(registry.authorized(id,"actor-playback","backend-a",true));
    assert(registry.authorized(id,"actor-playback","backend-a",false));
    assert(!registry.authorized(id,"other","backend-a",true));
    assert(!registry.authorized(id,"actor-playback","backend-b",true));
    assert(registry.owned(id,"backend-a",device()));
    auto described=registry.describeOwned(id,device());
    assert(described.has_value());
    assert(described->sessionId==id);
    assert(described->state=="ready");
    assert(described->resourceId=="native-opaque");
    assert(described->presentationProfileId=="hls-ts");
    assert(!registry.describeOwned("ms_ffffffffffffffffffffffffffffffff",device()));

    auto other=device();
    other.credential->credentialId="other-credential";
    assert(!registry.owned(id,"backend-a",other));
    assert(!registry.describeOwned(id,other));

    assert(grants.revokeGrant("actor-playback","media.recording.play","backend-a"));
    assert(!registry.authorized(id,"actor-playback","backend-a",true));
    assert(!registry.authorized(id,"actor-playback","backend-a",false));
    assert(!registry.describeOwned(id,device()));
    assert(grants.ensureGrant("actor-playback","media.recording.play","backend-a"));
    assert(registry.authorized(id,"actor-playback","backend-a",true));
    assert(identities.revokeCredential("credential-playback"));
    assert(!registry.authorized(id,"actor-playback","backend-a",true));
    assert(!registry.describeOwned(id,device()));
    registry.erase(id);
    assert(!registry.authorized(id,"actor-playback","backend-a",true));
    return 0;
}

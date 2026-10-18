#include "PublicRecordingDevicePlaybackAdmission.h"
#include "PublicRecordingIdentityRepository.h"
#include "VdrRecordingCacheRepository.h"
#include "Database.h"

#include <cassert>
#include <string>

namespace {
using Status = PublicRecordingDevicePlaybackAdmissionStatus;
VdrRecording recording(const std::string& id, const std::string& native) {
    VdrRecording r;
    r.id = id;
    r.backendNativeId = native;
    r.title = "Aufnahme";
    r.path = "/private/recording";
    return r;
}
RequestSecurityContext deviceContext() {
    RequestSecurityContext c;
    c.authenticationState = AuthenticationState::Authenticated;
    c.actor = {"actor-device", ActorType::Service, "TV", true};
    c.device = DeviceIdentity{"device-1", true};
    c.credential = CredentialIdentity{"credential-1", true, false, false};
    c.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    c.grants = {{"media.recording.play", "backend-a"}};
    return c;
}
void expect(const PublicRecordingDevicePlaybackAdmissionResult& result,
    Status status) {
    assert(result.status == status);
    if (status != Status::ready) {
        assert(result.recording.id.empty());
        assert(result.recording.backendNativeId.empty());
        assert(result.recording.path.empty());
    }
}
}

int main() {
    Database db;
    assert(db.open(":memory:"));
    PublicRecordingIdentityRepository identities(db);
    VdrRecordingCacheRepository cache(db);
    assert(identities.ensureSchema() && cache.ensureSchema());
    const std::string native = "/private/A/2026.rec";
    const std::string moved = "/private/B/2026.rec";
    assert(cache.replaceRecordingsForBackend("backend-a",
        {recording("media-internal-id", native)}));
    const auto publicId = identities.resolveOrCreate("backend-a", native);
    assert(publicId);
    PublicRecordingPlaybackTargetResolver resolver(identities, cache);
    bool online = true;
    int checks = 0;
    PublicRecordingDevicePlaybackAdmission admission(resolver,
        [&](const std::string& backend) {
            ++checks;
            return online && backend == "backend-a";
        });

    auto valid = deviceContext();
    const auto allowed = admission.admit(valid, "backend-a", *publicId);
    expect(allowed, Status::ready);
    assert(allowed.recording.id == "media-internal-id");
    assert(allowed.recording.backendNativeId == native);
    assert(allowed.recording.backendId == "backend-a");
    assert(checks == 1);

    auto anon = valid;
    anon.authenticationState = AuthenticationState::Anonymous;
    expect(admission.admit(anon, "backend-a", *publicId),
        Status::unauthenticated);
    auto expired = valid;
    expired.credential->expired = true;
    expect(admission.admit(expired, "backend-a", *publicId),
        Status::unauthenticated);
    auto revoked = valid;
    revoked.device->active = false;
    expect(admission.admit(revoked, "backend-a", *publicId),
        Status::unauthenticated);
    auto revokedActor = valid;
    revokedActor.actor.active = false;
    expect(admission.admit(revokedActor, "backend-a", *publicId),
        Status::unauthenticated);

    auto browser = valid;
    browser.device.reset();
    browser.credential.reset();
    browser.session = SessionIdentity{"browser-session"};
    browser.actor.type = ActorType::User;
    expect(admission.admit(browser, "backend-a", *publicId),
        Status::forbidden);
    auto fakeService = valid;
    fakeService.device.reset();
    expect(admission.admit(fakeService, "backend-a", *publicId),
        Status::forbidden);
    auto mixed = valid;
    mixed.session = SessionIdentity{"browser-session"};
    expect(admission.admit(mixed, "backend-a", *publicId),
        Status::forbidden);

    auto viewOnly = valid;
    viewOnly.grants = {{"recordings.view", "backend-a"}};
    expect(admission.admit(viewOnly, "backend-a", *publicId),
        Status::forbidden);
    auto foreignGrant = valid;
    foreignGrant.grants = {{"media.recording.play", "backend-b"}};
    expect(admission.admit(foreignGrant, "backend-a", *publicId),
        Status::forbidden);
    auto unresolved = valid;
    unresolved.permissionGrantResolution =
        PermissionGrantResolutionState::NotRequired;
    expect(admission.admit(unresolved, "backend-a", *publicId),
        Status::forbidden);
    auto unavailableGrants = valid;
    unavailableGrants.permissionGrantResolution =
        PermissionGrantResolutionState::Unavailable;
    expect(admission.admit(unavailableGrants, "backend-a", *publicId),
        Status::unavailable);

    const int beforeDenied = checks;
    expect(admission.admit(foreignGrant, "backend-a", *publicId),
        Status::forbidden);
    assert(checks == beforeDenied); // no backend probe on denial

    expect(admission.admit(valid, "backend-a", "rec_bad"),
        Status::invalidRequest);
    expect(admission.admit(valid, "backend-a",
        std::string("rec_") + std::string(32, '0')), Status::notFound);
    expect(admission.admit(valid, "../backend-a", *publicId),
        Status::forbidden);
    // Even explicitly granted access to backend-b never aliases backend-a.
    auto both = valid;
    both.grants.push_back({"media.recording.play", "backend-b"});
    expect(admission.admit(both, "backend-b", *publicId),
        Status::notFound);
    online = false;
    expect(admission.admit(valid, "backend-a", *publicId),
        Status::unavailable);
    online = true;

    assert(cache.replaceRecordingsForBackend("backend-a", {}));
    expect(admission.admit(valid, "backend-a", *publicId),
        Status::notFound);
    assert(identities.rebindAfterVerifiedMove("backend-a", native, moved));
    assert(cache.replaceRecordingsForBackend("backend-a",
        {recording("moved-media-id", moved)}));
    expect(admission.admit(valid, "backend-a", *publicId), Status::ready);
    assert(identities.removeAfterVerifiedDeletion("backend-a", moved));
    expect(admission.admit(valid, "backend-a", *publicId),
        Status::notFound);
    PublicRecordingDevicePlaybackAdmission noBackendResolver(resolver, {});
    expect(noBackendResolver.admit(valid, "backend-a", *publicId),
        Status::unavailable);
}

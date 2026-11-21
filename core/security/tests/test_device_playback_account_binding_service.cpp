#include "DevicePlaybackAccountBindingService.h"
#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"
#include <cassert>
#include <string>

int main() {
    Database database;
    assert(database.open(":memory:"));
    SecurityIdentityRepository identities(database);
    SecurityIdentityProvisioningRepository provisioning(database);
    HumanAccountRepository accounts(database);
    SecurityPermissionGrantRepository grants(database);
    AccountabilityEventRepository audit(database);
    assert(identities.ensureSchema());
    assert(accounts.ensureSchema());
    assert(grants.ensureSchema());
    assert(audit.ensureSchema());
    DevicePlaybackAccountBindingService links(
        database,identities,accounts,grants,audit);
    assert(links.ensureSchema());

    auto createHuman = [&](const std::string& actorId,
                            const std::string& accountId) {
        auto lease=database.acquireTransactionLease();
        assert(database.execute("BEGIN IMMEDIATE;"));
        assert(provisioning.ensureHumanCredentialInActiveTransaction(
            actorId, actorId, "credential_"+actorId, "human-password"));
        assert(accounts.ensureAccountInActiveTransaction(
            accountId,actorId,actorId));
        assert(database.execute("COMMIT;"));
    };
    createHuman("actor-admin","account-admin");
    createHuman("actor-alice","account-alice");
    createHuman("actor-bob","account-bob");
    assert(provisioning.ensureTechnicalIdentity(
        "actor-tv",ActorType::Service,"Living room TV",
        "device-tv","Living room","credential-tv","device-app"));

    assert(grants.ensureGrant("actor-tv","media.recording.play","default"));
    assert(grants.ensureGrant("actor-alice","media.recording.play","default"));
    assert(grants.ensureGrant("actor-bob","media.recording.play","default"));
    assert(grants.ensureGrant("actor-admin","devices.grants.modify","*"));

    RequestSecurityContext tv;
    tv.authenticationState=AuthenticationState::Authenticated;
    tv.actor={"actor-tv",ActorType::Service,"Living room",true};
    tv.device=DeviceIdentity{"device-tv",true};
    tv.credential=CredentialIdentity{"credential-tv",true,false,false};
    tv.permissionGrantResolution=PermissionGrantResolutionState::Resolved;
    tv.grants={{"media.recording.play","default"}};

    auto current=links.resolve(tv,"default");
    assert(current.status==DevicePlaybackActorStatus::deviceOnly);
    assert(current.actorId=="actor-tv");
    assert(links.read("device-tv").binding.revision==0);
    assert(links.set("actor-tv","device-tv","account-alice",0,
        "unprivileged-request").status==DevicePlaybackAccountStatus::unavailable);
    const auto first=links.set("actor-admin","device-tv","account-alice",0,
        "admin-binding-first");
    assert(first.status==DevicePlaybackAccountStatus::ok);
    assert(first.binding.revision==1);
    assert(first.binding.accountId=="account-alice");
    current=links.resolve(tv,"default");
    assert(current.status==DevicePlaybackActorStatus::linked);
    assert(current.actorId=="actor-alice");
    assert(current.accountId=="account-alice");

    // A revoked Human grant immediately removes Device access to the
    // associated Human history even while the Device grant is still valid.
    assert(grants.revokeGrant("actor-alice","media.recording.play","default"));
    assert(links.resolve(tv,"default").status==DevicePlaybackActorStatus::forbidden);
    assert(grants.ensureGrant("actor-alice","media.recording.play","default"));
    assert(links.resolve(tv,"default").actorId=="actor-alice");
    assert(links.resolve(tv,"wrong").status==DevicePlaybackActorStatus::forbidden);

    // An independent owner cannot overwrite without the latest revision.
    assert(links.set("actor-admin","device-tv","account-bob",0,
        "stale-admin-write").status==DevicePlaybackAccountStatus::revisionConflict);
    const auto changed=links.set("actor-admin","device-tv","account-bob",1,
        "admin-binding-second");
    assert(changed.status==DevicePlaybackAccountStatus::ok);
    assert(changed.binding.revision==2);
    assert(links.resolve(tv,"default").actorId=="actor-bob");
    assert(links.resolve(tv,"default").actorId!="actor-alice");

    assert(grants.revokeGrant("actor-tv","media.recording.play","default"));
    assert(links.resolve(tv,"default").status==DevicePlaybackActorStatus::forbidden);
    assert(grants.ensureGrant("actor-tv","media.recording.play","default"));

    // Explicit unassignment is revisioned: it never silently resurrects
    // a previously selected Human playback owner.
    const auto unlinked=links.set("actor-admin","device-tv","",2,
        "admin-binding-unlink");
    assert(unlinked.status==DevicePlaybackAccountStatus::ok);
    assert(unlinked.binding.revision==3);
    assert(links.resolve(tv,"default").actorId=="actor-tv");
    assert(links.set("actor-admin","device-tv","account-alice",1,
        "replay-old-link").status==DevicePlaybackAccountStatus::revisionConflict);

    assert(links.set("actor-admin","device-tv","missing-account",3,
        "no-identity").status==DevicePlaybackAccountStatus::notFound);
    assert(links.read("device-tv").binding.revision==3);
    assert(!audit.listAll().empty());

    // Administrator grant revocation between HTTP admission and the
    // transaction is rechecked; no stale admin may relink this Device.
    assert(grants.revokeGrant("actor-admin","devices.grants.modify","*"));
    assert(links.set("actor-admin","device-tv","account-alice",3,
        "revoked-admin").status==DevicePlaybackAccountStatus::invalid);
    assert(links.read("device-tv").binding.revision==3);

    assert(identities.revokeDevice("device-tv"));
    assert(links.resolve(tv,"default").status==DevicePlaybackActorStatus::forbidden);
    assert(links.read("device-tv").status==DevicePlaybackAccountStatus::notFound);
    return 0;
}

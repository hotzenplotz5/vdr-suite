#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "DeviceCredentialAuthenticator.h"
#include "DeviceCredentialRotationService.h"
#include "DeviceCredentialVerifierRepository.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <cassert>
#include <crypt.h>
#include <string>

namespace {
const std::string actorId = "actor_mu10e3";
const std::string deviceId = "device_mu10e3";
const std::string credentialId = "credential_mu10e3";
const std::string secret = "MU10E3ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefgh";
struct Fixture {
    Database db;
    SecurityIdentityRepository identities{db};
    SecurityIdentityProvisioningRepository provisioning{db};
    DeviceCredentialVerifierRepository verifiers{db};
    SecurityPermissionGrantRepository grants{db};
    AccountabilityEventRepository audit{db};
    DeviceCredentialRotationService service{db,identities,verifiers,audit};
    DeviceCredentialAuthenticator auth{verifiers,identities,grants};
    Fixture() {
        assert(db.open(":memory:"));
        assert(identities.ensureSchema());
        assert(verifiers.ensureSchema());
        assert(grants.ensureSchema());
        assert(audit.ensureSchema());
        assert(provisioning.ensureTechnicalIdentity(
            actorId, ActorType::Service, "Test TV",
            deviceId, "Test TV", credentialId, "device-app"));
        crypt_data state{};
        const char* hash = crypt_r(secret.c_str(),
            "$6$rounds=10000$rotationtestsalt$", &state);
        assert(hash);
        assert(db.execute("BEGIN IMMEDIATE;"));
        assert(verifiers.insertInActiveTransaction(credentialId,deviceId,hash));
        assert(db.execute("COMMIT;"));
    }
    bool login(const std::string& id,const std::string& key) {
        return auth.authenticate({{"Authorization",
            std::string(DeviceCredentialAuthenticator::Scheme)+id+"."+key}},
            "rotation-test","").authenticated();
    }
    DeviceCredentialRotationRequest request() const {
        return {"admin.mu10e3",deviceId,credentialId,
            "device-credential-lifecycle:"+credentialId+":active",
            "test-rotation",""};
    }
};
}
int main() {
    {
        Fixture f;
        assert(f.login(credentialId,secret));
        auto result=f.service.rotate(f.request());
        assert(result.status==DeviceCredentialRotationStatus::rotated);
        assert(result.issued);
        assert(result.issued->actorId==actorId);
        assert(result.issued->deviceId==deviceId);
        assert(result.issued->credentialId!=credentialId);
        assert(result.issued->credentialSecret.size()==64);
        assert(!f.login(credentialId,secret));
        assert(f.login(result.issued->credentialId,result.issued->credentialSecret));
        const auto newCredential=f.identities.findCredential(result.issued->credentialId);
        assert(newCredential && newCredential->rotatedFromCredentialId==credentialId);
        assert(f.service.rotate(f.request()).status==DeviceCredentialRotationStatus::stateConflict);
        auto replay=f.request();
        replay.previousCredentialId=result.issued->credentialId;
        replay.expectedResourceRevision="device-credential-lifecycle:"+
            replay.previousCredentialId+":active";
        auto next=f.service.rotate(replay);
        assert(next.status==DeviceCredentialRotationStatus::rotated);
        assert(next.issued);
        assert(!f.login(result.issued->credentialId,result.issued->credentialSecret));
        assert(f.login(next.issued->credentialId,next.issued->credentialSecret));
    }
    {
        Fixture f;
        auto req=f.request();
        req.previousCredentialId="not_owned";
        req.expectedResourceRevision="device-credential-lifecycle:not_owned:active";
        assert(f.service.rotate(req).status==DeviceCredentialRotationStatus::notFound);
        assert(f.login(credentialId,secret));
    }
    {
        Fixture f;
        assert(f.identities.revokeDevice(deviceId));
        assert(f.service.rotate(f.request()).status==DeviceCredentialRotationStatus::stateConflict);
        assert(!f.login(credentialId,secret));
    }
    {
        Fixture f;
        assert(f.identities.revokeCredential(credentialId));
        assert(f.service.rotate(f.request()).status==DeviceCredentialRotationStatus::stateConflict);
    }
    {
        Fixture f;
        assert(f.db.execute(
            "CREATE TRIGGER deny_rotation_audit BEFORE INSERT ON accountability_events "
            "BEGIN SELECT RAISE(ABORT, 'audit failed'); END;"));
        assert(f.service.rotate(f.request()).status==DeviceCredentialRotationStatus::unavailable);
        assert(f.login(credentialId,secret));
        assert(!f.identities.findCredential("credential_device_never_exists"));
    }
    {
        Fixture f;
        auto req=f.request();
        req.expectedResourceRevision="device-credential-lifecycle:other:active";
        assert(f.service.rotate(req).status==DeviceCredentialRotationStatus::invalid);
        assert(f.login(credentialId,secret));
    }
    return 0;
}

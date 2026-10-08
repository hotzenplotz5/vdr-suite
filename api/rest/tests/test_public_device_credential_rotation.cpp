#include "PublicApiRuntime.h"

#include <cassert>
#include <string>

int main() {
    auto& runtime = PublicApiRuntime::instance();
    runtime.resetDeviceCredentialRotation();
    const std::string path =
        "/api/v1/devices/device_mu10e3/credentials/credential_mu10e3/rotate";
    bool called = false;
    auto status = PublicDeviceCredentialRotationStatus::rotated;
    runtime.registerDeviceCredentialRotation(
        [&](const PublicDeviceCredentialRotationRequest& input) {
            called = true;
            assert(input.actorRef == "admin_mu10e3");
            assert(input.deviceId == "device_mu10e3");
            assert(input.credentialId == "credential_mu10e3");
            assert(input.expectedResourceRevision ==
                "device-credential-lifecycle:credential_mu10e3:active");
            PublicDeviceCredentialRotationResult result;
            result.status = status;
            result.deviceId = "device_mu10e3";
            result.actorId = "actor_mu10e3";
            result.credentialId = "credential_device_new";
            result.credentialSecret = "once_only";
            return result;
        });
    const std::string etag = "\"vsr-6465766963652d63726564656e7469616c2d6c6966656379636c653a63726564656e7469616c5f6d75313065333a616374697665\"";
    ApiResponse response;
    assert(runtime.tryHandlePost(path,"mu10e3-anon","",response,"{}",
        "","", "", "application/json"));
    assert(response.statusCode == 401);
    assert(!called);
    assert(runtime.tryHandlePost(path,"mu10e3-no-etag","",response,"{}",
        "admin_mu10e3","", "", "application/json"));
    assert(response.statusCode == 428);
    assert(!called);
    assert(runtime.tryHandlePost(path,"mu10e3-bad-body","",response,
        "{\"secret\":\"wrong\"}","admin_mu10e3",etag,"","application/json"));
    assert(response.statusCode == 422);
    assert(!called);
    assert(runtime.tryHandlePost(path,"mu10e3-success","",response,"{}",
        "admin_mu10e3",etag,"","application/json"));
    assert(response.statusCode == 200);
    assert(called);
    assert(response.body.find("\"credentialSecret\":\"once_only\"") !=
        std::string::npos);
    assert(response.headers.at("Cache-Control") == "no-store");
    called = false;
    status = PublicDeviceCredentialRotationStatus::stateConflict;
    assert(runtime.tryHandlePost(path,"mu10e3-replay","",response,"{}",
        "admin_mu10e3",etag,"","application/json"));
    assert(response.statusCode == 412);
    assert(called);
    assert(response.body.find("once_only") == std::string::npos);
    runtime.resetDeviceCredentialRotation();
}

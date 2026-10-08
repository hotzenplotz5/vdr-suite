#include "PublicApiRuntime.h"

#include <cassert>
#include <string>

namespace
{
const std::string DeviceId = "device_mu10e2";
const std::string CredentialId = "credential_mu10e2";
const std::string DevicePath = "/api/v1/devices/" + DeviceId + "/lifecycle";
const std::string CredentialPath = "/api/v1/devices/" + DeviceId +
    "/credentials/" + CredentialId + "/lifecycle";
PublicDeviceLifecycleResource deviceResource(bool revoked)
{
    PublicDeviceLifecycleResource item;
    item.deviceId = DeviceId;
    item.actorId = "actor_mu10e2";
    item.active = !revoked;
    item.revoked = revoked;
    item.resourceRevision = "device-lifecycle:" + DeviceId +
        (revoked ? ":revoked" : ":active");
    return item;
}
PublicDeviceLifecycleResource credentialResource(bool revoked)
{
    auto item = deviceResource(revoked);
    item.credentialId = CredentialId;
    item.resourceRevision = "device-credential-lifecycle:" + CredentialId +
        (revoked ? ":revoked" : ":active");
    return item;
}
}

int main()
{
    auto& runtime = PublicApiRuntime::instance();
    runtime.resetDeviceLifecycleLookup();
    runtime.resetDeviceLifecycleMutation();

    bool mutatedDevice = false;
    bool mutatedCredential = false;
    PublicDeviceLifecycleStatus resultStatus = PublicDeviceLifecycleStatus::ok;
    runtime.registerDeviceLifecycleLookup(
        [](const std::string& deviceId, const std::string& credentialId) {
            assert(deviceId == DeviceId);
            PublicDeviceLifecycleResult result;
            result.status = PublicDeviceLifecycleStatus::ok;
            result.resource = credentialId.empty()
                ? deviceResource(false) : credentialResource(false);
            if (!credentialId.empty()) assert(credentialId == CredentialId);
            return result;
        });
    runtime.registerDeviceLifecycleMutation(
        [&](const PublicDeviceLifecycleMutationRequest& input) {
            assert(input.actorRef == "admin_actor");
            assert(input.deviceId == DeviceId);
            assert(input.requestId.find("mu10e2-") == 0U);
            PublicDeviceLifecycleResult result;
            result.status = resultStatus;
            if (input.credentialId.empty())
            {
                mutatedDevice = true;
                assert(input.expectedResourceRevision ==
                    deviceResource(false).resourceRevision);
                result.resource = deviceResource(true);
            }
            else
            {
                mutatedCredential = true;
                assert(input.credentialId == CredentialId);
                assert(input.expectedResourceRevision ==
                    credentialResource(false).resourceRevision);
                result.resource = credentialResource(true);
            }
            return result;
        });

    ApiResponse anonymous;
    assert(runtime.tryHandleGet(DevicePath, "", "mu10e2-anon",
        "", anonymous));
    assert(anonymous.statusCode == 401);

    ApiResponse read;
    assert(runtime.tryHandleGet(DevicePath, "admin_actor", "mu10e2-read",
        "", read));
    assert(read.statusCode == 200);
    assert(read.body.find("\"deviceId\":\"device_mu10e2\"") !=
        std::string::npos);
    assert(read.body.find("\"active\":true") != std::string::npos);
    assert(read.body.find("credentialSecret") == std::string::npos);
    assert(read.headers.count("ETag") == 1U);

    ApiResponse notModified;
    assert(runtime.tryHandleGet(DevicePath, "admin_actor",
        "mu10e2-conditional", "", notModified, read.headers.at("ETag")));
    assert(notModified.statusCode == 304);

    ApiResponse credentialRead;
    assert(runtime.tryHandleGet(CredentialPath, "admin_actor",
        "mu10e2-credential-read", "", credentialRead));
    assert(credentialRead.statusCode == 200);
    assert(credentialRead.body.find(
        "\"credentialId\":\"credential_mu10e2\"") != std::string::npos);

    ApiResponse noPrecondition;
    assert(runtime.tryHandlePost(DevicePath, "mu10e2-no-etag", "",
        noPrecondition, "{}", "admin_actor", "",
        "", "application/json"));
    assert(noPrecondition.statusCode == 428);
    assert(!mutatedDevice);

    ApiResponse forbiddenBody;
    assert(runtime.tryHandlePost(DevicePath, "mu10e2-body", "",
        forbiddenBody, "{\"active\":false}", "admin_actor",
        read.headers.at("ETag"), "", "application/json"));
    assert(forbiddenBody.statusCode == 422);
    assert(!mutatedDevice);

    ApiResponse wrongETag;
    assert(runtime.tryHandlePost(DevicePath, "mu10e2-wrong-etag", "",
        wrongETag, "{}", "admin_actor",
        credentialRead.headers.at("ETag"), "", "application/json"));
    assert(wrongETag.statusCode == 400);
    assert(!mutatedDevice);

    ApiResponse revokeDevice;
    assert(runtime.tryHandlePost(DevicePath, "mu10e2-revoke-device", "",
        revokeDevice, "{}", "admin_actor", read.headers.at("ETag"),
        "", "application/json"));
    assert(revokeDevice.statusCode == 200);
    assert(mutatedDevice);
    assert(revokeDevice.body.find("\"revoked\":true") != std::string::npos);

    ApiResponse revokeCredential;
    assert(runtime.tryHandlePost(CredentialPath,
        "mu10e2-revoke-credential", "", revokeCredential, "{}",
        "admin_actor", credentialRead.headers.at("ETag"),
        "", "application/json"));
    assert(revokeCredential.statusCode == 200);
    assert(mutatedCredential);

    resultStatus = PublicDeviceLifecycleStatus::revisionConflict;
    ApiResponse stale;
    assert(runtime.tryHandlePost(DevicePath, "mu10e2-stale", "",
        stale, "{}", "admin_actor", read.headers.at("ETag"),
        "", "application/json"));
    assert(stale.statusCode == 412);

    ApiResponse unsupported;
    assert(runtime.tryHandleUnsupportedMethod(
        "DELETE", CredentialPath, "mu10e2-unsupported", "",
        unsupported));
    assert(unsupported.statusCode == 405);

    runtime.resetDeviceLifecycleMutation();
    runtime.resetDeviceLifecycleLookup();
}

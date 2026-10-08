#include "PublicApiRuntime.h"

#include <cassert>
#include <string>

namespace
{
const std::string RevisionA =
    "grant-set:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
const std::string RevisionB =
    "grant-set:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
const std::string Path = "/api/v1/devices/device_10001/grants";
PublicDeviceGrantSetResource grantSet(const std::string& revision)
{
    PublicDeviceGrantSetResource result;
    result.deviceId = "device_10001";
    result.actorId = "actor_device_10001";
    result.resourceRevision = revision;
    result.grants.push_back(
        PublicAccountGrantItem{"channels.view", "default"});
    return result;
}
}
int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetDeviceGrantLookup();
    runtime.resetDeviceGrantMutation();
    runtime.registerDeviceGrantLookup([](const std::string& deviceId) {
        assert(deviceId == "device_10001");
        PublicDeviceGrantLookupResult response;
        response.status = PublicDeviceGrantStatus::ok;
        response.grantSet = grantSet(RevisionA);
        return response;
    });
    bool called = false;
    PublicDeviceGrantStatus mutationStatus = PublicDeviceGrantStatus::ok;
    runtime.registerDeviceGrantMutation(
        [&called, &mutationStatus](const PublicDeviceGrantMutationRequest& request) {
            called = true;
            assert(request.actorRef == "admin_actor");
            assert(request.deviceId == "device_10001");
            assert(request.permission == "channels.view");
            assert(request.backendId == "default");
            assert(!request.active);
            assert(request.expectedResourceRevision == RevisionA);
            PublicDeviceGrantMutationResult response;
            response.status = mutationStatus;
            response.grantSet = grantSet(RevisionB);
            return response;
        });

    ApiResponse read;
    assert(runtime.tryHandleGet(
        Path, "admin_actor", "mu10e-get", "", read));
    assert(read.statusCode == 200);
    assert(read.headers.count("ETag") == 1U);
    assert(read.body.find("\"deviceId\":\"device_10001\"") !=
        std::string::npos);
    assert(read.body.find("\"actorId\":\"actor_device_10001\"") !=
        std::string::npos);
    assert(read.body.find("role.admin") == std::string::npos);
    assert(read.body.find("credentialSecret") == std::string::npos);

    ApiResponse notModified;
    assert(runtime.tryHandleGet(
        Path, "admin_actor", "mu10e-304", "", notModified,
        read.headers.at("ETag")));
    assert(notModified.statusCode == 304);

    ApiResponse anonymous;
    assert(runtime.tryHandleGet(
        Path, "", "mu10e-anonymous", "", anonymous));
    assert(anonymous.statusCode == 401);

    ApiResponse badQuery;
    assert(runtime.tryHandleGet(
        Path + "?extra=1", "admin_actor", "mu10e-query",
        "", badQuery));
    assert(badQuery.statusCode == 400);

    const std::string body =
        "{\"permission\":\"channels.view\","
        "\"backendId\":\"default\",\"active\":false}";

    ApiResponse missingRevision;
    assert(runtime.tryHandlePost(
        Path, "mu10e-missing-etag", "", missingRevision, body,
        "admin_actor", "", "", "application/json"));
    assert(missingRevision.statusCode == 428);
    assert(!called);

    ApiResponse mutation;
    assert(runtime.tryHandlePost(
        Path, "mu10e-mutate", "", mutation, body,
        "admin_actor", read.headers.at("ETag"), "",
        "application/json"));
    assert(mutation.statusCode == 200);
    assert(called);
    assert(mutation.headers.count("ETag") == 1U);

    mutationStatus = PublicDeviceGrantStatus::revisionConflict;
    ApiResponse revisionConflict;
    assert(runtime.tryHandlePost(
        Path, "mu10e-conflict", "", revisionConflict, body,
        "admin_actor", read.headers.at("ETag"), "",
        "application/json"));
    assert(revisionConflict.statusCode == 412);

    mutationStatus = PublicDeviceGrantStatus::invalid;
    ApiResponse rejectedGrant;
    assert(runtime.tryHandlePost(
        Path, "mu10e-invalid", "", rejectedGrant, body,
        "admin_actor", read.headers.at("ETag"), "",
        "application/json"));
    assert(rejectedGrant.statusCode == 422);

    ApiResponse unsupported;
    assert(runtime.tryHandleUnsupportedMethod(
        "DELETE", Path, "mu10e-method", "", unsupported));
    assert(unsupported.statusCode == 405);

    runtime.resetDeviceGrantMutation();
    runtime.resetDeviceGrantLookup();
    return 0;
}

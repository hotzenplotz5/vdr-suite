#include "PublicApiRuntime.h"

#include <cassert>
#include <string>

namespace
{
const std::string PairingId =
    "dpr_0123456789abcdef0123456789abcdef";
const std::string PairingPath =
    "/api/v1/device-pairings/" + PairingId;
const std::string PairingToken =
    "pairing-token-0123456789abcdef0123456789";

PublicDevicePairingAdministrativeResource adminResource(
    bool decided)
{
    PublicDevicePairingAdministrativeResource resource;
    resource.resource.pairingRequestId = PairingId;
    resource.resource.client.displayName = "Hisense 43A6K";
    resource.resource.client.clientKind = "vidaa";
    resource.resource.client.appVersion = "phase1";
    resource.resource.state =
        decided ? "approved" : "pending";
    resource.resource.expiresAt =
        "2099-01-01T00:10:00Z";
    resource.resource.pollIntervalSeconds = 3;
    resource.resourceRevision =
        "device-pairing:" + PairingId +
        (decided ? ":2" : ":1");
    if (decided)
    {
        resource.decidedByActorId = "admin-actor";
        resource.decidedAt = "2099-01-01T00:00:00Z";
    }
    return resource;
}
}

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetDeviceCredentialIssue();
    runtime.resetDevicePairingDecision();
    runtime.resetDevicePairingAdministrationLookup();
    runtime.resetDevicePairingAdministrationCollectionLookup();
    runtime.resetDevicePairingLookup();
    runtime.resetDevicePairingCreate();

    bool decided = false;

    runtime.registerDevicePairingCreate(
        [](const PublicDevicePairingCreateRequest& request)
        {
            assert(request.client.displayName == "Hisense 43A6K");
            assert(request.client.clientKind == "vidaa");
            assert(request.client.appVersion == "phase1");
            PublicDevicePairingCreateResult result;
            result.status =
                PublicDevicePairingCreateStatus::created;
            result.resource.pairingRequestId = PairingId;
            result.resource.client = request.client;
            result.resource.state = "pending";
            result.resource.expiresAt =
                "2099-01-01T00:10:00Z";
            result.resource.pollIntervalSeconds = 3;
            result.userCode = "ABCD-EFGH";
            result.pairingToken = PairingToken;
            return result;
        });

    runtime.registerDevicePairingLookup(
        [&decided](const PublicDevicePairingLookupRequest& request)
        {
            PublicDevicePairingLookupResult result;
            if (request.pairingToken == "wrong")
            {
                result.status =
                    PublicDevicePairingLookupStatus::unauthorized;
                return result;
            }
            assert(request.pairingRequestId == PairingId);
            assert(request.pairingToken == PairingToken);
            result.status =
                PublicDevicePairingLookupStatus::ok;
            result.resource =
                adminResource(decided).resource;
            return result;
        });

    runtime.registerDevicePairingAdministrationCollectionLookup(
        [&decided](
            const PublicDevicePairingAdministrationCollectionRequest&
                request)
        {
            assert(request.limit == 50U);
            PublicDevicePairingAdministrationCollectionResult result;
            result.status =
                PublicDevicePairingAdministrationStatus::ok;
            if (!decided)
                result.requests.push_back(adminResource(false));
            return result;
        });

    runtime.registerDevicePairingAdministrationLookup(
        [&decided](const std::string& pairingRequestId)
        {
            assert(pairingRequestId == PairingId);
            PublicDevicePairingAdministrationLookupResult result;
            result.status =
                PublicDevicePairingAdministrationStatus::ok;
            result.request = adminResource(decided);
            return result;
        });

    runtime.registerDevicePairingDecision(
        [&decided](const PublicDevicePairingDecisionRequest& request)
        {
            assert(request.actorRef == "admin-actor");
            assert(request.pairingRequestId == PairingId);
            assert(request.decision == "approve");
            PublicDevicePairingDecisionResult result;
            if (decided)
            {
                result.status =
                    request.expectedResourceRevision ==
                        "device-pairing:" + PairingId + ":1"
                    ? PublicDevicePairingAdministrationStatus::
                        revisionConflict
                    : PublicDevicePairingAdministrationStatus::
                        stateConflict;
                result.request = adminResource(true);
                return result;
            }
            assert(request.expectedResourceRevision ==
                "device-pairing:" + PairingId + ":1");
            decided = true;
            result.status =
                PublicDevicePairingAdministrationStatus::ok;
            result.request = adminResource(true);
            return result;
        });

    ApiResponse created;
    assert(runtime.tryHandlePost(
        "/api/v1/device-pairings",
        "mu10b-create",
        "mu10b-correlation",
        created,
        "{\"displayName\":\"Hisense 43A6K\","
        "\"clientKind\":\"vidaa\","
        "\"appVersion\":\"phase1\"}",
        "", "", "", "application/json", ""));
    assert(created.statusCode == 201);

    ApiResponse anonymousCollection;
    assert(runtime.tryHandleGet(
        "/api/v1/device-pairings",
        "",
        "mu10b-anon-list",
        "",
        anonymousCollection));
    assert(anonymousCollection.statusCode == 401);

    ApiResponse collection;
    assert(runtime.tryHandleGet(
        "/api/v1/device-pairings",
        "admin-actor",
        "mu10b-list",
        "",
        collection));
    assert(collection.statusCode == 200);
    assert(collection.body.find(PairingId) != std::string::npos);
    assert(collection.body.find(
        "\"status\":\"pending\"") != std::string::npos);
    assert(collection.body.find("pairingToken") ==
        std::string::npos);
    assert(collection.body.find("userCode") ==
        std::string::npos);

    ApiResponse anonymousItem;
    assert(runtime.tryHandleGet(
        PairingPath, "", "mu10b-anon-item", "", anonymousItem));
    assert(anonymousItem.statusCode == 401);

    ApiResponse adminItem;
    assert(runtime.tryHandleGet(
        PairingPath,
        "admin-actor",
        "mu10b-admin-item",
        "",
        adminItem));
    assert(adminItem.statusCode == 200);
    const std::string firstEtag =
        adminItem.headers.at("ETag");
    assert(!firstEtag.empty());
    assert(adminItem.body.find(
        "\"decidedByActorId\":null") != std::string::npos);

    ApiResponse missingPrecondition;
    assert(runtime.tryHandlePost(
        PairingPath,
        "mu10b-no-if-match",
        "",
        missingPrecondition,
        "{\"decision\":\"approve\"}",
        "admin-actor",
        "",
        "",
        "application/json",
        ""));
    assert(missingPrecondition.statusCode == 428);

    ApiResponse approved;
    assert(runtime.tryHandlePost(
        PairingPath,
        "mu10b-approve",
        "mu10b-correlation",
        approved,
        "{\"decision\":\"approve\"}",
        "admin-actor",
        firstEtag,
        "",
        "application/json",
        ""));
    assert(approved.statusCode == 200);
    assert(approved.headers.at("ETag") != firstEtag);
    assert(approved.body.find(
        "\"status\":\"approved\"") != std::string::npos);
    assert(approved.body.find(
        "\"decidedByActorId\":\"admin-actor\"") !=
        std::string::npos);

    ApiResponse stale;
    assert(runtime.tryHandlePost(
        PairingPath,
        "mu10b-stale",
        "",
        stale,
        "{\"decision\":\"approve\"}",
        "admin-actor",
        firstEtag,
        "",
        "application/json",
        ""));
    assert(stale.statusCode == 412);

    ApiResponse alreadyDecided;
    assert(runtime.tryHandlePost(
        PairingPath,
        "mu10b-repeat",
        "",
        alreadyDecided,
        "{\"decision\":\"approve\"}",
        "admin-actor",
        approved.headers.at("ETag"),
        "",
        "application/json",
        ""));
    assert(alreadyDecided.statusCode == 409);

    ApiResponse poll;
    assert(runtime.tryHandleGet(
        PairingPath,
        "",
        "mu10b-poll",
        "",
        poll,
        "",
        "",
        {},
        PairingToken));
    assert(poll.statusCode == 200);
    assert(poll.body.find(
        "\"status\":\"approved\"") != std::string::npos);
    assert(poll.body.find("decidedByActorId") ==
        std::string::npos);
    assert(poll.body.find("pairingToken") ==
        std::string::npos);

    ApiResponse wrong;
    assert(runtime.tryHandleGet(
        PairingPath, "", "mu10b-wrong", "", wrong,
        "", "", {}, "wrong"));
    assert(wrong.statusCode == 401);

    ApiResponse collectionAfter;
    assert(runtime.tryHandleGet(
        "/api/v1/device-pairings",
        "admin-actor",
        "mu10b-list-after",
        "",
        collectionAfter));
    assert(collectionAfter.statusCode == 200);
    assert(collectionAfter.body.find(
        "\"items\":[]") != std::string::npos);

    ApiResponse deleteCollection;
    assert(runtime.tryHandleUnsupportedMethod(
        "DELETE",
        "/api/v1/device-pairings",
        "mu10b-delete-collection",
        "",
        deleteCollection));
    assert(deleteCollection.statusCode == 405);
    assert(deleteCollection.headers.at("Allow") ==
        "GET, POST");

    ApiResponse deleteItem;
    assert(runtime.tryHandleUnsupportedMethod(
        "DELETE",
        PairingPath,
        "mu10b-delete-item",
        "",
        deleteItem));
    assert(deleteItem.statusCode == 405);
    assert(deleteItem.headers.at("Allow") ==
        "GET, POST");

    // MU.10C token-scoped issuance never requires an administrator cookie.
    int issuedCount = 0;
    runtime.registerDeviceCredentialIssue(
        [&issuedCount](const PublicDeviceCredentialIssueRequest& request)
        {
            assert(request.pairingRequestId == PairingId);
            PublicDeviceCredentialIssueResult result;
            if (request.pairingToken != PairingToken)
            {
                result.status = PublicDeviceCredentialIssueStatus::unauthorized;
                return result;
            }
            if (issuedCount++ > 0)
            {
                result.status = PublicDeviceCredentialIssueStatus::consumed;
                return result;
            }
            result.status = PublicDeviceCredentialIssueStatus::issued;
            result.actorId = "actor-device";
            result.deviceId = "device-tv";
            result.credentialId = "credential-tv";
            result.credentialSecret = "opaque-device-secret";
            return result;
        });

    const std::string credentialPath = PairingPath + "/credential";
    ApiResponse noToken;
    assert(runtime.tryHandlePost(credentialPath, "mu10c-no-token",
        "", noToken, "", "", "", "", "", ""));
    assert(noToken.statusCode == 401);

    ApiResponse invalidToken;
    assert(runtime.tryHandlePost(credentialPath, "mu10c-invalid-token",
        "", invalidToken, "", "", "", "", "", "", PairingToken + "-wrong"));
    assert(invalidToken.statusCode == 401);

    ApiResponse issuedCredential;
    assert(runtime.tryHandlePost(credentialPath, "mu10c-issued",
        "", issuedCredential, "", "", "", "", "", "", PairingToken));
    assert(issuedCredential.statusCode == 201);
    assert(issuedCredential.headers.at("Cache-Control") == "no-store");
    assert(issuedCredential.body.find("\"credentialSecret\":\"opaque-device-secret\"") !=
        std::string::npos);
    assert(issuedCredential.body.find("pairing-token") == std::string::npos);

    ApiResponse repeatedIssue;
    assert(runtime.tryHandlePost(credentialPath, "mu10c-replay",
        "", repeatedIssue, "", "", "", "", "", "", PairingToken));
    assert(repeatedIssue.statusCode == 409);
    assert(repeatedIssue.body.find("opaque-device-secret") ==
        std::string::npos);
    runtime.resetDeviceCredentialIssue();

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "",
        "mu10b-capabilities",
        "",
        capabilities));
    assert(capabilities.body.find(
        "\"id\":\"public-api.device-pairing-bootstrap\","
        "\"version\":1,\"availability\":\"available\"") !=
        std::string::npos);
    assert(capabilities.body.find(
        "\"id\":\"public-api.device-pairing-administration\","
        "\"version\":1,\"availability\":\"available\"") !=
        std::string::npos);

    runtime.resetDevicePairingDecision();
    runtime.resetDevicePairingAdministrationLookup();
    runtime.resetDevicePairingAdministrationCollectionLookup();
    runtime.resetDevicePairingLookup();
    runtime.resetDevicePairingCreate();
    return 0;
}

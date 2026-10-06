#include "PublicApiRuntime.h"

#include <cassert>
#include <string>

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetDevicePairingCreate();
    runtime.resetDevicePairingLookup();

    runtime.registerDevicePairingCreate(
        [](const PublicDevicePairingCreateRequest& request)
        {
            assert(request.client.displayName == "Hisense 43A6K");
            assert(request.client.clientKind == "vidaa");
            assert(request.client.appVersion == "phase1");
            assert(request.requestId == "mu10a-create-request");
            assert(request.correlationId == "mu10a-correlation");

            PublicDevicePairingCreateResult result;
            result.status = PublicDevicePairingCreateStatus::created;
            result.resource.pairingRequestId =
                "dpr_0123456789abcdef0123456789abcdef";
            result.resource.client = request.client;
            result.resource.state = "pending";
            result.resource.expiresAt = "2026-10-06T18:00:00Z";
            result.resource.pollIntervalSeconds = 3;
            result.userCode = "ABCD-EFGH";
            result.pairingToken =
                "pairing-token-0123456789abcdef0123456789";
            return result;
        });

    runtime.registerDevicePairingLookup(
        [](const PublicDevicePairingLookupRequest& request)
        {
            PublicDevicePairingLookupResult result;
            if (request.pairingToken == "wrong")
            {
                result.status =
                    PublicDevicePairingLookupStatus::unauthorized;
                return result;
            }
            if (request.pairingToken == "expired")
            {
                result.status =
                    PublicDevicePairingLookupStatus::expired;
                return result;
            }

            assert(request.pairingRequestId ==
                "dpr_0123456789abcdef0123456789abcdef");
            assert(request.pairingToken ==
                "pairing-token-0123456789abcdef0123456789");

            result.status = PublicDevicePairingLookupStatus::ok;
            result.resource.pairingRequestId = request.pairingRequestId;
            result.resource.client.displayName = "Hisense 43A6K";
            result.resource.client.clientKind = "vidaa";
            result.resource.client.appVersion = "phase1";
            result.resource.state = "pending";
            result.resource.expiresAt = "2026-10-06T18:00:00Z";
            result.resource.pollIntervalSeconds = 3;
            return result;
        });

    ApiResponse created;
    assert(runtime.tryHandlePost(
        "/api/v1/device-pairings",
        "mu10a-create-request",
        "mu10a-correlation",
        created,
        "{\"displayName\":\"Hisense 43A6K\",\"clientKind\":\"vidaa\",\"appVersion\":\"phase1\"}",
        "", "", "", "application/json", ""));
    assert(created.statusCode == 201);
    assert(created.headers.at("Location") ==
        "/api/v1/device-pairings/dpr_0123456789abcdef0123456789abcdef");
    assert(created.headers.at("Cache-Control") == "no-store");
    assert(created.body.find("\"userCode\":\"ABCD-EFGH\"") != std::string::npos);
    assert(created.body.find("\"pairingToken\":\"pairing-token-0123456789abcdef0123456789\"") != std::string::npos);
    assert(created.body.find("\"status\":\"pending\"") != std::string::npos);

    ApiResponse collectionGet;
    assert(runtime.tryHandleGet(
        "/api/v1/device-pairings", "", "mu10a-get-collection", "", collectionGet));
    assert(collectionGet.statusCode == 405);
    assert(collectionGet.headers.at("Allow") == "POST");

    const std::string itemPath =
        "/api/v1/device-pairings/dpr_0123456789abcdef0123456789abcdef";

    ApiResponse missingToken;
    assert(runtime.tryHandleGet(
        itemPath, "", "mu10a-poll-missing", "", missingToken));
    assert(missingToken.statusCode == 401);

    ApiResponse pending;
    assert(runtime.tryHandleGet(
        itemPath, "", "mu10a-poll", "mu10a-correlation", pending,
        "", "", {}, "pairing-token-0123456789abcdef0123456789"));
    assert(pending.statusCode == 200);
    assert(pending.body.find("\"status\":\"pending\"") != std::string::npos);
    assert(pending.body.find("userCode") == std::string::npos);
    assert(pending.body.find("pairingToken") == std::string::npos);

    ApiResponse wrong;
    assert(runtime.tryHandleGet(
        itemPath, "", "mu10a-poll-wrong", "", wrong,
        "", "", {}, "wrong"));
    assert(wrong.statusCode == 401);
    assert(wrong.body.find("\"code\":\"unauthorized\"") != std::string::npos);

    ApiResponse expired;
    assert(runtime.tryHandleGet(
        itemPath, "", "mu10a-poll-expired", "", expired,
        "", "", {}, "expired"));
    assert(expired.statusCode == 410);
    assert(expired.body.find("\"code\":\"pairing_expired\"") != std::string::npos);

    ApiResponse invalidBody;
    assert(runtime.tryHandlePost(
        "/api/v1/device-pairings", "mu10a-create-invalid", "", invalidBody,
        "{\"displayName\":\"TV\",\"clientKind\":\"vidaa\",\"unknown\":\"x\"}",
        "", "", "", "application/json", ""));
    assert(invalidBody.statusCode == 422);

    ApiResponse itemPost;
    assert(runtime.tryHandlePost(
        itemPath, "mu10a-item-post", "", itemPost, "{}",
        "", "", "", "application/json", ""));
    assert(itemPost.statusCode == 405);
    assert(itemPost.headers.at("Allow") == "GET");

    ApiResponse deleteCollection;
    assert(runtime.tryHandleUnsupportedMethod(
        "DELETE", "/api/v1/device-pairings",
        "mu10a-delete-collection", "", deleteCollection));
    assert(deleteCollection.statusCode == 405);
    assert(deleteCollection.headers.at("Allow") == "POST");

    ApiResponse deleteItem;
    assert(runtime.tryHandleUnsupportedMethod(
        "DELETE", itemPath, "mu10a-delete-item", "", deleteItem));
    assert(deleteItem.statusCode == 405);
    assert(deleteItem.headers.at("Allow") == "GET");

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities", "", "mu10a-capabilities", "", capabilities));
    assert(capabilities.body.find(
        "\"id\":\"public-api.device-pairing-bootstrap\",\"version\":1,\"availability\":\"available\"")
        != std::string::npos);

    runtime.resetDevicePairingLookup();
    runtime.resetDevicePairingCreate();
    return 0;
}

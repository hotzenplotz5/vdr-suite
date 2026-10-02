#include "PublicApiRuntime.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

namespace
{
std::string nextCursor(const std::string& body)
{
    const std::string marker = "\"nextCursor\":\"";
    const std::size_t start = body.find(marker);
    if (start == std::string::npos) return "";
    const std::size_t valueStart = start + marker.size();
    const std::size_t end = body.find('"', valueStart);
    if (end == std::string::npos) return "";
    return body.substr(valueStart, end - valueStart);
}

PublicAccountCollectionItem account(
    const std::string& accountId,
    const std::string& actorId,
    const std::string& displayName,
    bool active)
{
    PublicAccountCollectionItem item;
    item.accountId = accountId;
    item.actorId = actorId;
    item.displayName = displayName;
    item.active = active;
    return item;
}
}

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetAccountCollectionLookup();
    runtime.resetAccountLookup();
    runtime.resetAccountMutation();
    runtime.resetAccountCreate();

    const std::vector<PublicAccountCollectionItem> configured = {
        account("account-a", "actor-a", "Admin A", true),
        account("account-b", "actor-b", "User B", true),
        account("account-c", "actor-c", "Disabled C", false)};

    runtime.registerAccountCollectionLookup(
        [configured](const PublicAccountCollectionRequest& request)
        {
            PublicAccountCollectionResult result;
            result.status = PublicAccountCollectionStatus::ok;

            std::vector<PublicAccountCollectionItem> eligible;
            for (const auto& item : configured)
            {
                if (!request.afterAccountId.empty() &&
                    item.accountId <= request.afterAccountId)
                {
                    continue;
                }
                eligible.push_back(item);
            }

            result.hasMore = eligible.size() > request.limit;
            if (result.hasMore)
                eligible.resize(request.limit);
            result.accounts = std::move(eligible);
            return result;
        });

    runtime.registerAccountLookup(
        [](const std::string& accountId)
        {
            PublicAccountLookupResult result;
            if (accountId == "invalid")
            {
                result.status = PublicAccountLookupStatus::invalid;
                return result;
            }
            if (accountId == "unavailable")
            {
                result.status = PublicAccountLookupStatus::unavailable;
                return result;
            }
            if (accountId != "account-a")
            {
                result.status = PublicAccountLookupStatus::notFound;
                return result;
            }

            result.status = PublicAccountLookupStatus::ok;
            result.account.accountId = "account-a";
            result.account.actorId = "actor-a";
            result.account.displayName = "Admin A";
            result.account.active = true;
            result.account.resourceRevision = "account:7";
            return result;
        });

    PublicAccountMutationStatus forcedMutationStatus =
        PublicAccountMutationStatus::ok;
    runtime.registerAccountMutation(
        [&forcedMutationStatus](
            const PublicAccountMutationRequest& request)
        {
            PublicAccountMutationResult result;
            result.status = forcedMutationStatus;
            if (forcedMutationStatus !=
                PublicAccountMutationStatus::ok)
            {
                return result;
            }

            assert(request.actorRef == "actor:test");
            assert(request.accountId == "account-a");
            assert(request.expectedRevision == 7U);
            assert(request.requestId.find("mu6c-") == 0U);

            result.account.accountId = "account-a";
            result.account.actorId = "actor-a";
            result.account.displayName =
                request.kind ==
                    PublicAccountMutationKind::displayName
                ? request.displayName
                : "Admin A";
            result.account.active =
                request.kind ==
                    PublicAccountMutationKind::active
                ? request.active
                : true;
            result.account.resourceRevision =
                "account:8";
            result.revokedBrowserSessions =
                request.kind ==
                    PublicAccountMutationKind::active &&
                    !request.active
                ? 2U
                : 0U;
            return result;
        });

    PublicAccountCreateStatus forcedCreateStatus =
        PublicAccountCreateStatus::created;
    runtime.registerAccountCreate(
        [&forcedCreateStatus](
            const PublicAccountCreateRequest& request)
        {
            PublicAccountCreateResult result;
            result.status = forcedCreateStatus;
            if (forcedCreateStatus != PublicAccountCreateStatus::created &&
                forcedCreateStatus != PublicAccountCreateStatus::replayed)
            {
                return result;
            }

            assert(request.actorRef == "actor:test");
            assert(request.loginName == "viewer");
            assert(request.displayName == "Viewer");
            assert(request.password == "initial-password");
            assert(request.idempotencyKey == "idem-account-create-1");
            assert(request.requestId.find("mu6d-") == 0U);

            result.account.accountId = "account-created";
            result.account.actorId = "actor-created";
            result.account.displayName = "Viewer";
            result.account.active = true;
            result.account.resourceRevision = "account:1";
            return result;
        });

    ApiResponse root;
    assert(runtime.tryHandleGet(
        "/api/v1",
        "actor:test",
        "p2-accounts-root",
        "",
        root));
    assert(root.statusCode == 200);
    assert(root.body.find(
        "\"accounts\":\"/api/v1/accounts\"") != std::string::npos);

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor:test",
        "p2-accounts-capabilities",
        "",
        capabilities));
    assert(capabilities.statusCode == 200);
    assert(capabilities.body.find(
        "\"id\":\"public-api.accounts-read\"") != std::string::npos);
    assert(capabilities.body.find(
        "{\"id\":\"public-api.accounts-read\",\"version\":1,\"availability\":\"available\"}") !=
        std::string::npos);
    assert(capabilities.body.find(
        "{\"id\":\"public-api.accounts-lifecycle-mutation\",\"version\":1,\"availability\":\"available\"}") !=
        std::string::npos);
    assert(capabilities.body.find(
        "{\"id\":\"public-api.accounts-create\",\"version\":1,\"availability\":\"available\"}") !=
        std::string::npos);

    ApiResponse first;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts?limit=1&sort=accountId&order=asc",
        "actor:test",
        "p2-accounts-first",
        "p2-accounts-correlation",
        first));
    assert(first.statusCode == 200);
    assert(first.body.find("\"accountId\":\"account-a\"") != std::string::npos);
    assert(first.body.find("\"actorId\":\"actor-a\"") != std::string::npos);
    assert(first.body.find("\"displayName\":\"Admin A\"") != std::string::npos);
    assert(first.body.find("\"active\":true") != std::string::npos);
    assert(first.body.find("\"accountId\":\"account-b\"") == std::string::npos);
    assert(first.body.find("password") == std::string::npos);
    assert(first.body.find("credential") == std::string::npos);
    assert(first.body.find("session") == std::string::npos);
    assert(first.body.find("grant") == std::string::npos);
    assert(first.body.find("\"partial\":false") != std::string::npos);
    assert(first.body.find("\"hasMore\":true") != std::string::npos);
    assert(first.headers.at("Cache-Control") == "no-store");
    assert(first.headers.find("ETag") == first.headers.end());

    const std::string cursor = nextCursor(first.body);
    assert(!cursor.empty());
    assert(cursor.rfind("ac1_", 0U) == 0U);

    ApiResponse second;
    assert(runtime.tryHandleGet(
        std::string("/api/v1/accounts?limit=1&cursor=") + cursor,
        "actor:test",
        "p2-accounts-second",
        "",
        second));
    assert(second.statusCode == 200);
    assert(second.body.find("\"accountId\":\"account-b\"") != std::string::npos);

    ApiResponse item;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a",
        "actor:test",
        "mu6b-account-item",
        "mu6b-account-correlation",
        item));
    assert(item.statusCode == 200);
    assert(item.body.find("\"accountId\":\"account-a\"") != std::string::npos);
    assert(item.body.find("\"actorId\":\"actor-a\"") != std::string::npos);
    assert(item.body.find("\"displayName\":\"Admin A\"") != std::string::npos);
    assert(item.body.find("\"active\":true") != std::string::npos);
    assert(item.body.find("\"self\":\"/api/v1/accounts/account-a\"") !=
        std::string::npos);
    assert(item.body.find("resourceRevision") == std::string::npos);
    assert(item.body.find("password") == std::string::npos);
    assert(item.body.find("credential") == std::string::npos);
    assert(item.body.find("session") == std::string::npos);
    assert(item.body.find("grant") == std::string::npos);
    assert(item.headers.count("ETag") == 1U);
    assert(!item.headers.at("ETag").empty());
    const std::string itemEtag = item.headers.at("ETag");

    ApiResponse notModified;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a",
        "actor:test",
        "mu6b-account-item-304",
        "",
        notModified,
        itemEtag));
    assert(notModified.statusCode == 304);
    assert(notModified.body.empty());
    assert(notModified.headers.at("ETag") == itemEtag);

    ApiResponse malformedItemCondition;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a",
        "actor:test",
        "mu6b-account-item-bad-etag",
        "",
        malformedItemCondition,
        "not-an-etag"));
    assert(malformedItemCondition.statusCode == 400);

    ApiResponse missingItem;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/missing",
        "actor:test",
        "mu6b-account-item-missing",
        "",
        missingItem));
    assert(missingItem.statusCode == 404);

    ApiResponse invalidItem;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/invalid",
        "actor:test",
        "mu6b-account-item-invalid",
        "",
        invalidItem));
    assert(invalidItem.statusCode == 400);

    ApiResponse unavailableItem;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/unavailable",
        "actor:test",
        "mu6b-account-item-unavailable",
        "",
        unavailableItem));
    assert(unavailableItem.statusCode == 503);

    ApiResponse anonymousItem;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a",
        "",
        "mu6b-account-item-anonymous",
        "",
        anonymousItem));
    assert(anonymousItem.statusCode == 401);

    ApiResponse nestedItem;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a/private",
        "actor:test",
        "mu6b-account-item-nested",
        "",
        nestedItem));
    assert(nestedItem.statusCode == 404);

    ApiResponse malformed;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts?cursor=broken",
        "actor:test",
        "p2-accounts-bad-cursor",
        "",
        malformed));
    assert(malformed.statusCode == 400);

    for (const std::string& badQuery : {
             "limit=0",
             "limit=101",
             "sort=displayName",
             "order=desc",
             "offset=1",
             "backendId=default"})
    {
        ApiResponse bad;
        assert(runtime.tryHandleGet(
            std::string("/api/v1/accounts?") + badQuery,
            "actor:test",
            "p2-accounts-bad-query",
            "",
            bad));
        assert(bad.statusCode == 400);
    }

    runtime.resetAccountCollectionLookup();
    ApiResponse unavailable;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts",
        "actor:test",
        "p2-accounts-unavailable",
        "",
        unavailable));
    assert(unavailable.statusCode == 503);

    ApiResponse created;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts",
        "mu6d-account-create",
        "mu6d-account-correlation",
        created,
        "{\"loginName\":\"viewer\",\"displayName\":\"Viewer\",\"password\":\"initial-password\"}",
        "actor:test",
        "",
        "idem-account-create-1",
        "application/json"));
    assert(created.statusCode == 201);
    assert(created.headers.at("Location") ==
        "/api/v1/accounts/account-created");
    assert(created.headers.count("ETag") == 1U);
    assert(created.body.find("\"accountId\":\"account-created\"") !=
        std::string::npos);
    assert(created.body.find("\"actorId\":\"actor-created\"") !=
        std::string::npos);
    assert(created.body.find("\"displayName\":\"Viewer\"") !=
        std::string::npos);
    assert(created.body.find("\"active\":true") !=
        std::string::npos);
    assert(created.body.find("password") == std::string::npos);
    assert(created.body.find("credential") == std::string::npos);
    assert(created.body.find("grant") == std::string::npos);

    forcedCreateStatus = PublicAccountCreateStatus::replayed;
    ApiResponse replayed;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts",
        "mu6d-account-create-replay",
        "",
        replayed,
        "{\"displayName\":\"Viewer\",\"password\":\"initial-password\",\"loginName\":\"viewer\"}",
        "actor:test",
        "",
        "idem-account-create-1",
        "application/json"));
    assert(replayed.statusCode == 201);
    assert(replayed.headers.at("Location") ==
        "/api/v1/accounts/account-created");

    ApiResponse missingIdempotency;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts",
        "mu6d-account-create-idempotency-required",
        "",
        missingIdempotency,
        "{\"loginName\":\"viewer\",\"displayName\":\"Viewer\",\"password\":\"initial-password\"}",
        "actor:test",
        "",
        "",
        "application/json"));
    assert(missingIdempotency.statusCode == 400);

    ApiResponse invalidCreateShape;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts",
        "mu6d-account-create-shape",
        "",
        invalidCreateShape,
        "{\"loginName\":\"viewer\",\"displayName\":\"Viewer\"}",
        "actor:test",
        "",
        "idem-account-create-1",
        "application/json"));
    assert(invalidCreateShape.statusCode == 422);

    ApiResponse createMediaType;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts",
        "mu6d-account-create-content-type",
        "",
        createMediaType,
        "{\"loginName\":\"viewer\",\"displayName\":\"Viewer\",\"password\":\"initial-password\"}",
        "actor:test",
        "",
        "idem-account-create-1",
        "text/plain"));
    assert(createMediaType.statusCode == 415);

    forcedCreateStatus = PublicAccountCreateStatus::loginConflict;
    ApiResponse loginConflict;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts",
        "mu6d-account-create-login-conflict",
        "",
        loginConflict,
        "{\"loginName\":\"viewer\",\"displayName\":\"Viewer\",\"password\":\"initial-password\"}",
        "actor:test",
        "",
        "idem-account-create-1",
        "application/json"));
    assert(loginConflict.statusCode == 409);
    assert(loginConflict.body.find("\"code\":\"operation_conflict\"") !=
        std::string::npos);

    forcedCreateStatus =
        PublicAccountCreateStatus::idempotencyConflict;
    ApiResponse createIdempotencyConflict;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts",
        "mu6d-account-create-idempotency-conflict",
        "",
        createIdempotencyConflict,
        "{\"loginName\":\"viewer\",\"displayName\":\"Viewer\",\"password\":\"initial-password\"}",
        "actor:test",
        "",
        "idem-account-create-1",
        "application/json"));
    assert(createIdempotencyConflict.statusCode == 409);
    assert(createIdempotencyConflict.body.find(
        "\"code\":\"idempotency_conflict\"") != std::string::npos);

    forcedCreateStatus = PublicAccountCreateStatus::created;

    ApiResponse renamed;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a",
        "mu6c-account-display-name",
        "mu6c-account-correlation",
        renamed,
        "{\"displayName\":\"Renamed Admin\"}",
        "actor:test",
        itemEtag,
        "",
        "application/json"));
    assert(renamed.statusCode == 200);
    assert(renamed.body.find(
        "\"displayName\":\"Renamed Admin\"") !=
        std::string::npos);
    assert(renamed.body.find(
        "\"active\":true") !=
        std::string::npos);
    assert(renamed.headers.count("ETag") == 1U);
    assert(renamed.headers.at("ETag") != itemEtag);
    assert(renamed.body.find("resourceRevision") ==
        std::string::npos);

    ApiResponse deactivated;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a",
        "mu6c-account-deactivate",
        "",
        deactivated,
        "{\"active\":false}",
        "actor:test",
        itemEtag,
        "",
        "application/json"));
    assert(deactivated.statusCode == 200);
    assert(deactivated.body.find(
        "\"active\":false") !=
        std::string::npos);

    ApiResponse missingIfMatch;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a",
        "mu6c-account-if-match-required",
        "",
        missingIfMatch,
        "{\"displayName\":\"Renamed Admin\"}",
        "actor:test",
        "",
        "",
        "application/json"));
    assert(missingIfMatch.statusCode == 428);

    ApiResponse malformedIfMatch;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a",
        "mu6c-account-if-match-malformed",
        "",
        malformedIfMatch,
        "{\"displayName\":\"Renamed Admin\"}",
        "actor:test",
        "W/\"account:7\"",
        "",
        "application/json"));
    assert(malformedIfMatch.statusCode == 400);

    ApiResponse invalidShape;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a",
        "mu6c-account-closed-body",
        "",
        invalidShape,
        "{\"displayName\":\"Renamed Admin\",\"active\":false}",
        "actor:test",
        itemEtag,
        "",
        "application/json"));
    assert(invalidShape.statusCode == 422);

    ApiResponse unsupportedMediaType;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a",
        "mu6c-account-content-type",
        "",
        unsupportedMediaType,
        "{\"active\":true}",
        "actor:test",
        itemEtag,
        "",
        "text/plain"));
    assert(unsupportedMediaType.statusCode == 415);

    forcedMutationStatus =
        PublicAccountMutationStatus::revisionConflict;
    ApiResponse stale;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a",
        "mu6c-account-stale",
        "",
        stale,
        "{\"active\":true}",
        "actor:test",
        itemEtag,
        "",
        "application/json"));
    assert(stale.statusCode == 412);
    assert(stale.body.find(
        "\"code\":\"revision_conflict\"") !=
        std::string::npos);

    forcedMutationStatus =
        PublicAccountMutationStatus::finalAdministrator;
    ApiResponse finalAdministrator;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a",
        "mu6c-account-final-admin",
        "",
        finalAdministrator,
        "{\"active\":false}",
        "actor:test",
        itemEtag,
        "",
        "application/json"));
    assert(finalAdministrator.statusCode == 409);
    assert(finalAdministrator.body.find(
        "\"code\":\"operation_conflict\"") !=
        std::string::npos);

    forcedMutationStatus =
        PublicAccountMutationStatus::ok;

    for (const std::string& method :
         {std::string("PUT"),
          std::string("PATCH"),
          std::string("DELETE"),
          std::string("HEAD"),
          std::string("OPTIONS")})
    {
        ApiResponse mismatch;
        assert(runtime.tryHandleUnsupportedMethod(
            method,
            "/api/v1/accounts",
            "p2-accounts-method",
            "",
            mismatch));
        assert(mismatch.statusCode == 405);
        assert(mismatch.headers.at("Allow") == "GET, POST");

        ApiResponse itemMismatch;
        assert(runtime.tryHandleUnsupportedMethod(
            method,
            "/api/v1/accounts/account-a",
            "mu6b-account-item-method",
            "",
            itemMismatch));
        assert(itemMismatch.statusCode == 405);
        assert(itemMismatch.headers.at("Allow") == "GET, POST");
    }

    runtime.resetAccountLookup();
    runtime.resetAccountMutation();
    runtime.resetAccountCreate();
    return 0;
}

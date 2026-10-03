#include "PublicApiRuntime.h"
#include "PublicResourcePreconditions.h"

#include <cassert>
#include <string>

namespace
{
const std::string ActiveRevision =
    "session-lifecycle:" + std::string(64U, 'a');
const std::string TerminalRevision =
    "session-lifecycle:" + std::string(64U, 'b');
const std::string StaleRevision =
    "session-lifecycle:" + std::string(64U, 'c');

PublicAccountSessionResource resource(
    const std::string& revision,
    bool active,
    bool revoked)
{
    PublicAccountSessionResource value;
    value.accountId = "account-a";
    value.actorId = "actor-a";
    value.resourceRevision = revision;
    value.session.sessionId = "session-a";
    value.session.deviceId = "device-a";
    value.session.issuedFromCredentialId =
        "credential-human";
    value.session.active = active;
    value.session.revoked = revoked;
    value.session.expiresAt =
        "2099-01-01 00:00:00";
    value.session.lastSeenAt =
        "2026-10-03 20:00:00";
    value.session.createdAt =
        "2026-10-03 18:00:00";
    return value;
}
}

int main()
{
    PublicApiRuntime& runtime =
        PublicApiRuntime::instance();
    runtime.resetAccountSessionItemLookup();
    runtime.resetAccountSessionMutation();

    runtime.registerAccountSessionItemLookup(
        [](const std::string& accountId,
           const std::string& sessionId)
        {
            PublicAccountSessionLookupResult result;
            if (accountId != "account-a" ||
                sessionId != "session-a")
            {
                result.status =
                    PublicAccountSessionAdministrationStatus::notFound;
                return result;
            }

            result.status =
                PublicAccountSessionAdministrationStatus::ok;
            result.resource =
                resource(ActiveRevision, true, false);
            return result;
        });

    runtime.registerAccountSessionMutation(
        [](const PublicAccountSessionMutationRequest& request)
        {
            PublicAccountSessionMutationResult result;
            if (request.accountId != "account-a" ||
                request.sessionId != "session-a")
            {
                result.status =
                    PublicAccountSessionAdministrationStatus::notFound;
                return result;
            }

            if (request.expectedResourceRevision !=
                ActiveRevision)
            {
                result.status =
                    PublicAccountSessionAdministrationStatus::
                        revisionConflict;
                result.resource =
                    resource(ActiveRevision, true, false);
                return result;
            }

            result.status =
                PublicAccountSessionAdministrationStatus::ok;
            result.resource =
                resource(TerminalRevision, false, true);
            return result;
        });

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor-admin",
        "mu8b-capabilities",
        "",
        capabilities));
    assert(capabilities.statusCode == 200);
    assert(capabilities.body.find(
        "\"id\":\"public-api.accounts-session-revoke\"") !=
        std::string::npos);

    const std::string path =
        "/api/v1/accounts/account-a/sessions/session-a";

    ApiResponse item;
    assert(runtime.tryHandleGet(
        path,
        "actor-admin",
        "mu8b-get",
        "",
        item));
    assert(item.statusCode == 200);
    assert(item.headers.at("Cache-Control") ==
        "no-store");
    assert(item.headers.count("ETag") == 1U);
    assert(item.body.find(
        "\"sessionId\":\"session-a\"") !=
        std::string::npos);
    assert(item.body.find(
        "\"issuedFromCredentialId\":\"credential-human\"") !=
        std::string::npos);
    assert(item.body.find("resourceRevision") ==
        std::string::npos);
    for (const char* forbidden :
         {"browserCredentialId", "passwordHash",
          "sessionSecretHash", "csrfSecretHash",
          "tokenId"})
    {
        assert(item.body.find(forbidden) ==
            std::string::npos);
    }

    const std::string activeEtag =
        item.headers.at("ETag");

    ApiResponse conditional;
    assert(runtime.tryHandleGet(
        path,
        "actor-admin",
        "mu8b-conditional",
        "",
        conditional,
        activeEtag));
    assert(conditional.statusCode == 304);
    assert(conditional.headers.at("ETag") ==
        activeEtag);

    ApiResponse missingIfMatch;
    assert(runtime.tryHandlePost(
        path,
        "mu8b-missing",
        "",
        missingIfMatch,
        "{}",
        "actor-admin",
        "",
        "",
        "application/json"));
    assert(missingIfMatch.statusCode == 428);

    ApiResponse malformedIfMatch;
    assert(runtime.tryHandlePost(
        path,
        "mu8b-malformed",
        "",
        malformedIfMatch,
        "{}",
        "actor-admin",
        "W/\"bad\"",
        "",
        "application/json"));
    assert(malformedIfMatch.statusCode == 400);

    ApiResponse invalidBody;
    assert(runtime.tryHandlePost(
        path,
        "mu8b-body",
        "",
        invalidBody,
        "{\"x\":1}",
        "actor-admin",
        activeEtag,
        "",
        "application/json"));
    assert(invalidBody.statusCode == 422);

    const std::string staleEtag =
        vdrsuite::http::publicStrongEntityTag(
            StaleRevision);
    assert(!staleEtag.empty());

    ApiResponse stale;
    assert(runtime.tryHandlePost(
        path,
        "mu8b-stale",
        "",
        stale,
        "{}",
        "actor-admin",
        staleEtag,
        "",
        "application/json"));
    assert(stale.statusCode == 412);

    ApiResponse revoked;
    assert(runtime.tryHandlePost(
        path,
        "mu8b-revoke",
        "",
        revoked,
        "{}",
        "actor-admin",
        activeEtag,
        "",
        "application/json"));
    assert(revoked.statusCode == 200);
    assert(revoked.headers.count("ETag") == 1U);
    assert(revoked.headers.at("ETag") !=
        activeEtag);
    assert(revoked.body.find(
        "\"active\":false") !=
        std::string::npos);
    assert(revoked.body.find(
        "\"revoked\":true") !=
        std::string::npos);

    ApiResponse queryRejected;
    assert(runtime.tryHandleGet(
        path + "?x=1",
        "actor-admin",
        "mu8b-query",
        "",
        queryRejected));
    assert(queryRejected.statusCode == 400);

    ApiResponse putRejected;
    assert(runtime.tryHandleUnsupportedMethod(
        "PUT",
        path,
        "mu8b-put",
        "",
        putRejected));
    assert(putRejected.statusCode == 405);
    assert(putRejected.headers.at("Allow") ==
        "GET, POST");

    ApiResponse collectionPost;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/sessions",
        "mu8b-collection",
        "",
        collectionPost,
        "{}",
        "actor-admin",
        activeEtag,
        "",
        "application/json"));
    assert(collectionPost.statusCode == 405);
    assert(collectionPost.headers.at("Allow") ==
        "GET");

    runtime.resetAccountSessionMutation();
    runtime.resetAccountSessionItemLookup();
    return 0;
}

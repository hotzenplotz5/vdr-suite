#include "PublicApiRuntime.h"
#include "PublicResourcePreconditions.h"

#include <cassert>
#include <string>

namespace
{
const std::string ActiveRevision =
    "credential-lifecycle:" + std::string(64U, 'a');
const std::string TerminalRevision =
    "credential-lifecycle:" + std::string(64U, 'b');
const std::string StaleRevision =
    "credential-lifecycle:" + std::string(64U, 'c');

PublicAccountCredentialResource resource(
    const std::string& revision,
    bool active,
    bool revoked)
{
    PublicAccountCredentialResource value;
    value.accountId = "account-a";
    value.actorId = "actor-a";
    value.resourceRevision = revision;
    value.credential.credentialId = "credential-human";
    value.credential.credentialType = "human-password";
    value.credential.active = active;
    value.credential.revoked = revoked;
    value.credential.expiresAt = "";
    value.credential.createdAt = "2026-10-03 18:00:00";
    return value;
}
}

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetAccountCredentialItemLookup();
    runtime.resetAccountCredentialMutation();

    runtime.registerAccountCredentialItemLookup(
        [](const std::string& accountId,
           const std::string& credentialId)
        {
            PublicAccountCredentialLookupResult result;
            if (accountId != "account-a" ||
                credentialId != "credential-human")
            {
                result.status =
                    PublicAccountCredentialAdministrationStatus::notFound;
                return result;
            }

            result.status =
                PublicAccountCredentialAdministrationStatus::ok;
            result.resource =
                resource(ActiveRevision, true, false);
            return result;
        });

    int mutationMode = 0;
    runtime.registerAccountCredentialMutation(
        [&mutationMode](
            const PublicAccountCredentialMutationRequest& request)
        {
            PublicAccountCredentialMutationResult result;
            if (request.accountId != "account-a" ||
                request.credentialId != "credential-human")
            {
                result.status =
                    PublicAccountCredentialAdministrationStatus::notFound;
                return result;
            }

            if (mutationMode == 1)
            {
                result.status =
                    PublicAccountCredentialAdministrationStatus::
                        finalAdministrator;
                result.resource =
                    resource(ActiveRevision, true, false);
                return result;
            }

            if (request.expectedResourceRevision != ActiveRevision)
            {
                result.status =
                    PublicAccountCredentialAdministrationStatus::
                        revisionConflict;
                result.resource =
                    resource(ActiveRevision, true, false);
                return result;
            }

            result.status =
                PublicAccountCredentialAdministrationStatus::ok;
            result.resource =
                resource(TerminalRevision, false, true);
            return result;
        });

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor-admin",
        "mu8c-capabilities",
        "",
        capabilities));
    assert(capabilities.statusCode == 200);
    assert(capabilities.body.find(
        "\"id\":\"public-api.accounts-credential-revoke\"") !=
        std::string::npos);
    assert(capabilities.body.find(
        "\"id\":\"public-api.accounts-credential-revoke\","
        "\"version\":1,\"availability\":\"available\"") !=
        std::string::npos);

    const std::string path =
        "/api/v1/accounts/account-a/credentials/credential-human";

    ApiResponse item;
    assert(runtime.tryHandleGet(
        path,
        "actor-admin",
        "mu8c-get",
        "",
        item));
    assert(item.statusCode == 200);
    assert(item.headers.at("Cache-Control") == "no-store");
    assert(item.headers.count("ETag") == 1U);
    assert(item.body.find(
        "\"credentialId\":\"credential-human\"") !=
        std::string::npos);
    assert(item.body.find(
        "\"credentialType\":\"human-password\"") !=
        std::string::npos);
    assert(item.body.find("resourceRevision") ==
        std::string::npos);
    for (const char* forbidden :
         {"passwordHash", "sessionSecretHash",
          "csrfSecretHash", "tokenId",
          "browserCredentialId"})
    {
        assert(item.body.find(forbidden) ==
            std::string::npos);
    }

    const std::string activeEtag = item.headers.at("ETag");

    ApiResponse conditional;
    assert(runtime.tryHandleGet(
        path,
        "actor-admin",
        "mu8c-conditional",
        "",
        conditional,
        activeEtag));
    assert(conditional.statusCode == 304);
    assert(conditional.headers.at("ETag") == activeEtag);

    ApiResponse missingIfMatch;
    assert(runtime.tryHandlePost(
        path,
        "mu8c-missing",
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
        "mu8c-malformed",
        "",
        malformedIfMatch,
        "{}",
        "actor-admin",
        "W/\"bad\"",
        "",
        "application/json"));
    assert(malformedIfMatch.statusCode == 400);

    const std::string wrongTypeEtag =
        vdrsuite::http::publicStrongEntityTag(
            "session-lifecycle:" + std::string(64U, 'd'));
    assert(!wrongTypeEtag.empty());

    ApiResponse wrongRevisionType;
    assert(runtime.tryHandlePost(
        path,
        "mu8c-wrong-type",
        "",
        wrongRevisionType,
        "{}",
        "actor-admin",
        wrongTypeEtag,
        "",
        "application/json"));
    assert(wrongRevisionType.statusCode == 400);

    ApiResponse invalidBody;
    assert(runtime.tryHandlePost(
        path,
        "mu8c-body",
        "",
        invalidBody,
        "{\"x\":1}",
        "actor-admin",
        activeEtag,
        "",
        "application/json"));
    assert(invalidBody.statusCode == 422);

    const std::string staleEtag =
        vdrsuite::http::publicStrongEntityTag(StaleRevision);
    assert(!staleEtag.empty());

    ApiResponse stale;
    assert(runtime.tryHandlePost(
        path,
        "mu8c-stale",
        "",
        stale,
        "{}",
        "actor-admin",
        staleEtag,
        "",
        "application/json"));
    assert(stale.statusCode == 412);

    mutationMode = 1;
    ApiResponse finalAdmin;
    assert(runtime.tryHandlePost(
        path,
        "mu8c-final-admin",
        "",
        finalAdmin,
        "{}",
        "actor-admin",
        activeEtag,
        "",
        "application/json"));
    assert(finalAdmin.statusCode == 409);
    mutationMode = 0;

    ApiResponse revoked;
    assert(runtime.tryHandlePost(
        path,
        "mu8c-revoke",
        "",
        revoked,
        "{}",
        "actor-admin",
        activeEtag,
        "",
        "application/json"));
    assert(revoked.statusCode == 200);
    assert(revoked.headers.count("ETag") == 1U);
    assert(revoked.headers.at("ETag") != activeEtag);
    assert(revoked.body.find("\"active\":false") !=
        std::string::npos);
    assert(revoked.body.find("\"revoked\":true") !=
        std::string::npos);

    ApiResponse queryRejected;
    assert(runtime.tryHandleGet(
        path + "?x=1",
        "actor-admin",
        "mu8c-query",
        "",
        queryRejected));
    assert(queryRejected.statusCode == 400);

    ApiResponse putRejected;
    assert(runtime.tryHandleUnsupportedMethod(
        "PUT",
        path,
        "mu8c-put",
        "",
        putRejected));
    assert(putRejected.statusCode == 405);
    assert(putRejected.headers.at("Allow") == "GET, POST");

    ApiResponse collectionPost;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/credentials",
        "mu8c-collection",
        "",
        collectionPost,
        "{}",
        "actor-admin",
        activeEtag,
        "",
        "application/json"));
    assert(collectionPost.statusCode == 405);
    assert(collectionPost.headers.at("Allow") == "GET");

    runtime.resetAccountCredentialMutation();
    runtime.resetAccountCredentialItemLookup();
    return 0;
}

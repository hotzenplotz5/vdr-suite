#include "PublicApiRuntime.h"
#include "PublicResourcePreconditions.h"

#include <cassert>
#include <string>

namespace
{
const std::string RevisionA =
    "grant-set:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
const std::string RevisionB =
    "grant-set:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

PublicAccountGrantSetResource grantSet(
    const std::string& revision = RevisionA)
{
    PublicAccountGrantSetResource value;
    value.accountId = "account-a";
    value.actorId = "actor-a";
    value.resourceRevision = revision;
    value.supportedPermissions = {
        "channels.view",
        "role.admin",
        "role.read-only",
        "timers.view"};
    value.supportedScopeKinds = {
        "global",
        "backend"};

    PublicAccountGrantItem channel;
    channel.permission = "channels.view";
    channel.backendId = "default";
    value.grants.push_back(channel);

    PublicAccountGrantItem timers;
    timers.permission = "timers.view";
    timers.backendId = "default";
    value.grants.push_back(timers);
    return value;
}
}

int main()
{
    PublicApiRuntime& runtime =
        PublicApiRuntime::instance();

    runtime.resetAccountGrantLookup();
    runtime.resetAccountGrantMutation();

    PublicAccountGrantStatus lookupStatus =
        PublicAccountGrantStatus::ok;
    runtime.registerAccountGrantLookup(
        [&lookupStatus](const std::string& accountId)
        {
            assert(accountId == "account-a");
            PublicAccountGrantLookupResult result;
            result.status = lookupStatus;
            if (lookupStatus == PublicAccountGrantStatus::ok)
            {
                result.grantSet = grantSet();
            }
            return result;
        });

    PublicAccountGrantStatus mutationStatus =
        PublicAccountGrantStatus::ok;
    runtime.registerAccountGrantMutation(
        [&mutationStatus](
            const PublicAccountGrantMutationRequest& request)
        {
            assert(request.actorRef == "actor-admin");
            assert(request.accountId == "account-a");
            assert(request.permission == "channels.view");
            assert(request.backendId == "default");
            assert(request.active);
            assert(request.expectedResourceRevision ==
                RevisionA);
            assert(request.requestId.find("mu7-") == 0U);

            PublicAccountGrantMutationResult result;
            result.status = mutationStatus;
            if (mutationStatus ==
                    PublicAccountGrantStatus::ok ||
                mutationStatus ==
                    PublicAccountGrantStatus::revisionConflict ||
                mutationStatus ==
                    PublicAccountGrantStatus::finalAdministrator)
            {
                result.grantSet =
                    grantSet(
                        mutationStatus ==
                                PublicAccountGrantStatus::ok
                            ? RevisionB
                            : RevisionA);
            }
            return result;
        });

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities",
        "actor-admin",
        "mu7-capabilities",
        "",
        capabilities));
    assert(capabilities.statusCode == 200);
    assert(capabilities.body.find(
        "\"id\":\"public-api.accounts-grants-administration\"") !=
        std::string::npos);
    assert(capabilities.body.find(
        "\"availability\":\"available\"") !=
        std::string::npos);

    ApiResponse read;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a/grants",
        "actor-admin",
        "mu7-grants-read",
        "",
        read));
    assert(read.statusCode == 200);
    assert(read.body.find(
        "\"supportedPermissions\":[\"channels.view\",\"role.admin\",\"role.read-only\",\"timers.view\"]") !=
        std::string::npos);
    assert(read.body.find(
        "\"supportedScopeKinds\":[\"global\",\"backend\"]") !=
        std::string::npos);
    assert(read.headers.count("ETag") == 1U);
    assert(read.body.find(
        "\"accountId\":\"account-a\"") !=
        std::string::npos);
    assert(read.body.find(
        "\"actorId\":\"actor-a\"") !=
        std::string::npos);
    assert(read.body.find(
        "\"permission\":\"channels.view\"") !=
        std::string::npos);
    assert(read.body.find(
        "\"backendId\":\"default\"") !=
        std::string::npos);
    assert(read.body.find("password") ==
        std::string::npos);
    assert(read.body.find("credential") ==
        std::string::npos);
    assert(read.body.find("session") ==
        std::string::npos);

    const std::string etagA =
        read.headers.at("ETag");

    ApiResponse notModified;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a/grants",
        "actor-admin",
        "mu7-grants-304",
        "",
        notModified,
        etagA));
    assert(notModified.statusCode == 304);
    assert(notModified.headers.at("ETag") == etagA);

    ApiResponse queryRejected;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a/grants?x=1",
        "actor-admin",
        "mu7-grants-query",
        "",
        queryRejected));
    assert(queryRejected.statusCode == 400);

    ApiResponse mutated;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "mu7-grants-post",
        "",
        mutated,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true}",
        "actor-admin",
        etagA,
        "",
        "application/json"));
    assert(mutated.statusCode == 200);
    assert(mutated.headers.count("ETag") == 1U);
    assert(mutated.headers.at("ETag") != etagA);

    ApiResponse missingIfMatch;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "mu7-missing-if-match",
        "",
        missingIfMatch,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true}",
        "actor-admin",
        "",
        "",
        "application/json"));
    assert(missingIfMatch.statusCode == 428);

    ApiResponse weakIfMatch;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "mu7-weak-if-match",
        "",
        weakIfMatch,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true}",
        "actor-admin",
        "W/\"weak\"",
        "",
        "application/json"));
    assert(weakIfMatch.statusCode == 400);

    ApiResponse wrongRevisionType;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "mu7-wrong-revision",
        "",
        wrongRevisionType,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true}",
        "actor-admin",
        vdrsuite::http::publicStrongEntityTag("account:7"),
        "",
        "application/json"));
    assert(wrongRevisionType.statusCode == 400);

    ApiResponse ambiguous;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "mu7-ambiguous",
        "",
        ambiguous,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true,\"extra\":1}",
        "actor-admin",
        etagA,
        "",
        "application/json"));
    assert(ambiguous.statusCode == 422);

    ApiResponse mediaType;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "mu7-media-type",
        "",
        mediaType,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true}",
        "actor-admin",
        etagA,
        "",
        "text/plain"));
    assert(mediaType.statusCode == 415);

    mutationStatus =
        PublicAccountGrantStatus::revisionConflict;
    ApiResponse stale;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "mu7-stale",
        "",
        stale,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true}",
        "actor-admin",
        etagA,
        "",
        "application/json"));
    assert(stale.statusCode == 412);
    assert(stale.body.find(
        "\"code\":\"revision_conflict\"") !=
        std::string::npos);

    mutationStatus =
        PublicAccountGrantStatus::finalAdministrator;
    ApiResponse finalAdmin;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "mu7-final-admin",
        "",
        finalAdmin,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true}",
        "actor-admin",
        etagA,
        "",
        "application/json"));
    assert(finalAdmin.statusCode == 409);

    mutationStatus =
        PublicAccountGrantStatus::invalid;
    ApiResponse invalidGrant;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "mu7-invalid-grant",
        "",
        invalidGrant,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true}",
        "actor-admin",
        etagA,
        "",
        "application/json"));
    assert(invalidGrant.statusCode == 422);

    lookupStatus =
        PublicAccountGrantStatus::notFound;
    ApiResponse missing;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a/grants",
        "actor-admin",
        "mu7-missing-account",
        "",
        missing));
    assert(missing.statusCode == 404);

    ApiResponse unsupported;
    assert(runtime.tryHandleUnsupportedMethod(
        "PUT",
        "/api/v1/accounts/account-a/grants",
        "mu7-put",
        "",
        unsupported));
    assert(unsupported.statusCode == 405);
    assert(unsupported.headers.at("Allow") ==
        "GET, POST");

    runtime.resetAccountGrantMutation();
    runtime.resetAccountGrantLookup();
    return 0;
}

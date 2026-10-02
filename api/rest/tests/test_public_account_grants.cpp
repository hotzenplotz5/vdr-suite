#include "PublicApiRuntime.h"
#include "PublicResourcePreconditions.h"

#include <cassert>
#include <string>

namespace
{
PublicAccountGrantResource resource(bool granted)
{
    PublicAccountGrantResource value;
    value.accountId = "account-a";
    value.actorId = "actor-a";
    if (granted)
    {
        value.grants.push_back(
            {"channels.view", "default"});
    }
    value.resourceRevision = granted
        ? "account-grants-v1|9:account-a|7:actor-a|13:channels.view|7:default|"
        : "account-grants-v1|9:account-a|7:actor-a|";
    return value;
}
}

int main()
{
    PublicApiRuntime& runtime =
        PublicApiRuntime::instance();
    runtime.resetAccountGrantLookup();
    runtime.resetAccountGrantMutation();

    bool granted = false;
    runtime.registerAccountGrantLookup(
        [&](const std::string& accountId)
        {
            PublicAccountGrantLookupResult result;
            if (accountId != "account-a")
            {
                result.status =
                    PublicAccountGrantStatus::notFound;
                return result;
            }
            result.status = PublicAccountGrantStatus::ok;
            result.resource = resource(granted);
            return result;
        });

    runtime.registerAccountGrantMutation(
        [&](const PublicAccountGrantMutationRequest& request)
        {
            PublicAccountGrantMutationResult result;
            assert(request.actorRef == "actor-admin");
            assert(request.accountId == "account-a");
            assert(request.permission == "channels.view");
            assert(request.backendId == "default");
            if (request.expectedResourceRevision !=
                resource(granted).resourceRevision)
            {
                result.status =
                    PublicAccountGrantStatus::revisionConflict;
                return result;
            }
            granted = request.active;
            result.status = PublicAccountGrantStatus::ok;
            result.resource = resource(granted);
            return result;
        });

    ApiResponse response;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a/grants",
        "",
        "request-unauthenticated",
        "correlation-mu7",
        response));
    assert(response.statusCode == 401);

    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a/grants",
        "actor-admin",
        "request-read",
        "correlation-mu7",
        response));
    assert(response.statusCode == 200);
    assert(response.body.find("\"grants\":[]") !=
        std::string::npos);
    assert(response.body.find("password") ==
        std::string::npos);
    const std::string emptyTag =
        response.headers.at("ETag");
    assert(emptyTag ==
        vdrsuite::http::publicStrongEntityTag(
            resource(false).resourceRevision));

    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a/grants",
        "actor-admin",
        "request-not-modified",
        "correlation-mu7",
        response,
        emptyTag));
    assert(response.statusCode == 304);

    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "request-missing-precondition",
        "correlation-mu7",
        response,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true}",
        "actor-admin",
        "",
        "",
        "application/json"));
    assert(response.statusCode == 428);

    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "request-wrong-resource-tag",
        "correlation-mu7",
        response,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true}",
        "actor-admin",
        vdrsuite::http::publicStrongEntityTag("account:7"),
        "",
        "application/json"));
    assert(response.statusCode == 400);

    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "request-invalid-body",
        "correlation-mu7",
        response,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\"}",
        "actor-admin",
        emptyTag,
        "",
        "application/json"));
    assert(response.statusCode == 422);

    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "request-grant",
        "correlation-mu7",
        response,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":true}",
        "actor-admin",
        emptyTag,
        "",
        "application/json"));
    assert(response.statusCode == 200);
    assert(response.body.find(
        "\"permission\":\"channels.view\"") !=
        std::string::npos);
    assert(response.body.find(
        "\"backendId\":\"default\"") !=
        std::string::npos);
    const std::string grantedTag =
        response.headers.at("ETag");
    assert(grantedTag != emptyTag);

    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "request-stale-change",
        "correlation-mu7",
        response,
        "{\"permission\":\"channels.view\",\"backendId\":\"default\",\"active\":false}",
        "actor-admin",
        emptyTag,
        "",
        "application/json"));
    assert(response.statusCode == 412);

    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/grants",
        "request-revoke",
        "correlation-mu7",
        response,
        "{\"active\":false,\"backendId\":\"default\",\"permission\":\"channels.view\"}",
        "actor-admin",
        grantedTag,
        "",
        "application/json"));
    assert(response.statusCode == 200);
    assert(response.headers.at("ETag") == emptyTag);

    assert(runtime.tryHandleUnsupportedMethod(
        "DELETE",
        "/api/v1/accounts/account-a/grants",
        "request-method",
        "correlation-mu7",
        response));
    assert(response.statusCode == 405);

    runtime.resetAccountGrantMutation();
    runtime.resetAccountGrantLookup();
    return 0;
}

#include "PublicApiRuntime.h"

#include <cassert>
#include <string>

int main()
{
    PublicApiRuntime& runtime = PublicApiRuntime::instance();
    runtime.resetAccountCredentialLookup();
    runtime.resetAccountSessionLookup();

    runtime.registerAccountCredentialLookup(
        [](const std::string& accountId)
        {
            PublicAccountCredentialCollectionResult result;
            result.status = PublicAccountSecurityMetadataStatus::ok;
            result.collection.accountId = accountId;
            result.collection.actorId = "actor-a";
            PublicAccountCredentialItem item;
            item.credentialId = "credential-human";
            item.credentialType = "human-password";
            item.active = true;
            item.createdAt = "2026-10-03 18:00:00";
            result.collection.credentials.push_back(item);
            return result;
        });

    runtime.registerAccountSessionLookup(
        [](const std::string& accountId)
        {
            PublicAccountSessionCollectionResult result;
            result.status = PublicAccountSecurityMetadataStatus::ok;
            result.collection.accountId = accountId;
            result.collection.actorId = "actor-a";
            PublicAccountSessionItem item;
            item.sessionId = "session-a";
            item.deviceId = "device-a";
            item.issuedFromCredentialId = "credential-human";
            item.active = true;
            item.expiresAt = "2099-01-01 00:00:00";
            item.createdAt = "2026-10-03 18:00:00";
            result.collection.sessions.push_back(item);
            return result;
        });

    ApiResponse capabilities;
    assert(runtime.tryHandleGet(
        "/api/v1/capabilities", "actor-admin",
        "mu8a-capabilities", "", capabilities));
    assert(capabilities.statusCode == 200);
    assert(capabilities.body.find(
        "\"id\":\"public-api.accounts-credential-session-metadata\"") !=
        std::string::npos);

    ApiResponse credentials;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a/credentials",
        "actor-admin", "mu8a-credentials", "", credentials));
    assert(credentials.statusCode == 200);
    assert(credentials.headers.count("ETag") == 0U);
    assert(credentials.headers.at("Cache-Control") == "no-store");
    assert(credentials.body.find(
        "\"credentialId\":\"credential-human\"") != std::string::npos);
    for (const char* forbidden :
         {"passwordHash", "sessionSecretHash", "csrfSecretHash", "tokenId"})
        assert(credentials.body.find(forbidden) == std::string::npos);

    ApiResponse sessions;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a/sessions",
        "actor-admin", "mu8a-sessions", "", sessions));
    assert(sessions.statusCode == 200);
    assert(sessions.headers.count("ETag") == 0U);
    assert(sessions.body.find(
        "\"issuedFromCredentialId\":\"credential-human\"") !=
        std::string::npos);
    for (const char* forbidden :
         {"sessionSecretHash", "csrfSecretHash", "tokenId"})
        assert(sessions.body.find(forbidden) == std::string::npos);

    ApiResponse queryRejected;
    assert(runtime.tryHandleGet(
        "/api/v1/accounts/account-a/sessions?x=1",
        "actor-admin", "mu8a-query", "", queryRejected));
    assert(queryRejected.statusCode == 400);

    ApiResponse postRejected;
    assert(runtime.tryHandlePost(
        "/api/v1/accounts/account-a/credentials",
        "mu8a-post", "", postRejected, "{}",
        "actor-admin", "", "", "application/json"));
    assert(postRejected.statusCode == 405);
    assert(postRejected.headers.at("Allow") == "GET");

    ApiResponse putRejected;
    assert(runtime.tryHandleUnsupportedMethod(
        "PUT", "/api/v1/accounts/account-a/sessions",
        "mu8a-put", "", putRejected));
    assert(putRejected.statusCode == 405);
    assert(putRejected.headers.at("Allow") == "GET");

    runtime.resetAccountSessionLookup();
    runtime.resetAccountCredentialLookup();
    return 0;
}

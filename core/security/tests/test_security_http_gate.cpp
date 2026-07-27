#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "SecurityHttpGate.h"

#include <cassert>
#include <string>

namespace
{
const std::string kCredential =
    "Basic YWRtaW46dmRyLXN1aXRl";

HttpServerRequest getRequest(bool authenticated)
{
    HttpServerRequest request;
    request.method = "GET";
    request.path = "/api/backends";

    if (authenticated)
    {
        request.headers["Authorization"] = kCredential;
    }

    return request;
}

HttpServerRequest remoteRequest(
    const std::string& backendId,
    bool authenticated)
{
    HttpServerRequest request;
    request.method = "POST";
    request.path = "/api/vdr/remote/actions";
    request.body =
        "{\"backendId\":\"" + backendId +
        "\",\"operationId\":\"op-1\",\"action\":\"ok\"}";

    if (authenticated)
    {
        request.headers["Authorization"] = kCredential;
    }

    request.headers["X-Request-ID"] = "request-1";
    request.headers["X-Correlation-ID"] = "correlation-1";
    return request;
}

SecurityConfiguration compatibility()
{
    SecurityConfiguration configuration;
    configuration.mode =
        SecurityMode::LegacyBasicCompatibility;
    configuration.expectedAuthorizationHeader = kCredential;
    configuration.grants = {PermissionGrant{"*", "*"}};
    return configuration;
}

SecurityConfiguration enforced(
    const std::vector<PermissionGrant>& grants)
{
    SecurityConfiguration configuration;
    configuration.mode = SecurityMode::Enforced;
    configuration.expectedAuthorizationHeader = kCredential;
    configuration.grants = grants;
    return configuration;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    AccountabilityEventRepository repository(database);
    assert(repository.ensureSchema());

    SecurityHttpGate compatibilityGate(
        compatibility(),
        repository);

    const SecurityGateDecision anonymousCompatibility =
        compatibilityGate.evaluate(getRequest(false));
    assert(!anonymousCompatibility.allowed);
    assert(anonymousCompatibility.rejection.statusCode == 401);
    assert(anonymousCompatibility.rejection.body.find(
        "authentication_required") != std::string::npos);
    assert(anonymousCompatibility.rejection.body.find(
        kCredential) == std::string::npos);

    const SecurityGateDecision authenticatedCompatibility =
        compatibilityGate.evaluate(getRequest(true));
    assert(authenticatedCompatibility.allowed);
    assert(authenticatedCompatibility.context.actor.actorId ==
        "legacy-local-web");

    const SecurityGateDecision compatibilityRemote =
        compatibilityGate.evaluate(remoteRequest("default", true));
    assert(compatibilityRemote.allowed);
    assert(compatibilityRemote.protectedMutation);
    assert(compatibilityRemote.context.requestId == "request-1");

    SecurityHttpGate enforcedGate(
        enforced({PermissionGrant{"remote.control", "default"}}),
        repository);

    assert(enforcedGate.evaluate(getRequest(false)).allowed);

    const SecurityGateDecision anonymousRemote =
        enforcedGate.evaluate(remoteRequest("default", false));
    assert(!anonymousRemote.allowed);
    assert(anonymousRemote.rejection.statusCode == 401);
    assert(anonymousRemote.rejection.body.find(
        "authentication_required") != std::string::npos);

    const SecurityGateDecision permittedRemote =
        enforcedGate.evaluate(remoteRequest("default", true));
    assert(permittedRemote.allowed);

    const SecurityGateDecision wrongScope =
        enforcedGate.evaluate(remoteRequest("house-b", true));
    assert(!wrongScope.allowed);
    assert(wrongScope.rejection.statusCode == 403);
    assert(wrongScope.rejection.body.find(
        "backend_scope_denied") != std::string::npos);

    SecurityHttpGate noPermissionGate(
        enforced({PermissionGrant{"recordings.view", "*"}}),
        repository);
    const SecurityGateDecision missingPermission =
        noPermissionGate.evaluate(remoteRequest("default", true));
    assert(!missingPermission.allowed);
    assert(missingPermission.rejection.statusCode == 403);
    assert(missingPermission.rejection.body.find(
        "permission_denied") != std::string::npos);

    HttpServerRequest invalidCredential =
        remoteRequest("default", false);
    invalidCredential.headers["Authorization"] =
        "Basic definitely-not-valid";
    const SecurityGateDecision invalid =
        enforcedGate.evaluate(invalidCredential);
    assert(!invalid.allowed);
    assert(invalid.rejection.statusCode == 401);
    assert(invalid.rejection.body.find(
        "invalid_credentials") != std::string::npos);
    assert(invalid.rejection.body.find(
        "definitely-not-valid") == std::string::npos);

    const SecurityGateDecision missingBackend =
        enforcedGate.evaluate(remoteRequest("", true));
    assert(!missingBackend.allowed);
    assert(missingBackend.rejection.statusCode == 400);
    assert(missingBackend.rejection.body.find(
        "invalid_backend_scope") != std::string::npos);

    HttpServerRequest unmigrated;
    unmigrated.method = "POST";
    unmigrated.path = "/api/vdr/timers/actions/create";
    unmigrated.headers["Authorization"] = kCredential;
    const SecurityGateDecision failClosed =
        enforcedGate.evaluate(unmigrated);
    assert(!failClosed.allowed);
    assert(failClosed.rejection.statusCode == 503);
    assert(failClosed.rejection.body.find(
        "security_policy_not_migrated") != std::string::npos);

    HttpServerResponse response;
    enforcedGate.decorateResponse(
        permittedRemote.context,
        response);
    assert(response.headers.at("X-Request-ID") ==
        "request-1");
    assert(response.headers.at("X-Correlation-ID") ==
        "correlation-1");

    const auto events = repository.listAll();
    bool sawAllowed = false;
    bool sawDenied = false;
    bool sawUnmigratedDenial = false;

    for (const AccountabilityEvent& event : events)
    {
        sawAllowed = sawAllowed ||
            event.eventType == "authorization.allowed";
        sawDenied = sawDenied ||
            event.eventType == "authorization.denied";
        sawUnmigratedDenial = sawUnmigratedDenial ||
            event.reasonCode == "security_policy_not_migrated";
        assert(event.permission.find("Basic ") ==
            std::string::npos);
        assert(event.reasonCode.find("Basic ") ==
            std::string::npos);
    }

    assert(sawAllowed);
    assert(sawDenied);
    assert(sawUnmigratedDenial);

    Database closedDatabase;
    AccountabilityEventRepository unavailableRepository(
        closedDatabase);
    SecurityHttpGate unavailableGate(
        enforced({PermissionGrant{"remote.control", "default"}}),
        unavailableRepository);
    const SecurityGateDecision auditFailure =
        unavailableGate.evaluate(remoteRequest("default", true));
    assert(!auditFailure.allowed);
    assert(auditFailure.rejection.statusCode == 503);
    assert(auditFailure.rejection.body.find(
        "accountability_unavailable") != std::string::npos);

    return 0;
}

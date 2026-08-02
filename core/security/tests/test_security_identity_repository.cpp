#include "Database.h"
#include "PersistentIdentityResolver.h"
#include "SecurityIdentityRepository.h"

#include <cassert>
#include <cstdio>
#include <string>

namespace
{

RequestSecurityContext authenticatedContext()
{
    RequestSecurityContext context;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor.actorId = "legacy-local-web";
    context.actor.type = ActorType::User;
    context.actor.displayName = "untrusted transient value";
    context.device = DeviceIdentity{"legacy-browser", true};
    context.session = SessionIdentity{
        "legacy-basic-session",
        true,
        false,
        false};
    context.credential = CredentialIdentity{
        "legacy-basic-credential",
        true,
        false,
        false};
    return context;
}

}

int main()
{
    const std::string path =
        "/tmp/vdr-suite-security-identity-repository-test.db";
    std::remove(path.c_str());

    Database database;
    assert(database.open(path));

    SecurityIdentityRepository repository(database);
    assert(repository.ensureSchema());
    assert(repository.ensureCompatibilityIdentity(
        "legacy-local-web",
        ActorType::User,
        "Legacy local web client",
        "legacy-browser",
        "legacy-basic-session",
        "legacy-basic-credential"));

    const auto actor = repository.findActor("legacy-local-web");
    assert(actor.has_value());
    assert(actor->type == ActorType::User);
    assert(actor->displayName == "Legacy local web client");
    assert(actor->active);
    assert(!actor->revoked);

    const auto device = repository.findDevice("legacy-browser");
    assert(device.has_value());
    assert(device->actorId == "legacy-local-web");
    assert(device->active);
    assert(!device->revoked);

    const auto session = repository.findSession("legacy-basic-session");
    assert(session.has_value());
    assert(session->actorId == "legacy-local-web");
    assert(session->deviceId == "legacy-browser");
    assert(session->active);
    assert(!session->expired);
    assert(!session->revoked);

    const auto credential = repository.findCredential(
        "legacy-basic-credential");
    assert(credential.has_value());
    assert(credential->actorId == "legacy-local-web");
    assert(credential->credentialType == "legacy-basic");
    assert(credential->active);
    assert(!credential->expired);
    assert(!credential->revoked);

    PersistentIdentityResolver resolver(repository);
    RequestSecurityContext context = resolver.resolve(
        authenticatedContext());
    assert(context.authenticated());
    assert(context.actor.displayName == "Legacy local web client");

    assert(repository.setSessionExpiry(
        "legacy-basic-session",
        "2000-01-01 00:00:00"));
    context = resolver.resolve(authenticatedContext());
    assert(context.authenticationState == AuthenticationState::Expired);
    assert(context.session.has_value());
    assert(context.session->expired);

    assert(repository.setSessionExpiry(
        "legacy-basic-session",
        ""));
    assert(repository.revokeCredential(
        "legacy-basic-credential"));
    context = resolver.resolve(authenticatedContext());
    assert(context.authenticationState == AuthenticationState::Revoked);
    assert(context.credential.has_value());
    assert(context.credential->revoked);
    assert(!context.credential->active);

    assert(!repository.revokeCredential("missing-credential"));
    assert(!repository.ensureCompatibilityIdentity(
        "",
        ActorType::User,
        "invalid",
        "device",
        "session",
        "credential"));

    std::remove(path.c_str());
    return 0;
}

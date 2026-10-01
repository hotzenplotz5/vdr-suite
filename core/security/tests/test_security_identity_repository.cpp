#include "Database.h"
#include "PersistentIdentityResolver.h"
#include "SecurityIdentityRepository.h"
#include "SecurityIdentityProvisioningRepository.h"

#include <cassert>
#include <cstdio>
#include <string>

namespace
{

RequestSecurityContext authenticatedContext()
{
    RequestSecurityContext context;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor.actorId = "test-user";
    context.actor.type = ActorType::User;
    context.actor.displayName = "untrusted transient value";
    context.device = DeviceIdentity{"test-device", true};
    context.session = SessionIdentity{
        "test-session",
        true,
        false,
        false};
    context.credential = CredentialIdentity{
        "test-credential",
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
    SecurityIdentityProvisioningRepository provisioning(database);
    assert(provisioning.ensureIdentity(
        "test-user",
        ActorType::User,
        "Test user",
        "test-device",
        "Test device",
        "test-session",
        "test-credential",
        "test-credential"));

    const auto actor = repository.findActor("test-user");
    assert(actor.has_value());
    assert(actor->type == ActorType::User);
    assert(actor->displayName == "Test user");
    assert(actor->active);
    assert(!actor->revoked);

    const auto device = repository.findDevice("test-device");
    assert(device.has_value());
    assert(device->actorId == "test-user");
    assert(device->active);
    assert(!device->revoked);

    const auto session = repository.findSession("test-session");
    assert(session.has_value());
    assert(session->actorId == "test-user");
    assert(session->deviceId == "test-device");
    assert(session->active);
    assert(!session->expired);
    assert(!session->revoked);

    const auto credential = repository.findCredential(
        "test-credential");
    assert(credential.has_value());
    assert(credential->actorId == "test-user");
    assert(credential->credentialType == "test-credential");
    assert(credential->active);
    assert(!credential->expired);
    assert(!credential->revoked);

    PersistentIdentityResolver resolver(repository);
    RequestSecurityContext context = resolver.resolve(
        authenticatedContext());
    assert(context.authenticated());
    assert(context.actor.displayName == "Test user");

    assert(repository.setSessionExpiry(
        "test-session",
        "2000-01-01 00:00:00"));
    context = resolver.resolve(authenticatedContext());
    assert(context.authenticationState == AuthenticationState::Expired);
    assert(context.session.has_value());
    assert(context.session->expired);

    assert(repository.setSessionExpiry(
        "test-session",
        ""));
    assert(repository.revokeCredential(
        "test-credential"));
    context = resolver.resolve(authenticatedContext());
    assert(context.authenticationState == AuthenticationState::Revoked);
    assert(context.credential.has_value());
    assert(context.credential->revoked);
    assert(!context.credential->active);

    assert(!repository.revokeCredential("missing-credential"));
    assert(!provisioning.ensureIdentity(
        "",
        ActorType::User,
        "invalid",
        "device",
        "device",
        "session",
        "credential",
        "test-credential"));

    std::remove(path.c_str());
    return 0;
}

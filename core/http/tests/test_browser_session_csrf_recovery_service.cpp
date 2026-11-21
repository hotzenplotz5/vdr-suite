#include "BrowserSessionAuthenticator.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionCsrfRecoveryService.h"
#include "BrowserSessionIssuanceService.h"
#include "Database.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::vector<unsigned char> bytes(
    unsigned char start,
    std::size_t size)
{
    std::vector<unsigned char> result(size);
    for (std::size_t index = 0; index < size; ++index)
    {
        result[index] = static_cast<unsigned char>(start + index);
    }
    return result;
}

BrowserSessionIssuanceService::EntropySource sequenceEntropy(
    std::vector<std::vector<unsigned char>> chunks)
{
    return [chunks = std::move(chunks), index = std::size_t{0}](
               unsigned char* output,
               std::size_t size) mutable
    {
        if (output == nullptr ||
            index >= chunks.size() ||
            chunks[index].size() != size)
        {
            return false;
        }

        std::copy(
            chunks[index].begin(),
            chunks[index].end(),
            output);
        ++index;
        return true;
    };
}

BrowserSessionIssuanceService::Clock fixedClock()
{
    return []
    {
        return std::chrono::system_clock::time_point(
            std::chrono::seconds(4070908800));
    };
}

std::string jsonStringValue(
    const std::string& body,
    const std::string& field)
{
    const std::string prefix = "\"" + field + "\":\"";
    const std::size_t begin = body.find(prefix);
    if (begin == std::string::npos)
    {
        return {};
    }
    const std::size_t valueBegin = begin + prefix.size();
    const std::size_t end = body.find('"', valueBegin);
    return end == std::string::npos
        ? std::string()
        : body.substr(valueBegin, end - valueBegin);
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    SecurityIdentityRepository identityRepository(database);
    assert(identityRepository.ensureSchema());

    SecurityIdentityProvisioningRepository provisioningRepository(database);
    assert(provisioningRepository.ensureIdentity(
        "csrf-recovery-actor",
        ActorType::User,
        "CSRF recovery actor",
        "csrf-recovery-device",
        "CSRF recovery browser",
        "csrf-recovery-source-session",
        "csrf-recovery-source-credential",
        "managed-basic"));

    BrowserSessionCredentialRepository credentialRepository(database);
    assert(credentialRepository.ensureSchema());

    SecurityPermissionGrantRepository grantRepository(database);
    assert(grantRepository.ensureSchema());

    BrowserSessionIssuanceService issuanceService(
        database,
        identityRepository,
        credentialRepository,
        sequenceEntropy({
            bytes(0x10, 16),
            bytes(0x20, 16),
            bytes(0x30, 16),
            bytes(0x40, 32),
            bytes(0x60, 32),
            bytes(0x80, 16),
            bytes(0xa0, 16),
        }),
        fixedClock());

    BrowserSessionIssuanceRequest request;
    request.actorId = "csrf-recovery-actor";
    request.deviceId = "csrf-recovery-device";
    request.issuedFromCredentialId =
        "csrf-recovery-source-credential";
    request.lifetimeSeconds = 900;

    auto issued = issuanceService.issue(request);
    assert(issued.has_value());
    const std::string cookie = issued->sessionCookieValue;
    const std::string originalCsrf = issued->csrfToken;

    RequestSecurityContext context;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor.actorId = request.actorId;
    context.actor.type = ActorType::User;
    context.actor.active = true;
    context.device = DeviceIdentity{request.deviceId, true};
    context.session = SessionIdentity{
        issued->sessionId,
        true,
        false,
        false};
    context.credential = CredentialIdentity{
        issued->credentialId,
        true,
        false,
        false};
    context.requestId = "req-csrf-recovery";

    BrowserSessionCsrfRecoveryService recoveryService(
        credentialRepository);
    const HttpServerResponse recovered =
        recoveryService.recover(context);
    assert(recovered.statusCode == 200);
    assert(recovered.headers.at("Cache-Control") == "no-store");
    assert(recovered.headers.at("Vary") == "Cookie");

    const std::string recoveredCsrf =
        jsonStringValue(recovered.body, "csrfToken");
    assert(recoveredCsrf.size() >= 32);
    assert(recoveredCsrf != originalCsrf);
    assert(recovered.body.find(originalCsrf) == std::string::npos);

    BrowserSessionAuthenticator authenticator(
        credentialRepository,
        grantRepository);
    const std::map<std::string, std::string> recoveredHeaders = {
        {"Cookie", "vdr_suite_session=" + cookie},
        {"X-CSRF-Token", recoveredCsrf},
    };
    assert(authenticator.verifyCsrf(recoveredHeaders));

    const std::map<std::string, std::string> originalHeaders = {
        {"Cookie", "vdr_suite_session=" + cookie},
        {"X-CSRF-Token", originalCsrf},
    };
    assert(authenticator.verifyCsrf(originalHeaders));

    std::string invalidCsrf = recoveredCsrf;
    invalidCsrf[0] = invalidCsrf[0] == 'A' ? 'B' : 'A';
    const std::map<std::string, std::string> invalidHeaders = {
        {"Cookie", "vdr_suite_session=" + cookie},
        {"X-CSRF-Token", invalidCsrf},
    };
    assert(!authenticator.verifyCsrf(invalidHeaders));

    RequestSecurityContext anonymous;
    anonymous.requestId = "req-csrf-anonymous";
    const HttpServerResponse denied =
        recoveryService.recover(anonymous);
    assert(denied.statusCode == 401);
    assert(denied.body.find("authentication_required") !=
        std::string::npos);

    issued->clearSecrets();
    return 0;
}

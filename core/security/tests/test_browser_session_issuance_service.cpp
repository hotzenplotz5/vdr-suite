#include "BrowserSessionAuthenticator.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionIssuanceService.h"
#include "Database.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <string>
#include <type_traits>
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

std::string prefixedHex(
    const std::string& prefix,
    const std::vector<unsigned char>& value)
{
    static constexpr char Hex[] = "0123456789abcdef";
    std::string result = prefix;
    for (const unsigned char byte : value)
    {
        result.push_back(Hex[(byte >> 4) & 0x0f]);
        result.push_back(Hex[byte & 0x0f]);
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

bool safeSecret(const std::string& value)
{
    if (value.size() < 32 || value.size() > 256)
    {
        return false;
    }

    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return std::isalnum(character) ||
                character == '-' ||
                character == '_';
        });
}
}

int main()
{
    static_assert(!std::is_copy_constructible<IssuedBrowserSession>::value);
    static_assert(!std::is_copy_assignable<IssuedBrowserSession>::value);
    static_assert(std::is_move_constructible<IssuedBrowserSession>::value);

    Database database;
    assert(database.open(":memory:"));

    SecurityIdentityRepository identityRepository(database);
    assert(identityRepository.ensureSchema());

    SecurityIdentityProvisioningRepository provisioningRepository(database);
    assert(provisioningRepository.ensureIdentity(
        "user-phase62-admin",
        ActorType::User,
        "Phase 62 administrator",
        "device-phase62-browser",
        "Phase 62 browser",
        "session-phase62-source",
        "credential-phase62-admin",
        "managed-basic"));

    BrowserSessionCredentialRepository credentialRepository(database);
    assert(credentialRepository.ensureSchema());

    const auto firstToken = bytes(0x10, 16);
    const auto firstSession = bytes(0x20, 16);
    const auto firstCredential = bytes(0x30, 16);
    const auto secondSession = bytes(0x40, 16);
    const auto secondCredential = bytes(0x50, 16);

    std::vector<std::vector<unsigned char>> entropyChunks = {
        firstToken,
        firstSession,
        firstCredential,
        bytes(0x60, 32),
        bytes(0x80, 32),
        bytes(0xa0, 16),
        bytes(0xb0, 16),
        firstToken,
        secondSession,
        secondCredential,
        bytes(0xc0, 32),
        bytes(0xe0, 32),
        bytes(0x21, 16),
        bytes(0x31, 16),
    };

    const auto fixedNow = std::chrono::system_clock::time_point(
        std::chrono::seconds(1785240000));
    BrowserSessionIssuanceService service(
        database,
        identityRepository,
        credentialRepository,
        sequenceEntropy(std::move(entropyChunks)),
        [fixedNow]
        {
            return fixedNow;
        });

    BrowserSessionIssuanceRequest request;
    request.actorId = "user-phase62-admin";
    request.deviceId = "device-phase62-browser";
    request.issuedFromCredentialId = "credential-phase62-admin";
    request.lifetimeSeconds = 3600;

    auto issued = service.issue(request);
    assert(issued.has_value());
    assert(issued->tokenId == prefixedHex("bst_", firstToken));
    assert(issued->sessionId == prefixedHex("bss_", firstSession));
    assert(issued->credentialId == prefixedHex("bsc_", firstCredential));
    assert(issued->sessionCookieValue.rfind(issued->tokenId + ".", 0) == 0);
    assert(safeSecret(issued->sessionCookieValue.substr(
        issued->tokenId.size() + 1)));
    assert(safeSecret(issued->csrfToken));
    assert(issued->expiresAt == "2026-07-28 09:20:00");

    const auto stored = credentialRepository.findByTokenId(issued->tokenId);
    assert(stored.has_value());
    assert(stored->actorId == request.actorId);
    assert(stored->deviceId == request.deviceId);
    assert(stored->sessionId == issued->sessionId);
    assert(stored->credentialId == issued->credentialId);
    assert(stored->issuedFromCredentialId == request.issuedFromCredentialId);
    assert(stored->sessionSecretHash.rfind("$6$rounds=10000$", 0) == 0);
    assert(stored->csrfSecretHash.rfind("$6$rounds=10000$", 0) == 0);
    assert(stored->sessionSecretHash != issued->sessionCookieValue);
    assert(stored->csrfSecretHash != issued->csrfToken);
    assert(stored->expiresAt == issued->expiresAt);
    assert(stored->active);
    assert(!stored->expired);
    assert(!stored->revoked);

    const auto session = identityRepository.findSession(issued->sessionId);
    assert(session.has_value());
    assert(session->actorId == request.actorId);
    assert(session->deviceId == request.deviceId);
    assert(session->active);
    assert(!session->expired);
    assert(!session->revoked);

    const auto credential = identityRepository.findCredential(
        issued->credentialId);
    assert(credential.has_value());
    assert(credential->actorId == request.actorId);
    assert(credential->credentialType == "browser-session");
    assert(credential->active);
    assert(!credential->expired);
    assert(!credential->revoked);

    BrowserSessionAuthenticator authenticator(
        credentialRepository,
        {PermissionGrant{"remote.control", "default"}});
    const std::map<std::string, std::string> headers = {
        {"Cookie", "vdr_suite_session=" + issued->sessionCookieValue},
        {"X-CSRF-Token", issued->csrfToken},
    };
    const RequestSecurityContext authenticated = authenticator.authenticate(
        headers,
        "request-issued-session",
        "correlation-issued-session");
    assert(authenticated.authenticated());
    assert(authenticated.actor.actorId == request.actorId);
    assert(authenticator.verifyCsrf(headers));

    const std::string rolledBackSessionId =
        prefixedHex("bss_", secondSession);
    const std::string rolledBackCredentialId =
        prefixedHex("bsc_", secondCredential);
    auto collision = service.issue(request);
    assert(!collision.has_value());
    assert(!identityRepository.findSession(rolledBackSessionId).has_value());
    assert(!identityRepository.findCredential(
        rolledBackCredentialId).has_value());
    assert(credentialRepository.findByTokenId(issued->tokenId).has_value());

    BrowserSessionIssuanceRequest tooShort = request;
    tooShort.lifetimeSeconds =
        BrowserSessionIssuanceService::MinimumLifetimeSeconds - 1;
    assert(!service.issue(tooShort).has_value());

    BrowserSessionIssuanceRequest tooLong = request;
    tooLong.lifetimeSeconds =
        BrowserSessionIssuanceService::MaximumLifetimeSeconds + 1;
    assert(!service.issue(tooLong).has_value());

    BrowserSessionIssuanceService entropyFailureService(
        database,
        identityRepository,
        credentialRepository,
        [](unsigned char*, std::size_t)
        {
            return false;
        },
        [fixedNow]
        {
            return fixedNow;
        });
    assert(!entropyFailureService.issue(request).has_value());

    assert(provisioningRepository.ensureIdentity(
        "user-phase62-revoked-source",
        ActorType::User,
        "Revoked source user",
        "device-phase62-revoked-source",
        "Revoked source browser",
        "session-phase62-revoked-source",
        "credential-phase62-revoked-source",
        "managed-basic"));
    assert(identityRepository.revokeCredential(
        "credential-phase62-revoked-source"));

    BrowserSessionIssuanceRequest revokedRequest;
    revokedRequest.actorId = "user-phase62-revoked-source";
    revokedRequest.deviceId = "device-phase62-revoked-source";
    revokedRequest.issuedFromCredentialId =
        "credential-phase62-revoked-source";
    revokedRequest.lifetimeSeconds = 3600;

    BrowserSessionIssuanceService revokedSourceService(
        database,
        identityRepository,
        credentialRepository,
        sequenceEntropy({
            bytes(0x41, 16),
            bytes(0x51, 16),
            bytes(0x61, 16),
            bytes(0x71, 32),
            bytes(0x91, 32),
            bytes(0xb1, 16),
            bytes(0xc1, 16),
        }),
        [fixedNow]
        {
            return fixedNow;
        });
    assert(!revokedSourceService.issue(revokedRequest).has_value());
    assert(!identityRepository.findSession(
        prefixedHex("bss_", bytes(0x51, 16))).has_value());
    assert(!identityRepository.findCredential(
        prefixedHex("bsc_", bytes(0x61, 16))).has_value());

    issued->clearSecrets();
    assert(issued->sessionCookieValue.empty());
    assert(issued->csrfToken.empty());

    return 0;
}

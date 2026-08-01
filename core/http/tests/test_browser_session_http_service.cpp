#include "BrowserSessionAuthenticator.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionHttpService.h"
#include "BrowserSessionIssuanceService.h"
#include "BrowserSessionLifecycleService.h"
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

std::string cookiePair(const std::string& setCookie)
{
    const std::size_t separator = setCookie.find(';');
    return separator == std::string::npos
        ? setCookie
        : setCookie.substr(0, separator);
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

    SecurityPermissionGrantRepository permissionGrantRepository(database);
    assert(permissionGrantRepository.ensureSchema());

    const auto tokenBytes = bytes(0x10, 16);
    const auto sessionBytes = bytes(0x20, 16);
    const auto credentialBytes = bytes(0x30, 16);

    BrowserSessionIssuanceService issuanceService(
        database,
        identityRepository,
        credentialRepository,
        sequenceEntropy({
            tokenBytes,
            sessionBytes,
            credentialBytes,
            bytes(0x40, 32),
            bytes(0x60, 32),
            bytes(0x80, 16),
            bytes(0xa0, 16),
        }),
        []
        {
            return std::chrono::system_clock::time_point(
                std::chrono::seconds(4070908800));
        });
    BrowserSessionLifecycleService lifecycleService(
        database,
        identityRepository,
        credentialRepository);

    BrowserSessionLifetimeConfiguration lifetimeConfiguration;
    lifetimeConfiguration.seconds = 900;
    BrowserSessionHttpService httpService(
        issuanceService,
        lifecycleService,
        lifetimeConfiguration);

    RequestSecurityContext basicContext;
    basicContext.actor = ActorIdentity{
        "user-phase62-admin",
        ActorType::User,
        "Phase 62 administrator",
        true};
    basicContext.device = DeviceIdentity{
        "device-phase62-browser",
        true};
    basicContext.session = SessionIdentity{
        "session-phase62-source",
        true,
        false,
        false};
    basicContext.credential = CredentialIdentity{
        "credential-phase62-admin",
        true,
        false,
        false};
    basicContext.authenticationState =
        AuthenticationState::Authenticated;
    basicContext.requestId = "request-browser-login";

    const HttpServerResponse login = httpService.login(basicContext);
    assert(login.statusCode == 200);
    assert(login.headers.at("Content-Type") == "application/json");
    assert(login.headers.at("Cache-Control") == "no-store");
    assert(login.headers.at("Pragma") == "no-cache");

    const std::string setCookie = login.headers.at("Set-Cookie");
    assert(setCookie.rfind("vdr_suite_session=", 0) == 0);
    assert(setCookie.find("; Path=/") != std::string::npos);
    assert(setCookie.find("; Max-Age=900") != std::string::npos);
    assert(setCookie.find("; Max-Age=28800") == std::string::npos);
    assert(setCookie.find("; HttpOnly") != std::string::npos);
    assert(setCookie.find("; Secure") != std::string::npos);
    assert(setCookie.find("; SameSite=Strict") != std::string::npos);
    assert(setCookie.find("Domain=") == std::string::npos);

    const std::string cookie = cookiePair(setCookie);
    const std::string cookieValue = cookie.substr(
        std::string("vdr_suite_session=").size());
    const std::string csrfToken =
        jsonStringValue(login.body, "csrfToken");
    assert(!cookieValue.empty());
    assert(!csrfToken.empty());
    assert(login.body.find(cookieValue) == std::string::npos);
    assert(login.body.find("vdr_suite_session") == std::string::npos);
    assert(login.body.find("2099-01-01 00:15:00") != std::string::npos);
    assert(login.body.find("request-browser-login") != std::string::npos);

    const std::string tokenId = prefixedHex("bst_", tokenBytes);
    const std::string sessionId = prefixedHex("bss_", sessionBytes);
    const std::string credentialId = prefixedHex(
        "bsc_",
        credentialBytes);
    const auto stored = credentialRepository.findByTokenId(tokenId);
    assert(stored.has_value());
    assert(stored->sessionId == sessionId);
    assert(stored->credentialId == credentialId);
    assert(stored->expiresAt == "2099-01-01 00:15:00");
    assert(stored->sessionSecretHash != cookieValue);
    assert(stored->csrfSecretHash != csrfToken);

    BrowserSessionAuthenticator authenticator(
        credentialRepository,
        permissionGrantRepository);
    const std::map<std::string, std::string> logoutHeaders = {
        {"Cookie", cookie},
        {"X-CSRF-Token", csrfToken},
    };
    RequestSecurityContext browserContext = authenticator.authenticate(
        logoutHeaders,
        "request-browser-logout",
        "");
    assert(browserContext.authenticated());
    assert(browserContext.permissionGrantResolution ==
        PermissionGrantResolutionState::Resolved);
    assert(browserContext.grants.empty());
    assert(authenticator.verifyCsrf(logoutHeaders));

    const HttpServerResponse logout = httpService.logout(browserContext);
    assert(logout.statusCode == 204);
    assert(logout.body.empty());
    assert(logout.headers.at("Cache-Control") == "no-store");
    assert(logout.headers.at("Pragma") == "no-cache");
    const std::string clearedCookie = logout.headers.at("Set-Cookie");
    assert(clearedCookie.rfind("vdr_suite_session=;", 0) == 0);
    assert(clearedCookie.find("Max-Age=0") != std::string::npos);
    assert(clearedCookie.find("Expires=Thu, 01 Jan 1970 00:00:00 GMT") !=
        std::string::npos);
    assert(clearedCookie.find("HttpOnly") != std::string::npos);
    assert(clearedCookie.find("Secure") != std::string::npos);
    assert(clearedCookie.find("SameSite=Strict") != std::string::npos);

    const auto revokedBrowser =
        credentialRepository.findBySessionId(sessionId);
    const auto revokedSession = identityRepository.findSession(sessionId);
    const auto revokedCredential =
        identityRepository.findCredential(credentialId);
    assert(revokedBrowser.has_value());
    assert(!revokedBrowser->active && revokedBrowser->revoked);
    assert(revokedSession.has_value());
    assert(!revokedSession->active && revokedSession->revoked);
    assert(revokedCredential.has_value());
    assert(!revokedCredential->active && revokedCredential->revoked);
    assert(authenticator.authenticate(
        logoutHeaders,
        "request-after-logout",
        "").authenticationState == AuthenticationState::Revoked);

    RequestSecurityContext invalidContext;
    invalidContext.requestId = "request-invalid-login";
    const HttpServerResponse invalidLogin =
        httpService.login(invalidContext);
    assert(invalidLogin.statusCode == 401);
    assert(invalidLogin.headers.find("Set-Cookie") ==
        invalidLogin.headers.end());
    assert(invalidLogin.body.find("invalid_session_issuance_context") !=
        std::string::npos);

    BrowserSessionLifetimeConfiguration invalidLifetimeConfiguration;
    invalidLifetimeConfiguration.configuredValueValid = false;
    BrowserSessionHttpService invalidLifetimeService(
        issuanceService,
        lifecycleService,
        invalidLifetimeConfiguration);
    basicContext.requestId = "request-invalid-lifetime";
    const HttpServerResponse invalidLifetimeLogin =
        invalidLifetimeService.login(basicContext);
    assert(invalidLifetimeLogin.statusCode == 503);
    assert(invalidLifetimeLogin.headers.find("Set-Cookie") ==
        invalidLifetimeLogin.headers.end());
    assert(invalidLifetimeLogin.headers.at("Cache-Control") == "no-store");
    assert(invalidLifetimeLogin.body.find(
        "browser_session_lifetime_configuration_invalid") !=
        std::string::npos);
    assert(invalidLifetimeLogin.body.find("request-invalid-lifetime") !=
        std::string::npos);

    BrowserSessionLifetimeConfiguration belowMinimumConfiguration;
    belowMinimumConfiguration.seconds = 299;
    BrowserSessionHttpService belowMinimumService(
        issuanceService,
        lifecycleService,
        belowMinimumConfiguration);
    const HttpServerResponse belowMinimumLogin =
        belowMinimumService.login(basicContext);
    assert(belowMinimumLogin.statusCode == 503);
    assert(belowMinimumLogin.headers.find("Set-Cookie") ==
        belowMinimumLogin.headers.end());

    return 0;
}
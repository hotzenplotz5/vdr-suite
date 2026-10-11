#include "MediaAccessCredentialHttp.h"

#include <cassert>
#include <string>

int main()
{
    const std::string sessionId =
        "ms_0123456789abcdef0123456789abcdef";
    const std::string credential =
        "mg_0123456789abcdef0123456789abcdef."
        "AbCdEfGhIjKlMnOpQrStUvWxYz0123456789-_";

    assert(std::string(MediaAccessCredentialHttp::CookieName) ==
        "vdr_suite_media");
    assert(std::string(MediaAccessCredentialHttp::AuthorizationHeader) ==
        "X-VDR-Suite-Media-Authorization");

    const std::string path =
        "/api/media/sessions/" + sessionId + "/";
    assert(MediaAccessCredentialHttp::cookiePath(sessionId) == path);

    assert(MediaAccessCredentialHttp::sessionCookie(
        sessionId,
        credential,
        21600) ==
        "vdr_suite_media=" + credential +
        "; Path=" + path +
        "; Max-Age=21600; HttpOnly; Secure; SameSite=Strict");

    assert(MediaAccessCredentialHttp::expiredSessionCookie(sessionId) ==
        "vdr_suite_media=; Path=" + path +
        "; Max-Age=0; Expires=Thu, 01 Jan 1970 00:00:00 GMT; "
        "HttpOnly; Secure; SameSite=Strict");

    // The Web/browser credential remains restricted to its legacy path.
    // Public-v1 media access uses a different, explicitly scoped cookie.
    const std::string publicV1Path =
        "/api/v1/media/sessions/" + sessionId + "/";
    const std::string externalPublicV1Path =
        "/vdr-suite" + publicV1Path;
    assert(MediaAccessCredentialHttp::publicV1CookiePath(
        sessionId, "") == publicV1Path);
    assert(MediaAccessCredentialHttp::publicV1CookiePath(
        sessionId, "/vdr-suite") == externalPublicV1Path);
    assert(MediaAccessCredentialHttp::publicV1SessionCookie(
        sessionId, credential, 300, "/vdr-suite") ==
        "vdr_suite_media=" + credential +
        "; Path=" + externalPublicV1Path +
        "; Max-Age=300; HttpOnly; Secure; SameSite=Strict");
    assert(MediaAccessCredentialHttp::expiredPublicV1SessionCookie(
        sessionId, "/vdr-suite") ==
        "vdr_suite_media=; Path=" + externalPublicV1Path +
        "; Max-Age=0; Expires=Thu, 01 Jan 1970 00:00:00 GMT; "
        "HttpOnly; Secure; SameSite=Strict");

    for (const std::string& invalidPrefix : {
        std::string("vdr-suite"),
        std::string("/"),
        std::string("/vdr-suite/"),
        std::string("//vdr-suite"),
        std::string("/vdr//suite"),
        std::string("/../api"),
        std::string("/./api"),
        std::string("/vdr-suite%2Fapi"),
        std::string("/vdr-suite?path=/"),
        std::string("/vdr-suite; Secure"),
        std::string("/vdr-suite\\other"),
        std::string("/vdr-suite\r\nSet-Cookie: bad=1"),
        std::string("/") + std::string(130, 'a')
    }) {
        assert(MediaAccessCredentialHttp::publicV1CookiePath(
            sessionId, invalidPrefix).empty());
        assert(MediaAccessCredentialHttp::publicV1SessionCookie(
            sessionId, credential, 300, invalidPrefix).empty());
        assert(MediaAccessCredentialHttp::expiredPublicV1SessionCookie(
            sessionId, invalidPrefix).empty());
    }
    assert(MediaAccessCredentialHttp::publicV1CookiePath(
        "../escape", "/vdr-suite").empty());
    assert(MediaAccessCredentialHttp::publicV1SessionCookie(
        sessionId, "grant;Path=/", 300, "/vdr-suite").empty());
    assert(MediaAccessCredentialHttp::publicV1SessionCookie(
        sessionId, credential, 299, "/vdr-suite").empty());
    assert(MediaAccessCredentialHttp::publicV1SessionCookie(
        sessionId, credential, 21601, "/vdr-suite").empty());

    assert(MediaAccessCredentialHttp::cookiePath("../escape").empty());
    assert(MediaAccessCredentialHttp::cookiePath("session/child").empty());
    assert(MediaAccessCredentialHttp::sessionCookie(
        sessionId,
        "credential; Path=/",
        21600).empty());
    assert(MediaAccessCredentialHttp::sessionCookie(
        sessionId,
        "credential\r\nSet-Cookie=evil",
        21600).empty());
    assert(MediaAccessCredentialHttp::sessionCookie(
        sessionId,
        credential,
        299).empty());
    assert(MediaAccessCredentialHttp::sessionCookie(
        sessionId,
        credential,
        21601).empty());

    return 0;
}

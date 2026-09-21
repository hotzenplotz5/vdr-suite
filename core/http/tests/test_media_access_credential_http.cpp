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

#include "CurlExternalArtworkHttpTransport.h"

#include <cassert>
#include <set>
#include <string>

int main()
{
    const std::set<std::string> hosts = {
        "api.themoviedb.org",
        "image.tmdb.org"
    };

    assert(CurlExternalArtworkHttpTransport::isAllowedHttpsUrl(
        "https://api.themoviedb.org/3/tv/1/images", hosts));
    assert(CurlExternalArtworkHttpTransport::isAllowedHttpsUrl(
        "https://image.tmdb.org/t/p/original/test.jpg", hosts));
    assert(!CurlExternalArtworkHttpTransport::isAllowedHttpsUrl(
        "http://api.themoviedb.org/3/tv/1", hosts));
    assert(!CurlExternalArtworkHttpTransport::isAllowedHttpsUrl(
        "https://evil.api.themoviedb.org/3/tv/1", hosts));
    assert(!CurlExternalArtworkHttpTransport::isAllowedHttpsUrl(
        "https://api.themoviedb.org.evil.example/3/tv/1", hosts));
    assert(!CurlExternalArtworkHttpTransport::isAllowedHttpsUrl(
        "https://user@api.themoviedb.org/3/tv/1", hosts));
    assert(!CurlExternalArtworkHttpTransport::isAllowedHttpsUrl(
        "https://api.themoviedb.org:443/3/tv/1", hosts));
    assert(!CurlExternalArtworkHttpTransport::isAllowedHttpsUrl(
        "https://api.themoviedb.org\\evil", hosts));

    CurlExternalArtworkHttpTransport transport;
    ExternalArtworkHttpRequest invalid;
    invalid.url = "https://127.0.0.1/private";
    const ExternalArtworkHttpResponse response = transport.perform(invalid);
    assert(response.attempted);
    assert(response.transportError);
    assert(response.statusCode == 0);
    assert(response.body.empty());

    invalid.url = "https://api.themoviedb.org/3/tv/1";
    invalid.bearerToken = "bad token";
    const ExternalArtworkHttpResponse tokenResponse = transport.perform(invalid);
    assert(tokenResponse.attempted);
    assert(tokenResponse.transportError);

    return 0;
}

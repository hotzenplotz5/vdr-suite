#include "HttpServerResponse.h"
#include "PublicResourcePreconditions.h"
#include <openssl/sha.h>

#include <array>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>

// Compile the actual production asset table and dispatch, not a duplicate
// implementation. This also catches invalid C++17 in those include files.
namespace
{
#include "../src/TestHttpServerPaths.inc"
#include "../src/TestHttpServerAssets.inc"
#include "../src/TestHttpServerRoutes.inc"
}

int main()
{
    char directory[] = "/tmp/vdr-suite-diagnostics-routes.XXXXXX";
    char* root = mkdtemp(directory);
    assert(root != nullptr);
    const std::string rootPath(root);
    const std::string previousRoot = std::getenv("VDR_SUITE_FRONTEND_ROOT")
        ? std::getenv("VDR_SUITE_FRONTEND_ROOT") : "";
    assert(setenv("VDR_SUITE_FRONTEND_ROOT", root, 1) == 0);

    struct AssetCase {
        const char* name;
        const char* contentType;
    };
    const std::array<AssetCase, 5> assets = {{
        {"browser-artwork-probe.js", "application/javascript; charset=utf-8"},
        {"browser-performance-bridge.js", "application/javascript; charset=utf-8"},
        {"browser-performance-diagnostics.js", "application/javascript; charset=utf-8"},
        {"browser-performance-diagnostics.html", "text/html; charset=utf-8"},
        {"browser-performance-home.html", "text/html; charset=utf-8"}
    }};

    for (const AssetCase& asset : assets)
    {
        const std::string path = rootPath + "/" + asset.name;
        const std::string body = std::string("diagnostic-test:") + asset.name;
        {
            std::ofstream file(path, std::ios::binary);
            assert(file.good());
            file << body;
            assert(file.good());
        }
        const std::string request = std::string("/frontend/") + asset.name;
        const FrontendAsset* entry = findFrontendAsset(request);
        assert(entry != nullptr);
        assert(entry->addonRelativePath == nullptr);
        assert(isFrontendPath(request));
        const HttpServerResponse response = serveFrontendPath(request);
        assert(response.statusCode == 200);
        assert(response.headers.at("Content-Type") == asset.contentType);
        assert(response.headers.at("Cache-Control") == "no-cache");
        assert(response.body == body);
        assert(response.streamBodyPath.empty());
    }

    const std::string script = "/frontend/browser-artwork-probe.js";
    const auto initial = revalidateFrontendAsset(serveFrontendPath(script), "");
    assert(initial.statusCode == 200 && !initial.body.empty());
    const auto etag = initial.headers.at("ETag");
    assert(etag.compare(0, 2, "W/") == 0);
    assert(initial.headers.at("Vary") == "Accept-Encoding");
    for (const std::string& condition : {etag, etag.substr(2),
            std::string("\"other\", ") + etag, std::string("*")})
    {
        const auto unchanged = revalidateFrontendAsset(serveFrontendPath(script), condition);
        assert(unchanged.statusCode == 304 && unchanged.body.empty());
        assert(unchanged.headers.at("ETag") == etag);
        assert(unchanged.headers.at("Cache-Control") == "no-cache");
        assert(unchanged.headers.at("Vary") == "Accept-Encoding");
    }
    assert(revalidateFrontendAsset(serveFrontendPath(script), "malformed").statusCode == 200);
    {
        std::ofstream file(rootPath + "/browser-artwork-probe.js", std::ios::binary);
        file << "updated";
    }
    const auto updated = revalidateFrontendAsset(serveFrontendPath(script), etag);
    assert(updated.statusCode == 200 && updated.body == "updated");
    assert(updated.headers.at("ETag") != etag);
    const auto html = revalidateFrontendAsset(
        serveFrontendPath("/frontend/browser-performance-home.html"), "*");
    assert(html.statusCode == 200 && html.headers.count("ETag") == 0);
    assert(revalidateFrontendAsset(
        serveFrontendPath("/frontend/browser-performance-missing.html"), "*").statusCode == 404);

    // Revalidate the effective bundle, not just the primary script file.
    std::filesystem::create_directory(rootPath + "/modules");
    { std::ofstream file(rootPath + "/app.js"); file << "primary"; }
    { std::ofstream file(rootPath + "/modules/remote.js"); file << "addon-v1"; }
    const auto bundle = revalidateFrontendAsset(serveFrontendPath("/frontend/app.js"), "");
    assert(bundle.body == "primary\n\naddon-v1");
    { std::ofstream file(rootPath + "/modules/remote.js"); file << "addon-v2"; }
    const auto changedBundle = revalidateFrontendAsset(
        serveFrontendPath("/frontend/app.js"), bundle.headers.at("ETag"));
    assert(changedBundle.statusCode == 200 && changedBundle.body == "primary\n\naddon-v2");
    assert(changedBundle.headers.at("ETag") != bundle.headers.at("ETag"));
    assert(unlink((rootPath + "/modules/remote.js").c_str()) == 0);
    assert(rmdir((rootPath + "/modules").c_str()) == 0);
    assert(unlink((rootPath + "/app.js").c_str()) == 0);
    { std::ofstream file(rootPath + "/style.css"); file << "body{}"; }
    const auto css = revalidateFrontendAsset(serveFrontendPath("/frontend/style.css"), "");
    assert(css.statusCode == 200 && css.body == "body{}");
    assert(revalidateFrontendAsset(serveFrontendPath("/frontend/style.css"),
        css.headers.at("ETag")).statusCode == 304);
    assert(unlink((rootPath + "/style.css").c_str()) == 0);

    assert(!isFrontendPath("/frontend/browser-performance-missing.html"));
    assert(serveFrontendPath("/frontend/browser-performance-missing.html").statusCode == 404);
    assert(isFrontendPath("/frontend/"));
    assert(findFrontendAsset("/frontend/app.js") != nullptr);
    assert(findFrontendAsset("/frontend/home-recording-discovery.js") != nullptr);

    if (previousRoot.empty())
        assert(unsetenv("VDR_SUITE_FRONTEND_ROOT") == 0);
    else
        assert(setenv("VDR_SUITE_FRONTEND_ROOT", previousRoot.c_str(), 1) == 0);
    for (const AssetCase& asset : assets)
        assert(unlink((rootPath + "/" + asset.name).c_str()) == 0);
    assert(rmdir(root) == 0);
    std::cout << "production browser diagnostics asset routes ok\n";
    return 0;
}

#include "HttpServerResponse.h"

#include <array>
#include <cassert>
#include <cctype>
#include <cstdlib>
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

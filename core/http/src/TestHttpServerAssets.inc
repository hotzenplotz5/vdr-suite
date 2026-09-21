HttpServerResponse makeStaticNotFoundResponse()
{
    HttpServerResponse response;

    response.statusCode = 404;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"error\":\"frontend asset not found\"}";

    return response;
}

bool readFile(
    const std::string& path,
    std::string& content)
{
    std::ifstream file(path, std::ios::binary);

    if (!file)
    {
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();

    return true;
}

std::vector<std::string> frontendRoots()
{
    const char* configuredRoot =
        std::getenv("VDR_SUITE_FRONTEND_ROOT");

    std::vector<std::string> roots;

    if (configuredRoot != nullptr && configuredRoot[0] != '\0')
    {
        roots.emplace_back(configuredRoot);
    }

    roots.emplace_back("web/frontend");
    roots.emplace_back("/usr/share/vdr-suite/web/frontend");
    roots.emplace_back("/usr/local/share/vdr-suite/web/frontend");

    return roots;
}

std::vector<std::string> channelLogoRoots()
{
    const char* configuredRoot =
        std::getenv("VDR_SUITE_CHANNEL_LOGO_ROOT");
    const char* configuredCache =
        std::getenv("VDR_SUITE_CHANNEL_LOGO_CACHE");

    std::vector<std::string> roots;

    if (configuredRoot != nullptr && configuredRoot[0] != '\0')
    {
        roots.emplace_back(configuredRoot);
    }

    if (configuredCache != nullptr && configuredCache[0] != '\0')
    {
        roots.emplace_back(configuredCache);
    }

    roots.emplace_back("/var/cache/vdr-suite/channel-logos");
    roots.emplace_back(
        "/usr/share/vdr/plugins/skindesigner/skins/estuary4vdr/logos");
    roots.emplace_back(
        "/usr/share/vdr/plugins/skindesigner/logos");

    return roots;
}

bool readFrontendAsset(
    const std::string& relativePath,
    std::string& content)
{
    for (const std::string& root : frontendRoots())
    {
        const std::string path =
            root + "/" + relativePath;

        if (readFile(path, content))
        {
            return true;
        }
    }

    return false;
}

bool readChannelLogoAsset(
    const std::string& relativePath,
    std::string& content)
{
    for (const std::string& root : channelLogoRoots())
    {
        const std::string path =
            root + "/" + relativePath;

        if (readFile(path, content))
        {
            return true;
        }
    }

    return false;
}

HttpServerResponse makeFrontendAssetResponse(
    const std::string& relativePath,
    const std::string& contentType)
{
    std::string content;

    if (!readFrontendAsset(relativePath, content))
    {
        return makeStaticNotFoundResponse();
    }

    HttpServerResponse response;

    response.statusCode = 200;
    response.headers["Content-Type"] = contentType;
    response.headers["Cache-Control"] = "no-cache";
    response.body = content;

    return response;
}

HttpServerResponse makeFrontendScriptBundleResponse(
    const std::string& primaryRelativePath,
    const std::string& addonRelativePath)
{
    std::string content;

    if (!readFrontendAsset(primaryRelativePath, content))
    {
        return makeStaticNotFoundResponse();
    }

    std::string addon;

    if (readFrontendAsset(addonRelativePath, addon))
    {
        content += "\n\n";
        content += addon;
    }

    HttpServerResponse response;

    response.statusCode = 200;
    response.headers["Content-Type"] =
        "application/javascript; charset=utf-8";
    response.headers["Cache-Control"] = "no-cache";
    response.body = content;

    return response;
}

HttpServerResponse makeMissingChannelLogoResponse(
    const std::string& relativePath)
{
    HttpServerResponse response;

    response.statusCode = 204;
    response.headers["Content-Type"] = logoContentType(relativePath);
    response.headers["Cache-Control"] = "no-cache";
    response.body = "";

    return response;
}

HttpServerResponse makeChannelLogoResponse(
    const std::string& path)
{
    const std::string prefix = "/channel-logos/";
    const std::string relativePath =
        percentDecodePath(path.substr(prefix.size()));

    if (!isSafeRelativePath(relativePath) ||
        !isSupportedLogoPath(relativePath))
    {
        return makeStaticNotFoundResponse();
    }

    std::string content;

    if (!readChannelLogoAsset(relativePath, content))
    {
        return makeMissingChannelLogoResponse(relativePath);
    }

    HttpServerResponse response;

    response.statusCode = 200;
    response.headers["Content-Type"] = logoContentType(relativePath);
    response.headers["Cache-Control"] = "no-cache";
    response.body = content;

    return response;
}

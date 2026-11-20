bool isIndexPath(const std::string& path)
{
    return path == "/" ||
        path == "/frontend" ||
        path == "/frontend/" ||
        path == "/frontend/index.html";
}

const FrontendAsset* findFrontendAsset(const std::string& path)
{
    for (const FrontendAsset& asset : FRONTEND_ASSETS)
    {
        if (path == asset.requestPath)
        {
            return &asset;
        }
    }

    return nullptr;
}

// Versioned static URLs keep the resource identity in the existing
// exact frontend asset allowlist; only the cache key uses the query.
std::string frontendAssetRequestPath(const std::string& requestTarget)
{
    const auto query = requestTarget.find('?');
    return query == std::string::npos
        ? requestTarget
        : requestTarget.substr(0, query);
}

bool isFrontendPath(const std::string& path)
{
    const std::string resource = frontendAssetRequestPath(path);
    return isIndexPath(resource) || findFrontendAsset(resource) != nullptr;
}

bool isChannelLogoPath(const std::string& path)
{
    return startsWith(path, "/channel-logos/");
}

HttpServerResponse serveFrontendPath(const std::string& path)
{
    const std::string resource = frontendAssetRequestPath(path);
    if (isIndexPath(resource))
    {
        return makeFrontendAssetResponse(
            "index.html",
            "text/html; charset=utf-8");
    }

    const FrontendAsset* asset = findFrontendAsset(resource);

    if (asset == nullptr)
    {
        return makeStaticNotFoundResponse();
    }

    if (asset->addonRelativePath != nullptr)
    {
        return makeFrontendScriptBundleResponse(
            asset->relativePath,
            asset->addonRelativePath);
    }

    return makeFrontendAssetResponse(
        asset->relativePath,
        asset->contentType);
}


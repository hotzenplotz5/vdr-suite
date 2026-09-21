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

bool isFrontendPath(const std::string& path)
{
    return isIndexPath(path) || findFrontendAsset(path) != nullptr;
}

bool isChannelLogoPath(const std::string& path)
{
    return startsWith(path, "/channel-logos/");
}

HttpServerResponse serveFrontendPath(const std::string& path)
{
    if (isIndexPath(path))
    {
        return makeFrontendAssetResponse(
            "index.html",
            "text/html; charset=utf-8");
    }

    const FrontendAsset* asset = findFrontendAsset(path);

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


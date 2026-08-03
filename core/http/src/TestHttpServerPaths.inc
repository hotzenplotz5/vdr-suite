struct FrontendAsset
{
    const char* requestPath;
    const char* relativePath;
    const char* contentType;
    const char* addonRelativePath;
};

const FrontendAsset FRONTEND_ASSETS[] = {
    {"/frontend/app.js", "app.js", "application/javascript; charset=utf-8", "modules/remote.js"},
    {"/frontend/api/client-api.js", "api/client-api.js", "application/javascript; charset=utf-8", "api/live-remote-client-api.js"},
    {"/frontend/api/genre-client-api.js", "api/genre-client-api.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/platform/public-url.js", "platform/public-url.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/platform/bootstrap.js", "platform/bootstrap.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/platform/i18n.js", "platform/i18n.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/platform/helpers.js", "platform/helpers.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/platform/deferred-runtime-loader.js", "platform/deferred-runtime-loader.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/settings-series-artwork.js", "settings-series-artwork.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/locales/de.js", "locales/de.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/locales/en.js", "locales/en.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/channel-logos.js", "channel-logos.js", "application/javascript; charset=utf-8", "epg-cache.js"},
    {"/frontend/channel-day-program.js", "channel-day-program.js", "application/javascript; charset=utf-8", "api/session-frontend-sync.js"},
    {"/frontend/channel-day-program-compat.js", "channel-day-program-compat.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/recordings2-shared.js", "recordings2-shared.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/recordings2-folder-artwork.js", "recordings2-folder-artwork.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/recordings2-actions.js", "recordings2-actions.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/recordings2-browser-view.js", "recordings2-browser-view.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/recordings2-person-search-view.js", "recordings2-person-search-view.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/recordings2-metadata-view.js", "recordings2-metadata-view.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/recordings2.js", "recordings2.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/recordings2-metadata-detail.js", "recordings2-metadata-detail.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/epg-searchtimer-actions.js", "epg-searchtimer-actions.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/epg-detail-owner.js", "epg-detail-owner.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/channel-browser.js", "channel-browser.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/modules/channels.js", "modules/channels.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/modules/timers.js", "modules/timers.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/modules/searchtimers.js", "modules/searchtimers.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/modules/genres.js", "modules/genres.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/modules/global-search.js", "modules/global-search.js", "application/javascript; charset=utf-8", nullptr},
    {"/frontend/style.css", "style.css", "text/css; charset=utf-8", nullptr}
};

std::string toLowerAscii(const std::string& value)
{
    std::string lowered;
    lowered.reserve(value.size());

    for (const char character : value)
    {
        lowered.push_back(
            static_cast<char>(
                std::tolower(
                    static_cast<unsigned char>(character))));
    }

    return lowered;
}

bool startsWith(
    const std::string& value,
    const std::string& prefix)
{
    return value.size() >= prefix.size() &&
        value.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(
    const std::string& value,
    const std::string& suffix)
{
    return value.size() >= suffix.size() &&
        value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int hexValue(const char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }

    if (character >= 'a' && character <= 'f')
    {
        return 10 + character - 'a';
    }

    if (character >= 'A' && character <= 'F')
    {
        return 10 + character - 'A';
    }

    return -1;
}

std::string percentDecodePath(const std::string& value)
{
    std::string decoded;
    decoded.reserve(value.size());

    for (std::size_t index = 0; index < value.size(); ++index)
    {
        if (value[index] == '%' && index + 2 < value.size())
        {
            const int high = hexValue(value[index + 1]);
            const int low = hexValue(value[index + 2]);

            if (high >= 0 && low >= 0)
            {
                decoded.push_back(
                    static_cast<char>((high * 16) + low));
                index += 2;
                continue;
            }
        }

        decoded.push_back(value[index]);
    }

    return decoded;
}

bool isSafeRelativePath(const std::string& path)
{
    if (path.empty() ||
        path[0] == '/' ||
        path.find('\\') != std::string::npos ||
        path.find("//") != std::string::npos)
    {
        return false;
    }

    std::size_t segmentStart = 0;

    while (segmentStart <= path.size())
    {
        const std::size_t segmentEnd =
            path.find('/', segmentStart);
        const std::string segment =
            path.substr(
                segmentStart,
                segmentEnd == std::string::npos
                    ? std::string::npos
                    : segmentEnd - segmentStart);

        if (segment.empty() || segment == "." || segment == "..")
        {
            return false;
        }

        if (segmentEnd == std::string::npos)
        {
            break;
        }

        segmentStart = segmentEnd + 1;
    }

    return true;
}

bool isSupportedLogoPath(const std::string& path)
{
    const std::string lowered = toLowerAscii(path);

    return endsWith(lowered, ".png") ||
        endsWith(lowered, ".svg");
}

std::string logoContentType(const std::string& path)
{
    const std::string lowered = toLowerAscii(path);

    if (endsWith(lowered, ".svg"))
    {
        return "image/svg+xml";
    }

    return "image/png";
}

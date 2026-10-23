#pragma once

#include <cstdlib>
#include <string>
#include <sys/un.h>

namespace vdrsuite::agent
{

// The VDR plugin and its local clients share one optional process environment
// setting. Only private runtime sockets under /run are accepted. An invalid
// explicitly supplied path is NOT silently replaced by the default.
inline bool validSuiteBridgeControlSocketPath(const std::string& path)
{
    if (path.size() <= 5 || path.size() >= sizeof(sockaddr_un::sun_path) ||
        path.compare(0, 5, "/run/") != 0 ||
        path.back() == '/')
    {
        return false;
    }
    std::size_t start = 1;
    while (start < path.size())
    {
        const std::size_t end = path.find('/', start);
        const std::string component = path.substr(
            start, end == std::string::npos
                ? std::string::npos
                : end - start);
        if (component.empty() || component == "." || component == "..")
        {
            return false;
        }
        for (unsigned char c : component)
        {
            if (c < 0x21 || c == 0x7f)
            {
                return false;
            }
        }
        if (end == std::string::npos)
        {
            break;
        }
        start = end + 1;
    }
    return true;
}

inline std::string suiteBridgeControlSocketPathFromEnvironment()
{
    const char* configured = std::getenv("VDR_SUITE_CONTROL_SOCKET_PATH");
    if (configured == nullptr || *configured == '\0')
    {
        return "/run/vdr/vdr-suite-control/control.sock";
    }

    const std::string path(configured);
    return validSuiteBridgeControlSocketPath(path)
        ? path
        : std::string();
}

} // namespace vdrsuite::agent

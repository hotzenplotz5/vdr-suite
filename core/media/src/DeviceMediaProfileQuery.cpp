#include "DeviceMediaProfileQuery.h"

#include <map>

std::optional<DeviceMediaProfileKey> parseDeviceMediaProfileQuery(
    const std::string& target,
    const std::string& collection,
    const std::string& authenticatedDeviceId)
{
    if (target.size() > 1024U || collection.empty() ||
        target.rfind(collection + "?", 0) != 0) return std::nullopt;
    const std::string params = target.substr(collection.size() + 1U);
    std::map<std::string, std::string> fields;
    std::size_t at = 0;
    while (at < params.size()) {
        const auto end = params.find('&', at);
        const std::string token = params.substr(at,
            end == std::string::npos ? end : end-at);
        const auto eq = token.find('=');
        if (eq == std::string::npos || eq == 0 ||
            eq + 1 == token.size() || token.find('=', eq + 1) != std::string::npos)
            return std::nullopt;
        const auto name = token.substr(0,eq);
        const auto value = token.substr(eq+1);
        if (name != "appVersion" && name != "firmwareVersion" &&
            name != "playerVersion" && name != "engine")
            return std::nullopt;
        if (!fields.emplace(name,value).second) return std::nullopt;
        if (end == std::string::npos) break;
        at = end + 1;
    }
    if (fields.size() != 4U) return std::nullopt;
    DeviceMediaProfileKey key;
    key.deviceId = authenticatedDeviceId;
    key.appVersion = fields["appVersion"];
    key.firmwareVersion = fields["firmwareVersion"];
    key.playerVersion = fields["playerVersion"];
    const std::string engine = fields["engine"];
    if (engine == "mse") key.engine = MediaPlaybackEngine::MediaSourceExtensions;
    else if (engine == "native") key.engine = MediaPlaybackEngine::Native;
    else if (engine == "external") key.engine = MediaPlaybackEngine::External;
    else return std::nullopt;
    if (!key.valid()) return std::nullopt;
    return key;
}

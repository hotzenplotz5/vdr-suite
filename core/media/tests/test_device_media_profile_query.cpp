#include "DeviceMediaProfileQuery.h"
#include <cassert>

int main()
{
    const std::string collection =
        "/api/v1/recording-playback-sessions/capabilities";
    const auto valid = collection+
        "?appVersion=b20261011.24&firmwareVersion=vidaa9"
        "&playerVersion=mse_v1&engine=mse";
    const auto parsed = parseDeviceMediaProfileQuery(valid, collection, "dev_a");
    assert(parsed && parsed->valid() && parsed->deviceId == "dev_a");
    assert(parsed->engine == MediaPlaybackEngine::MediaSourceExtensions);
    assert(!parseDeviceMediaProfileQuery(valid, collection, "").has_value());
    assert(!parseDeviceMediaProfileQuery(valid+"&deviceId=dev_b",
        collection, "dev_a").has_value());
    assert(!parseDeviceMediaProfileQuery(valid+"&engine=native",
        collection, "dev_a").has_value());
    assert(!parseDeviceMediaProfileQuery(collection,collection,"dev_a").has_value());
    assert(!parseDeviceMediaProfileQuery(
        collection+"?appVersion=x&firmwareVersion=../../evil&"
        "playerVersion=mse_v1&engine=mse",collection,"dev_a").has_value());
    assert(!parseDeviceMediaProfileQuery(
        collection+"?appVersion=x&firmwareVersion=v&"
        "playerVersion=mse_v1&engine=hacked",collection,"dev_a").has_value());
}

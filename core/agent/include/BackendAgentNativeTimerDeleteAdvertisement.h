#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerDelete.h"

#include <algorithm>
#include <cstddef>
#include <string>

namespace vdrsuite::agent
{

inline bool backendAgentNativeTimerDeleteAdvertisementValid(
    const BackendAgentCommandPollRequest& request,
    std::string& reasonCode)
{
    const auto advertised = std::count(
        request.supportedCommandTypes.begin(),
        request.supportedCommandTypes.end(),
        std::string(kBackendAgentNativeTimerDeleteCommandType));
    if (advertised == 0)
    {
        reasonCode.clear();
        return true;
    }
    if (advertised != 1)
    {
        reasonCode = "invalid_native_timer_delete_advertisement";
        return false;
    }

    const auto provider = std::find_if(
        request.localProviders.begin(), request.localProviders.end(),
        [](const BackendAgentLocalProviderFacts& facts) {
            return facts.providerId == kBackendAgentNativeTimerDeleteProviderId;
        });
    if (provider == request.localProviders.end() ||
        !backendAgentLocalProviderValidFacts(*provider) ||
        provider->providerKind != kBackendAgentNativeTimerDeleteProviderKind ||
        !provider->available ||
        std::find(
            provider->capabilities.begin(), provider->capabilities.end(),
            kBackendAgentNativeTimerDeleteCapability) ==
            provider->capabilities.end())
    {
        reasonCode = "native_timer_delete_provider_advertisement_required";
        return false;
    }

    reasonCode = "native_timer_delete_advertisement_accepted";
    return true;
}

} // namespace vdrsuite::agent

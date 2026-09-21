#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentLocalProvider.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerDelete.h"
#include "BackendAgentNativeTimerModify.h"

#include <algorithm>
#include <string>

namespace vdrsuite::agent
{
namespace detail
{
inline bool timerAdvertisementRequirement(
    const BackendAgentCommandPollRequest& request,
    const char* commandType,
    const char* providerId,
    const char* providerKind,
    const char* capability,
    const char* duplicateReason,
    const char* providerReason,
    std::string& reasonCode)
{
    const auto advertised = std::count(
        request.supportedCommandTypes.begin(),
        request.supportedCommandTypes.end(),
        std::string(commandType));
    if (advertised == 0) return true;
    if (advertised != 1)
    {
        reasonCode = duplicateReason;
        return false;
    }

    const auto provider = std::find_if(
        request.localProviders.begin(), request.localProviders.end(),
        [&](const BackendAgentLocalProviderFacts& facts) {
            return facts.providerId == providerId;
        });
    if (provider == request.localProviders.end() ||
        !backendAgentLocalProviderValidFacts(*provider) ||
        provider->providerKind != providerKind ||
        !provider->available ||
        std::find(
            provider->capabilities.begin(),
            provider->capabilities.end(),
            capability) == provider->capabilities.end())
    {
        reasonCode = providerReason;
        return false;
    }
    return true;
}
} // namespace detail

inline bool backendAgentNativeTimerAdvertisementValid(
    const BackendAgentCommandPollRequest& request,
    std::string& reasonCode)
{
    if (!detail::timerAdvertisementRequirement(
            request,
            kBackendAgentNativeTimerCreateCommandType,
            kBackendAgentNativeTimerCreateProviderId,
            kBackendAgentNativeTimerCreateProviderKind,
            kBackendAgentNativeTimerCreateCapability,
            "invalid_native_timer_create_advertisement",
            "native_timer_create_provider_advertisement_required",
            reasonCode) ||
        !detail::timerAdvertisementRequirement(
            request,
            kBackendAgentNativeTimerUpdateCommandType,
            kBackendAgentNativeTimerModifyProviderId,
            kBackendAgentNativeTimerModifyProviderKind,
            kBackendAgentNativeTimerUpdateCapability,
            "invalid_native_timer_update_advertisement",
            "native_timer_update_provider_advertisement_required",
            reasonCode) ||
        !detail::timerAdvertisementRequirement(
            request,
            kBackendAgentNativeTimerToggleCommandType,
            kBackendAgentNativeTimerModifyProviderId,
            kBackendAgentNativeTimerModifyProviderKind,
            kBackendAgentNativeTimerToggleCapability,
            "invalid_native_timer_toggle_advertisement",
            "native_timer_toggle_provider_advertisement_required",
            reasonCode) ||
        !detail::timerAdvertisementRequirement(
            request,
            kBackendAgentNativeTimerDeleteCommandType,
            kBackendAgentNativeTimerDeleteProviderId,
            kBackendAgentNativeTimerDeleteProviderKind,
            kBackendAgentNativeTimerDeleteCapability,
            "invalid_native_timer_delete_advertisement",
            "native_timer_delete_provider_advertisement_required",
            reasonCode))
        return false;

    reasonCode = "native_timer_advertisement_accepted";
    return true;
}

inline bool backendAgentNativeTimerDeleteAdvertisementValid(
    const BackendAgentCommandPollRequest& request,
    std::string& reasonCode)
{
    return backendAgentNativeTimerAdvertisementValid(request, reasonCode);
}

} // namespace vdrsuite::agent

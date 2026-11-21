#pragma once

#include "DashboardController.h"
#include "SuiteBridgeHbbtvMediaResolver.h"

#include <cstdint>
#include <functional>
#include <string>

class HbbtvApplicationSessionService;
class IHbbtvApplicationDiscoveryService;
class IHbbtvPresentationSource;
class IHbbtvMediaSourceResolver;

enum class HbbtvMediaSessionMutationOperation
{
    Start,
    Stop
};

struct HbbtvMediaSessionMutationRequest
{
    HbbtvMediaSessionMutationOperation operation =
        HbbtvMediaSessionMutationOperation::Stop;
    std::string actorId;
    std::string clientContext;
    std::string backendId;
    std::string sessionId;
    std::uint64_t mediaRevision = 0;
    HbbtvMediaSource media;
};

class HbbtvApiRuntime
{
public:
    using PresentationLookup =
        std::function<IHbbtvPresentationSource*(const std::string& backendId)>;
    using MediaLookup =
        std::function<IHbbtvMediaSourceResolver*(const std::string& backendId)>;
    using MediaSessionHandler =
        std::function<ApiResponse(const HbbtvMediaSessionMutationRequest&)>;
    static HbbtvApiRuntime& instance();

    bool configure(
        IHbbtvApplicationDiscoveryService& readService,
        HbbtvApplicationSessionService& sessionService,
        PresentationLookup presentationLookup = {},
        MediaLookup mediaLookup = {});
    void setMediaSessionHandler(MediaSessionHandler handler);
    void reset();
    bool configured() const;

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response,
        const std::string& actorRef = "",
        const std::string& clientRef = "") const;

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& body,
        const std::string& actorRef,
        const std::string& clientRef,
        const std::string& correlationRef,
        ApiResponse& response) const;

private:
    HbbtvApiRuntime() = default;

    IHbbtvApplicationDiscoveryService* readService_ = nullptr;
    HbbtvApplicationSessionService* sessionService_ = nullptr;
    PresentationLookup presentationLookup_;
    MediaLookup mediaLookup_;
    MediaSessionHandler mediaSessionHandler_;
};

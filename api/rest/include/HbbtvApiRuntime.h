#pragma once

#include "DashboardController.h"

#include <functional>
#include <string>

class HbbtvApplicationSessionService;
class IHbbtvApplicationDiscoveryService;
class IHbbtvPresentationSource;

class HbbtvApiRuntime
{
public:
    using PresentationLookup =
        std::function<IHbbtvPresentationSource*(const std::string& backendId)>;
    static HbbtvApiRuntime& instance();

    bool configure(
        IHbbtvApplicationDiscoveryService& readService,
        HbbtvApplicationSessionService& sessionService,
        PresentationLookup presentationLookup = {});
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
};

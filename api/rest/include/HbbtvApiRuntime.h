#pragma once

#include "DashboardController.h"

#include <string>

class HbbtvApplicationSessionService;
class IHbbtvApplicationDiscoveryService;

class HbbtvApiRuntime
{
public:
    static HbbtvApiRuntime& instance();

    bool configure(
        IHbbtvApplicationDiscoveryService& readService,
        HbbtvApplicationSessionService& sessionService);
    void reset();
    bool configured() const;

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response) const;

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
};

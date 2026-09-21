#pragma once

#include "DashboardController.h"

#include <string>

class LegacyOsdSessionService;
class OsdViewerBindingService;

class LegacyOsdApiRuntime
{
public:
    static LegacyOsdApiRuntime& instance();
    bool configure(LegacyOsdSessionService& sessionService);
    bool configure(
        LegacyOsdSessionService& sessionService,
        OsdViewerBindingService& viewerService);
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
    LegacyOsdApiRuntime() = default;
    LegacyOsdSessionService* sessionService_ = nullptr;
    OsdViewerBindingService* viewerService_ = nullptr;
};

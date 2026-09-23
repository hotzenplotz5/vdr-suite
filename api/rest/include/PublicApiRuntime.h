#pragma once

#include "DashboardController.h"

#include <string>

class PublicApiRuntime
{
public:
    static PublicApiRuntime& instance();

    bool tryHandleGet(
        const std::string& requestTarget,
        const std::string& actorRef,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response) const;

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& requestId,
        const std::string& correlationId,
        ApiResponse& response) const;

private:
    PublicApiRuntime() = default;
};

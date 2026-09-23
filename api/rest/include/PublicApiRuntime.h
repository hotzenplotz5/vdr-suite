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
        ApiResponse& response) const;

private:
    PublicApiRuntime() = default;
};

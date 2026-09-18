#pragma once

#include "DashboardController.h"

#include <string>

class HbbtvControlPlaneReadService;

class HbbtvApiRuntime
{
public:
    static HbbtvApiRuntime& instance();

    bool configure(HbbtvControlPlaneReadService& readService);
    void reset();
    bool configured() const;

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response) const;

private:
    HbbtvApiRuntime() = default;

    HbbtvControlPlaneReadService* readService_ = nullptr;
};

#pragma once

#include "PollingService.h"

#include <string>
#include <vector>

class BackendPollingCoordinator
{
public:
    void addPollingService(
        const std::string& backendId,
        PollingService& pollingService);

    void pollAll();

    int backendCount() const;

private:
    struct Entry
    {
        std::string backendId;
        PollingService* pollingService;
    };

    std::vector<Entry> entries_;
};

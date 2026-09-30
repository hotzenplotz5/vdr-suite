#pragma once

#include "HttpServerRequest.h"
#include "HttpServerResponse.h"

#include <atomic>
#include <string>

class FirstAdminClaimService;

class FirstAdminClaimHttpService
{
public:
    explicit FirstAdminClaimHttpService(
        FirstAdminClaimService& claimService);

    bool handles(const HttpServerRequest& request) const;
    HttpServerResponse handle(
        const HttpServerRequest& request) const;

private:
    std::string opaqueId(const std::string& prefix) const;

    FirstAdminClaimService& claimService_;
    mutable std::atomic<unsigned long long> idCounter_{0};
};

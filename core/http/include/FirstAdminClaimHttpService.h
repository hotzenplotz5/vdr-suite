#pragma once

#include "HttpServerRequest.h"
#include "HttpServerResponse.h"

#include <atomic>
#include <string>

class FirstAdminClaimService;
class FirstAdminBootstrapRepository;

class FirstAdminClaimHttpService
{
public:
    explicit FirstAdminClaimHttpService(
        FirstAdminClaimService& claimService,
        FirstAdminBootstrapRepository* bootstrapRepository = nullptr);

    bool handles(const HttpServerRequest& request) const;
    HttpServerResponse handle(
        const HttpServerRequest& request) const;

private:
    std::string opaqueId(const std::string& prefix) const;

    FirstAdminClaimService& claimService_;
    FirstAdminBootstrapRepository* bootstrapRepository_;
    mutable std::atomic<unsigned long long> idCounter_{0};
};

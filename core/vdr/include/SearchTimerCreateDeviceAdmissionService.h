#pragma once

#include "SearchTimerCreateAdmissionService.h"
#include "SecurityIdentity.h"

#include <cstdint>
#include <optional>
#include <string>

class AuthorizationService;
class BackendAccessPolicy;
class BackendRegistryService;

// Inject only a server-owned, live backend generation authority. Caller-
// supplied generations are intentionally absent from the Device request.
class ISearchTimerBackendGenerationAuthority
{
public:
    virtual ~ISearchTimerBackendGenerationAuthority() = default;
    virtual std::optional<std::uint64_t> currentGeneration(
        const std::string& backendId) const = 0;
};

enum class SearchTimerCreateDeviceAdmissionStatus
{
    prepared,
    replayed,
    deniedIdentity,
    deniedPermission,
    deniedBackend,
    generationUnavailable,
    invalid,
    conflict,
    unavailable,
};

struct SearchTimerCreateDeviceAdmissionRequest
{
    RequestSecurityContext context;
    std::string backendId;
    std::string idempotencyKey;
    std::int64_t requestedAt = 0;
    SearchTimerCreateOperationPayload payload;
};

struct SearchTimerCreateDeviceAdmissionResult
{
    SearchTimerCreateDeviceAdmissionStatus status =
        SearchTimerCreateDeviceAdmissionStatus::unavailable;
    vdrsuite::operations::MutationOperation operation;
};

// Domain-only preparation; NOT registered as a Public-v1 endpoint.
// No Device grant becomes assignable through this class and no RESTfulAPI
// mutation or dispatch is performed. The future HTTP boundary additionally
// owns authentication, validation, audit and operation response.
class SearchTimerCreateDeviceAdmissionService
{
public:
    SearchTimerCreateDeviceAdmissionService(
        SearchTimerCreateAdmissionService& admission,
        const AuthorizationService& authorization,
        const BackendAccessPolicy& access,
        const BackendRegistryService& backends,
        const ISearchTimerBackendGenerationAuthority& generations);

    SearchTimerCreateDeviceAdmissionResult admit(
        const SearchTimerCreateDeviceAdmissionRequest& request) const;

private:
    SearchTimerCreateAdmissionService& admission_;
    const AuthorizationService& authorization_;
    const BackendAccessPolicy& access_;
    const BackendRegistryService& backends_;
    const ISearchTimerBackendGenerationAuthority& generations_;
};

#include "BackendAccessPolicy.h"

#include "BackendRegistry.h"
#include "BackendRegistryService.h"

#include <cassert>
#include <iostream>
#include <string>

static BackendNode makeBackend(
    const std::string& backendId,
    const std::string& accessMode,
    bool enabled = true)
{
    BackendNode backend;
    backend.backendId = backendId;
    backend.backendName = backendId;
    backend.accessMode = accessMode;
    backend.enabled = enabled;
    return backend;
}

static void test_write_allowed_for_read_write_backend()
{
    BackendRegistry registry;
    registry.addBackend(makeBackend("living-room", "read-write"));
    BackendRegistryService service(registry);
    BackendAccessPolicy policy;

    const BackendAccessDecision decision =
        policy.canWriteToBackend(service, "living-room");

    assert(decision.allowed);
    assert(decision.backendFound);
    assert(!decision.readOnly);
    assert(decision.backendId == "living-room");
    assert(decision.accessMode == "read-write");
    assert(decision.reason == "backend write access allowed");
    assert(decision.errors.empty());
}

static void test_write_blocked_for_read_only_backend()
{
    BackendRegistry registry;
    registry.addBackend(makeBackend("remote-house", "read-only"));
    BackendRegistryService service(registry);
    BackendAccessPolicy policy;

    const BackendAccessDecision decision =
        policy.canWriteToBackend(service, "remote-house");

    assert(!decision.allowed);
    assert(decision.backendFound);
    assert(decision.readOnly);
    assert(decision.backendId == "remote-house");
    assert(decision.accessMode == "read-only");
    assert(decision.reason == "backend is read-only");
    assert(decision.errors.size() == 1);
    assert(decision.errors.at(0) == "backend is read-only");
}

static void test_write_blocked_for_missing_backend()
{
    BackendRegistry registry;
    BackendRegistryService service(registry);
    BackendAccessPolicy policy;

    const BackendAccessDecision decision =
        policy.canWriteToBackend(service, "missing");

    assert(!decision.allowed);
    assert(!decision.backendFound);
    assert(!decision.readOnly);
    assert(decision.backendId == "missing");
    assert(decision.reason == "backend not found");
    assert(decision.errors.size() == 1);
}

static void test_write_blocked_for_empty_backend_id()
{
    BackendRegistry registry;
    BackendRegistryService service(registry);
    BackendAccessPolicy policy;

    const BackendAccessDecision decision =
        policy.canWriteToBackend(service, "");

    assert(!decision.allowed);
    assert(!decision.backendFound);
    assert(decision.backendId.empty());
    assert(decision.reason == "backend id is required");
    assert(decision.errors.size() == 1);
}

static void test_write_blocked_for_disabled_backend()
{
    BackendRegistry registry;
    registry.addBackend(makeBackend("disabled", "read-write", false));
    BackendRegistryService service(registry);
    BackendAccessPolicy policy;

    const BackendAccessDecision decision =
        policy.canWriteToBackend(service, "disabled");

    assert(!decision.allowed);
    assert(decision.backendFound);
    assert(!decision.readOnly);
    assert(decision.reason == "backend is disabled");
    assert(decision.errors.size() == 1);
}

int main()
{
    test_write_allowed_for_read_write_backend();
    test_write_blocked_for_read_only_backend();
    test_write_blocked_for_missing_backend();
    test_write_blocked_for_empty_backend_id();
    test_write_blocked_for_disabled_backend();

    std::cout
        << "test_backend_access_policy passed"
        << std::endl;

    return 0;
}

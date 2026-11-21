#include "BackendRegistryController.h"

#include "BackendRegistry.h"
#include "BackendRegistryJsonSerializer.h"
#include "BackendRegistryService.h"

#include <cassert>
#include <string>

static BackendNode makeBackend(
    const std::string& id,
    const std::string& name,
    const std::string& accessMode = "read-write")
{
    BackendNode backend;

    backend.backendId = id;
    backend.backendName = name;
    backend.backendType = "vdr";
    backend.accessMode = accessMode;
    backend.enabled = true;

    return backend;
}

int main()
{
    BackendRegistry emptyRegistry;
    BackendRegistryService emptyService(emptyRegistry);
    BackendRegistryJsonSerializer emptySerializer;

    BackendRegistryController emptyController(
        emptyService,
        emptySerializer);

    ApiResponse emptyListResponse =
        emptyController.getBackends();

    assert(emptyListResponse.statusCode == 200);
    assert(emptyListResponse.contentType == "application/json");
    assert(emptyListResponse.body == "{\"backends\":[]}");

    ApiResponse missingDefaultResponse =
        emptyController.getDefaultBackend();

    assert(missingDefaultResponse.statusCode == 200);
    assert(missingDefaultResponse.contentType == "application/json");
    assert(missingDefaultResponse.body == "{}");

    BackendRegistry registry;

    registry.addBackend(
        makeBackend(
            "default",
            "Default VDR"));

    registry.addBackend(
        makeBackend(
            "ferienhaus",
            "Ferienhaus VDR",
            "read-only"));

    BackendRegistryService service(registry);
    BackendRegistryJsonSerializer serializer;

    BackendRegistryController controller(
        service,
        serializer);

    ApiResponse listResponse =
        controller.getBackends();

    assert(listResponse.statusCode == 200);
    assert(listResponse.contentType == "application/json");

    assert(
        listResponse.body.find(
            "\"backendId\":\"default\"")
        != std::string::npos);

    assert(
        listResponse.body.find(
            "\"backendId\":\"ferienhaus\"")
        != std::string::npos);

    assert(
        listResponse.body.find(
            "\"accessMode\":\"read-write\"")
        != std::string::npos);

    assert(
        listResponse.body.find(
            "\"accessMode\":\"read-only\"")
        != std::string::npos);

    assert(
        listResponse.body.find(
            "\"readOnly\":true")
        != std::string::npos);

    ApiResponse defaultResponse =
        controller.getDefaultBackend();

    assert(defaultResponse.statusCode == 200);

    assert(
        defaultResponse.body.find(
            "\"backendId\":\"default\"")
        != std::string::npos);

    assert(
        defaultResponse.body.find(
            "\"readOnly\":false")
        != std::string::npos);

    return 0;
}

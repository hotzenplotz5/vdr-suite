#include "BackendRegistryController.h"

#include "BackendRegistry.h"
#include "BackendRegistryJsonSerializer.h"
#include "BackendRegistryService.h"

#include <cassert>
#include <string>

static BackendNode makeBackend(
    const std::string& id,
    const std::string& name)
{
    BackendNode backend;

    backend.backendId = id;
    backend.backendName = name;
    backend.backendType = "vdr";
    backend.enabled = true;

    return backend;
}

int main()
{
    BackendRegistry registry;

    registry.addBackend(
        makeBackend(
            "default",
            "Default VDR"));

    registry.addBackend(
        makeBackend(
            "ferienhaus",
            "Ferienhaus VDR"));

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

    ApiResponse defaultResponse =
        controller.getDefaultBackend();

    assert(defaultResponse.statusCode == 200);

    assert(
        defaultResponse.body.find(
            "\"backendId\":\"default\"")
        != std::string::npos);

    return 0;
}

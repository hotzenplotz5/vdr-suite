
#include "BackendAccessPolicy.h"
#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "IVdrChannelMoveExecutor.h"
#include "VdrChannelMoveController.h"
#include "VdrChannelMoveExecutionService.h"
#include "VdrChannelMoveExecutorAdapterRegistry.h"
#include "VdrChannelMoveRequestParser.h"
#include "VdrChannelMoveResultJsonSerializer.h"

#include <cassert>
#include <memory>
#include <string>

class TestChannelMoveExecutor final : public IVdrChannelMoveExecutor
{
public:
    int callCount = 0;
    VdrChannelMoveRequest lastRequest;

    VdrChannelMoveResult moveChannel(
        const VdrChannelMoveRequest& request) override
    {
        ++callCount;
        lastRequest = request;

        VdrChannelMoveResult result;
        result.success = true;
        result.backendId = request.backendId;
        result.sourceNumber = request.sourceNumber;
        result.targetNumber = request.targetNumber;
        result.command =
            "MOVC " +
            std::to_string(request.sourceNumber) +
            " " +
            std::to_string(request.targetNumber);
        result.message = "test channel move completed";
        result.rawOutput = "250 channel moved";
        return result;
    }
};

int main()
{
    BackendRegistry backendRegistry;

    BackendNode defaultBackend;
    defaultBackend.backendId = "default";
    defaultBackend.backendName = "Default VDR";
    defaultBackend.backendType = "restfulapi";
    defaultBackend.enabled = true;
    defaultBackend.online = true;
    defaultBackend.accessMode = "read-write";
    backendRegistry.addBackend(defaultBackend);

    BackendNode readOnlyBackend;
    readOnlyBackend.backendId = "readonly";
    readOnlyBackend.backendName = "Read-only VDR";
    readOnlyBackend.backendType = "restfulapi";
    readOnlyBackend.enabled = true;
    readOnlyBackend.online = true;
    readOnlyBackend.accessMode = "read-only";
    backendRegistry.addBackend(readOnlyBackend);

    BackendNode noAdapterBackend;
    noAdapterBackend.backendId = "no-adapter";
    noAdapterBackend.backendName = "Writable without Channel Move adapter";
    noAdapterBackend.backendType = "restfulapi";
    noAdapterBackend.enabled = true;
    noAdapterBackend.online = true;
    noAdapterBackend.accessMode = "read-write";
    backendRegistry.addBackend(noAdapterBackend);

    BackendRegistryService backendRegistryService(backendRegistry);
    BackendAccessPolicy backendAccessPolicy;

    auto executor =
        std::make_shared<TestChannelMoveExecutor>();

    VdrChannelMoveExecutorAdapterRegistry registry;
    registry.registerAdapter(
        std::make_shared<VdrChannelMoveExecutorAdapter>(
            "default",
            executor));

    VdrChannelMoveExecutionService executionService;
    VdrChannelMoveResultJsonSerializer jsonSerializer;
    VdrChannelMoveRequestParser requestParser;

    VdrChannelMoveController controller(
        executionService,
        jsonSerializer,
        requestParser,
        registry,
        backendRegistryService,
        backendAccessPolicy);

    ApiResponse success =
        controller.moveBody(
            "{\"backendId\":\"default\",\"sourceNumber\":7,\"targetNumber\":3}");

    assert(success.statusCode == 200);
    assert(success.contentType == "application/json");
    assert(success.body.find("\"success\":true") != std::string::npos);
    assert(success.body.find("\"sourceNumber\":7") != std::string::npos);
    assert(success.body.find("\"targetNumber\":3") != std::string::npos);
    assert(executor->callCount == 1);
    assert(executor->lastRequest.sourceNumber == 7);
    assert(executor->lastRequest.targetNumber == 3);

    ApiResponse dryRun =
        controller.moveBody(
            "{\"backendId\":\"default\",\"sourceNumber\":8,\"targetNumber\":4,\"dryRun\":true}");

    assert(dryRun.statusCode == 200);
    assert(dryRun.body.find("\"success\":true") != std::string::npos);
    assert(dryRun.body.find("\"dryRun\":true") != std::string::npos);
    assert(dryRun.body.find("\"command\":\"MOVC 8 4\"") != std::string::npos);
    assert(executor->callCount == 1);

    ApiResponse noAdapterDryRun =
        controller.moveBody(
            "{\"backendId\":\"no-adapter\",\"sourceNumber\":9,\"targetNumber\":2,\"dryRun\":true}");

    assert(noAdapterDryRun.statusCode == 200);
    assert(noAdapterDryRun.body.find("\"success\":true") != std::string::npos);
    assert(noAdapterDryRun.body.find("\"dryRun\":true") != std::string::npos);
    assert(noAdapterDryRun.body.find("\"command\":\"MOVC 9 2\"") != std::string::npos);
    assert(executor->callCount == 1);

    ApiResponse invalidSamePosition =
        controller.moveBody(
            "{\"backendId\":\"default\",\"sourceNumber\":5,\"targetNumber\":5}");

    assert(invalidSamePosition.statusCode == 200);
    assert(invalidSamePosition.body.find("\"success\":false") != std::string::npos);
    assert(invalidSamePosition.body.find("source and target channel number are identical") != std::string::npos);
    assert(executor->callCount == 1);

    ApiResponse readOnly =
        controller.moveBody(
            "{\"backendId\":\"readonly\",\"sourceNumber\":2,\"targetNumber\":1}");

    assert(readOnly.statusCode == 200);
    assert(readOnly.body.find("\"success\":false") != std::string::npos);
    assert(readOnly.body.find("backend is read-only") != std::string::npos);
    assert(executor->callCount == 1);

    ApiResponse missingAdapter =
        controller.moveBody(
            "{\"backendId\":\"no-adapter\",\"sourceNumber\":6,\"targetNumber\":2}");

    assert(missingAdapter.statusCode == 200);
    assert(missingAdapter.body.find("\"success\":false") != std::string::npos);
    assert(missingAdapter.body.find(
        "no channel move executor registered for backend") !=
        std::string::npos);
    assert(executor->callCount == 1);

    ApiResponse unknownBackend =
        controller.moveBody(
            "{\"backendId\":\"unknown\",\"sourceNumber\":6,\"targetNumber\":2}");

    assert(unknownBackend.statusCode == 200);
    assert(unknownBackend.body.find("\"success\":false") != std::string::npos);
    assert(unknownBackend.body.find("backend not found") != std::string::npos);
    assert(unknownBackend.body.find(
        "no channel move executor registered") == std::string::npos);
    assert(executor->callCount == 1);

    return 0;
}

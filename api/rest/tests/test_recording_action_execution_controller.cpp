#include "RecordingActionExecutionController.h"

#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionExecutionService.h"
#include "RecordingActionValidationRequestParser.h"
#include "VdrSnapshotReadService.h"

#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace
{
class TestBackendExecutorAdapter final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    explicit TestBackendExecutorAdapter(
        std::string backendIdValue)
        : backendIdValue_(backendIdValue)
    {
    }

    std::string backendId() const override
    {
        return backendIdValue_;
    }

    std::string backendType() const override
    {
        return "test-backend";
    }

    RecordingActionCapabilitySet capabilities() const override
    {
        RecordingActionCapabilityContract contract;
        return contract.restfulApiDefaultCapabilities();
    }

    RecordingActionExecutionResult execute(
        const RecordingActionJobPayload& payload) override
    {
        RecordingActionExecutionResult result;

        result.success = !payload.dryRun;
        result.type = payload.type;
        result.backendId = payload.backendId;
        result.recordingId = payload.recordingId;

        if (payload.dryRun)
        {
            result.message = "dry-run backend execution skipped";
        }
        else
        {
            result.message = "backend execution completed";
        }

        return result;
    }

private:
    std::string backendIdValue_;
};

class CapturingBackendExecutorAdapter final
    : public IRecordingActionBackendExecutorAdapter
{
public:
    explicit CapturingBackendExecutorAdapter(
        std::string backendIdValue)
        : backendIdValue_(backendIdValue)
    {
    }

    std::string backendId() const override
    {
        return backendIdValue_;
    }

    std::string backendType() const override
    {
        return "test-backend";
    }

    RecordingActionCapabilitySet capabilities() const override
    {
        RecordingActionCapabilityContract contract;
        return contract.restfulApiDefaultCapabilities();
    }

    RecordingActionExecutionResult execute(
        const RecordingActionJobPayload& payload) override
    {
        lastPayload = payload;

        RecordingActionExecutionResult result;

        result.success = true;
        result.type = payload.type;
        result.backendId = payload.backendId;
        result.recordingId = payload.recordingId;
        result.message = "captured backend execution";

        return result;
    }

    RecordingActionJobPayload lastPayload;

private:
    std::string backendIdValue_;
};

class TestSnapshotAccessService final : public ISnapshotAccessService
{
public:
    bool hasSnapshot() const override
    {
        return true;
    }

    const VdrSnapshot* snapshot() const override
    {
        return &snapshot_;
    }

    bool hasSnapshotForBackend(const std::string& backendId) const override
    {
        return snapshot_.backendId == backendId;
    }

    const VdrSnapshot* snapshotForBackend(const std::string& backendId) const override
    {
        if (snapshot_.backendId != backendId)
        {
            return nullptr;
        }

        return &snapshot_;
    }

    std::vector<VdrSnapshot> snapshots() const override
    {
        return {snapshot_};
    }

    VdrSnapshot snapshot_;
};
}

int main()
{
    RecordingActionExecutionService executionService;
    RecordingActionExecutionResultJsonSerializer jsonSerializer;
    RecordingActionBackendExecutorAdapterRegistry registry;

    registry.registerAdapter(
        std::make_shared<TestBackendExecutorAdapter>("living-room"));

    RecordingActionExecutionController controller(
        executionService,
        jsonSerializer,
        registry);

    RecordingActionRequest request;
    request.backendId = "living-room";
    request.recordingId = "recording-1";
    request.type = RecordingActionType::Delete;
    request.dryRun = false;

    const ApiResponse response =
        controller.execute(request);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"success\":true") != std::string::npos);
    assert(response.body.find("\"type\":\"DELETE\"") != std::string::npos);
    assert(response.body.find("\"backendId\":\"living-room\"") != std::string::npos);
    assert(response.body.find("\"recordingId\":\"recording-1\"") != std::string::npos);
    assert(response.body.find("\"message\":\"backend execution completed\"") != std::string::npos);

    RecordingActionExecutionController controllerWithoutParser(
        executionService,
        jsonSerializer,
        registry);

    const ApiResponse parserMissingResponse =
        controllerWithoutParser.executeBody("{}");

    assert(parserMissingResponse.statusCode == 500);
    assert(parserMissingResponse.contentType == "application/json");
    assert(parserMissingResponse.body.find("request parser unavailable") != std::string::npos);

    RecordingActionValidationRequestParser requestParser;

    RecordingActionExecutionController bodyController(
        executionService,
        jsonSerializer,
        registry,
        requestParser);

    const std::string body =
        "{"
        "\"backendId\":\"living-room\","
        "\"recordingId\":\"recording-2\","
        "\"action\":\"MOVE\","
        "\"dryRun\":true,"
        "\"targetPath\":\"/video/archive\""
        "}";

    const ApiResponse bodyResponse =
        bodyController.executeBody(body);

    assert(bodyResponse.statusCode == 200);
    assert(bodyResponse.contentType == "application/json");
    assert(bodyResponse.body.find("\"success\":false") != std::string::npos);
    assert(bodyResponse.body.find("\"type\":\"MOVE\"") != std::string::npos);
    assert(bodyResponse.body.find("\"backendId\":\"living-room\"") != std::string::npos);
    assert(bodyResponse.body.find("\"recordingId\":\"recording-2\"") != std::string::npos);
    assert(bodyResponse.body.find("\"message\":\"dry-run backend execution skipped\"") != std::string::npos);
    assert(bodyResponse.body.find("\"dry-run only\"") != std::string::npos);

    RecordingActionExecutionService resolvedExecutionService;
    RecordingActionExecutionResultJsonSerializer resolvedJsonSerializer;
    RecordingActionBackendExecutorAdapterRegistry resolvedRegistry;
    BackendRegistry backendRegistry;
    RecordingActionValidationRequestParser resolvedRequestParser;
    TestSnapshotAccessService snapshotAccessService;
    VdrSnapshotReadService snapshotReadService(snapshotAccessService);

    BackendNode backendNode;
    backendNode.backendId = "living-room";
    backendNode.backendType = "restfulapi";
    backendNode.enabled = true;
    backendNode.online = true;
    backendRegistry.addBackend(backendNode);

    VdrRecording recording;
    recording.id = "recording-3";
    recording.backendId = "living-room";
    recording.backendNativeId = "/srv/vdr/video/Movies/Test/2026-06-19.20.15.1-0.rec";
    snapshotAccessService.snapshot_.backendId = "living-room";
    snapshotAccessService.snapshot_.recordings.push_back(recording);

    auto capturingAdapter =
        std::make_shared<CapturingBackendExecutorAdapter>("living-room");

    resolvedRegistry.registerAdapter(capturingAdapter);

    RecordingActionExecutionController resolvedBodyController(
        resolvedExecutionService,
        resolvedJsonSerializer,
        resolvedRegistry,
        backendRegistry,
        resolvedRequestParser,
        snapshotReadService);

    int refreshCount = 0;
    resolvedBodyController.setAfterSuccessfulExecutionCallback(
        [&refreshCount]() {
            ++refreshCount;
        });

    const std::string resolvedBody =
        "{"
        "\"backendId\":\"living-room\","
        "\"recordingId\":\"recording-3\","
        "\"action\":\"DELETE\","
        "\"dryRun\":false"
        "}";

    const ApiResponse resolvedBodyResponse =
        resolvedBodyController.executeBody(resolvedBody);

    assert(resolvedBodyResponse.statusCode == 200);
    assert(resolvedBodyResponse.contentType == "application/json");
    assert(resolvedBodyResponse.body.find("\"success\":true") != std::string::npos);
    assert(resolvedBodyResponse.body.find("\"type\":\"DELETE\"") != std::string::npos);
    assert(resolvedBodyResponse.body.find("\"backendNativeId\":\"" + recording.backendNativeId + "\"") != std::string::npos);
    assert(resolvedBodyResponse.body.find("\"snapshotRefreshed\":true") != std::string::npos);
    assert(capturingAdapter->lastPayload.parameters.at("backendNativeId") == recording.backendNativeId);
    assert(refreshCount == 1);

    return 0;
}

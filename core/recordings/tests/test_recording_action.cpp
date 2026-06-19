#include "RecordingActionRequest.h"
#include "RecordingActionUtils.h"
#include "RecordingActionValidationResult.h"
#include "RecordingActionPlan.h"
#include "RecordingActionJobPayload.h"
#include "RecordingActionCapabilityRequirements.h"
#include "RecordingActionCapabilityEvaluationResult.h"
#include "RecordingActionPermissionEvaluationResult.h"
#include "RecordingActionExecutionReadinessResult.h"
#include "RecordingActionExecutionResult.h"
#include "IRecordingActionExecutor.h"
#include "RecordingActionExecutorRegistry.h"
#include "RecordingActionExecutorRegistration.h"
#include "RecordingActionExecutorLookupResult.h"
#include "RecordingActionExecutorSelectionResult.h"
#include "RecordingActionDispatchResult.h"
#include "RecordingActionDefaultExecutorResolutionResult.h"
#include "RecordingActionCapabilityDispatchRule.h"
#include "RecordingActionPermissionDispatchRule.h"
#include "RecordingActionExecutorResolutionService.h"
#include "RecordingActionDispatchService.h"
#include "IRecordingActionBackendExecutorAdapter.h"
#include "MockRecordingActionBackendExecutorAdapter.h"
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionBackendExecutorAdapterLookupResult.h"
#include "RecordingActionBackendExecutorAdapterResolutionService.h"
#include "RecordingActionBackendExecutorAdapterDispatchService.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"
#include "RestfulApiRecordingActionBackendConfig.h"
#include "RestfulApiRecordingActionRequestBuilder.h"

#include <cassert>
#include <iostream>

int main()
{
    auto action = RecordingActionType::Shrink;
    auto text = toString(action);

    assert(text == "SHRINK");

    auto parsed = fromString(text);
    assert(parsed == RecordingActionType::Shrink);

    assert(toString(RecordingActionType::Move) == "MOVE");
    assert(toString(RecordingActionType::Delete) == "DELETE");
    assert(toString(RecordingActionType::MetadataRefresh) == "METADATA_REFRESH");

    assert(fromString("MOVE") == RecordingActionType::Move);
    assert(fromString("DELETE") == RecordingActionType::Delete);
    assert(fromString("METADATA_REFRESH") == RecordingActionType::MetadataRefresh);
    assert(fromString("DOES_NOT_EXIST") == RecordingActionType::Unknown);

    RecordingActionRequest request;
    request.backendId = "default";
    request.recordingId = "recording-001";
    request.type = RecordingActionType::Move;
    request.dryRun = true;
    request.parameters["targetPath"] = "/srv/vdr/video/archive";

    assert(request.backendId == "default");
    assert(request.recordingId == "recording-001");
    assert(request.type == RecordingActionType::Move);
    assert(request.dryRun);
    assert(request.parameters.at("targetPath") == "/srv/vdr/video/archive");

    RecordingActionValidationResult validation;
    validation.valid = true;
    validation.dryRun = true;
    validation.wouldCreateJob = false;
    validation.backendId = request.backendId;
    validation.recordingId = request.recordingId;
    validation.requiredCapabilities.push_back("recordings.action.move");
    validation.requiredPermissions.push_back("recordings.action.move");
    validation.warnings.push_back("dry-run only");

    assert(validation.valid);
    assert(validation.dryRun);
    assert(!validation.wouldCreateJob);
    assert(validation.backendId == "default");
    assert(validation.recordingId == "recording-001");
    assert(validation.requiredCapabilities.size() == 1);
    assert(validation.requiredCapabilities.at(0) == "recordings.action.move");
    assert(validation.requiredPermissions.size() == 1);
    assert(validation.requiredPermissions.at(0) == "recordings.action.move");
    assert(validation.warnings.size() == 1);
    assert(validation.errors.empty());

    RecordingActionValidationResult failedValidation;
    failedValidation.valid = false;
    failedValidation.dryRun = true;
    failedValidation.wouldCreateJob = false;
    failedValidation.backendId = "default";
    failedValidation.recordingId = "";
    failedValidation.errors.push_back("recordingId is required");

    assert(!failedValidation.valid);
    assert(failedValidation.dryRun);
    assert(!failedValidation.wouldCreateJob);
    assert(failedValidation.errors.size() == 1);
    assert(failedValidation.errors.at(0) == "recordingId is required");

    RecordingActionPlan dryRunPlan;
    dryRunPlan.backendId = validation.backendId;
    dryRunPlan.recordingId = validation.recordingId;
    dryRunPlan.type = RecordingActionType::Move;
    dryRunPlan.dryRun = true;
    dryRunPlan.executionAllowed = false;
    dryRunPlan.createJob = false;
    dryRunPlan.plannedJobType = "MOVE";
    dryRunPlan.requiredCapabilities = validation.requiredCapabilities;
    dryRunPlan.requiredPermissions = validation.requiredPermissions;
    dryRunPlan.warnings = validation.warnings;

    assert(dryRunPlan.backendId == "default");
    assert(dryRunPlan.recordingId == "recording-001");
    assert(dryRunPlan.type == RecordingActionType::Move);
    assert(dryRunPlan.dryRun);
    assert(!dryRunPlan.executionAllowed);
    assert(!dryRunPlan.createJob);
    assert(dryRunPlan.plannedJobType == "MOVE");
    assert(dryRunPlan.requiredCapabilities.size() == 1);
    assert(dryRunPlan.requiredCapabilities.at(0) == "recordings.action.move");
    assert(dryRunPlan.requiredPermissions.size() == 1);
    assert(dryRunPlan.requiredPermissions.at(0) == "recordings.action.move");
    assert(dryRunPlan.warnings.size() == 1);

    RecordingActionPlan executionPlan;
    executionPlan.backendId = "default";
    executionPlan.recordingId = "recording-001";
    executionPlan.type = RecordingActionType::MetadataRefresh;
    executionPlan.dryRun = false;
    executionPlan.executionAllowed = true;
    executionPlan.createJob = true;
    executionPlan.plannedJobType = "METADATA_REFRESH";
    executionPlan.requiredCapabilities.push_back("recordings.action.metadata.refresh");
    executionPlan.requiredPermissions.push_back("recordings.action.metadata.refresh");

    assert(executionPlan.backendId == "default");
    assert(executionPlan.recordingId == "recording-001");
    assert(executionPlan.type == RecordingActionType::MetadataRefresh);
    assert(!executionPlan.dryRun);
    assert(executionPlan.executionAllowed);
    assert(executionPlan.createJob);
    assert(executionPlan.plannedJobType == "METADATA_REFRESH");
    assert(executionPlan.requiredCapabilities.size() == 1);
    assert(executionPlan.requiredPermissions.size() == 1);

    RecordingActionJobPayload payload;
    payload.backendId = executionPlan.backendId;
    payload.recordingId = executionPlan.recordingId;
    payload.type = executionPlan.type;
    payload.jobType = executionPlan.plannedJobType;
    payload.dryRun = executionPlan.dryRun;
    payload.parameters["refreshMode"] = "metadata";
    payload.requiredCapabilities = executionPlan.requiredCapabilities;
    payload.requiredPermissions = executionPlan.requiredPermissions;
    payload.warnings = executionPlan.warnings;

    assert(payload.backendId == "default");
    assert(payload.recordingId == "recording-001");
    assert(payload.type == RecordingActionType::MetadataRefresh);
    assert(payload.jobType == "METADATA_REFRESH");
    assert(!payload.dryRun);
    assert(payload.parameters.at("refreshMode") == "metadata");
    assert(payload.requiredCapabilities.size() == 1);
    assert(payload.requiredPermissions.size() == 1);
    assert(payload.warnings.empty());

    RecordingActionCapabilityRequirements moveRequirements;
    moveRequirements.type = RecordingActionType::Move;
    moveRequirements.requiredCapabilities.push_back("recordings.action.move");
    moveRequirements.requiresWriteAccess = true;
    moveRequirements.requiresDryRunSupport = true;

    assert(moveRequirements.type == RecordingActionType::Move);
    assert(moveRequirements.requiredCapabilities.size() == 1);
    assert(moveRequirements.requiredCapabilities.at(0) == "recordings.action.move");
    assert(moveRequirements.requiresWriteAccess);
    assert(moveRequirements.requiresDryRunSupport);

    RecordingActionCapabilityRequirements metadataRequirements;
    metadataRequirements.type = RecordingActionType::MetadataRefresh;
    metadataRequirements.requiredCapabilities.push_back("recordings.action.metadata.refresh");
    metadataRequirements.requiresWriteAccess = false;
    metadataRequirements.requiresDryRunSupport = true;

    assert(metadataRequirements.type == RecordingActionType::MetadataRefresh);
    assert(metadataRequirements.requiredCapabilities.size() == 1);
    assert(metadataRequirements.requiredCapabilities.at(0) == "recordings.action.metadata.refresh");
    assert(!metadataRequirements.requiresWriteAccess);
    assert(metadataRequirements.requiresDryRunSupport);


    RecordingActionCapabilityEvaluationResult evaluation;
    evaluation.type = RecordingActionType::Move;
    evaluation.allowed = true;
    evaluation.availableCapabilities.push_back("recordings.action.move");

    assert(evaluation.type == RecordingActionType::Move);
    assert(evaluation.allowed);
    assert(evaluation.availableCapabilities.size() == 1);
    assert(evaluation.availableCapabilities.at(0) == "recordings.action.move");
    assert(evaluation.missingCapabilities.empty());


    RecordingActionPermissionEvaluationResult permissionEvaluation;
    permissionEvaluation.type = RecordingActionType::Move;
    permissionEvaluation.allowed = true;
    permissionEvaluation.grantedPermissions.push_back("recordings.move");

    assert(permissionEvaluation.type == RecordingActionType::Move);
    assert(permissionEvaluation.allowed);
    assert(permissionEvaluation.grantedPermissions.size() == 1);
    assert(permissionEvaluation.grantedPermissions.at(0) == "recordings.move");
    assert(permissionEvaluation.missingPermissions.empty());


    RecordingActionExecutionReadinessResult readiness;
    readiness.type = RecordingActionType::Move;
    readiness.ready = true;
    readiness.readinessChecksPassed.push_back("capabilities");
    readiness.readinessChecksPassed.push_back("permissions");

    assert(readiness.type == RecordingActionType::Move);
    assert(readiness.ready);
    assert(readiness.readinessChecksPassed.size() == 2);
    assert(readiness.readinessChecksFailed.empty());


    RecordingActionExecutionResult execution;
    execution.type = RecordingActionType::Move;
    execution.success = true;
    execution.backendId = "default";
    execution.recordingId = "recording-001";
    execution.message = "execution accepted";
    execution.warnings.push_back("placeholder boundary only");

    assert(execution.type == RecordingActionType::Move);
    assert(execution.success);
    assert(execution.backendId == "default");
    assert(execution.recordingId == "recording-001");
    assert(execution.message == "execution accepted");
    assert(execution.warnings.size() == 1);
    assert(execution.errors.empty());

    struct TestRecordingActionExecutor final : IRecordingActionExecutor
    {
        RecordingActionExecutionResult execute(
            const RecordingActionJobPayload& payload) override
        {
            RecordingActionExecutionResult result;
            result.type = payload.type;
            result.success = true;
            result.backendId = payload.backendId;
            result.recordingId = payload.recordingId;
            result.message = "test executor accepted payload";
            return result;
        }
    };

    TestRecordingActionExecutor executor;
    auto executorResult = executor.execute(payload);

    assert(executorResult.type == payload.type);
    assert(executorResult.success);
    assert(executorResult.backendId == payload.backendId);
    assert(executorResult.recordingId == payload.recordingId);
    assert(executorResult.message == "test executor accepted payload");


    RecordingActionExecutorRegistry registry;
    auto sharedExecutor = std::make_shared<TestRecordingActionExecutor>();

    RecordingActionExecutorRegistration registration;
    registration.backendId = "default";
    registration.executor = sharedExecutor;

    registry.registerExecutor(registration);

    auto foundExecutor = registry.findExecutor("default");
    auto missingExecutor = registry.findExecutor("missing");

    assert(foundExecutor.found);
    assert(foundExecutor.backendId == "default");
    assert(foundExecutor.executor != nullptr);
    assert(foundExecutor.message == "executor found");

    assert(!missingExecutor.found);
    assert(missingExecutor.backendId == "missing");
    assert(missingExecutor.executor == nullptr);
    assert(missingExecutor.message == "executor not found");

    auto registryResult = foundExecutor.executor->execute(payload);

    assert(registryResult.success);
    assert(registryResult.backendId == payload.backendId);
    assert(registryResult.recordingId == payload.recordingId);


    assert(registration.backendId == "default");
    assert(registration.executor != nullptr);


    RecordingActionExecutorSelectionResult selection;
    selection.selected = foundExecutor.found;
    selection.backendId = foundExecutor.backendId;
    selection.executor = foundExecutor.executor;
    selection.reason = "executor selected from registry lookup result";

    assert(selection.selected);
    assert(selection.backendId == "default");
    assert(selection.executor != nullptr);
    assert(selection.reason == "executor selected from registry lookup result");

    auto selectionResult = selection.executor->execute(payload);

    assert(selectionResult.success);
    assert(selectionResult.backendId == payload.backendId);
    assert(selectionResult.recordingId == payload.recordingId);


    RecordingActionDispatchResult dispatch;
    dispatch.dispatched = selection.selected;
    dispatch.executionResult = selectionResult;
    dispatch.reason = "action dispatched to selected executor";

    assert(dispatch.dispatched);
    assert(dispatch.executionResult.success);
    assert(dispatch.reason == "action dispatched to selected executor");


    RecordingActionDefaultExecutorResolutionResult resolution;

    resolution.resolved = true;
    resolution.usedDefaultExecutor = false;
    resolution.backendId = payload.backendId;
    resolution.executor = selection.executor;
    resolution.reason = "resolved by explicit backend id";

    assert(resolution.resolved);
    assert(!resolution.usedDefaultExecutor);
    assert(resolution.backendId == payload.backendId);
    assert(resolution.executor != nullptr);
    assert(
        resolution.reason ==
        "resolved by explicit backend id");


    RecordingActionCapabilityDispatchRule moveRule;
    moveRule.action = "move";
    moveRule.requiredCapability = "moveRecording";
    moveRule.capabilityRequired = true;

    assert(moveRule.action == "move");
    assert(moveRule.requiredCapability == "moveRecording");
    assert(moveRule.capabilityRequired);

    RecordingActionCapabilityDispatchRule deleteRule;
    deleteRule.action = "delete";
    deleteRule.requiredCapability = "deleteRecording";
    deleteRule.capabilityRequired = true;

    assert(deleteRule.action == "delete");
    assert(deleteRule.requiredCapability == "deleteRecording");
    assert(deleteRule.capabilityRequired);


    RecordingActionPermissionDispatchRule permissionMoveRule;
    permissionMoveRule.action = "move";
    permissionMoveRule.requiredPermission = "moveRecording";
    permissionMoveRule.permissionRequired = true;

    assert(permissionMoveRule.action == "move");
    assert(permissionMoveRule.requiredPermission == "moveRecording");
    assert(permissionMoveRule.permissionRequired);

    RecordingActionPermissionDispatchRule permissionDeleteRule;
    permissionDeleteRule.action = "delete";
    permissionDeleteRule.requiredPermission = "deleteRecording";
    permissionDeleteRule.permissionRequired = true;

    assert(permissionDeleteRule.action == "delete");
    assert(permissionDeleteRule.requiredPermission == "deleteRecording");
    assert(permissionDeleteRule.permissionRequired);


    RecordingActionExecutorResolutionService service;

    auto resolvedSelection =
        service.resolve(
            foundExecutor,
            resolution);

    assert(resolvedSelection.selected);
    assert(resolvedSelection.executor != nullptr);
    assert(resolvedSelection.backendId == "default");
    assert(resolvedSelection.reason == "resolved by lookup");


    RecordingActionDispatchService dispatchService;

    auto dispatchServiceResult =
        dispatchService.dispatch(
            resolvedSelection,
            payload);

    assert(dispatchServiceResult.dispatched);
    assert(dispatchServiceResult.executionResult.success);
    assert(dispatchServiceResult.executionResult.backendId == payload.backendId);
    assert(dispatchServiceResult.executionResult.recordingId == payload.recordingId);
    assert(dispatchServiceResult.reason == "action dispatched");


    struct TestBackendExecutorAdapter final
        : IRecordingActionBackendExecutorAdapter
    {
        std::string backendId() const override
        {
            return "default";
        }

        std::string backendType() const override
        {
            return "mock";
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
            result.type = payload.type;
            result.success = true;
            result.backendId = payload.backendId;
            result.recordingId = payload.recordingId;
            result.message = "backend adapter accepted payload";
            return result;
        }
    };

    TestBackendExecutorAdapter backendAdapter;

    auto backendAdapterResult = backendAdapter.execute(payload);

    assert(backendAdapter.backendId() == "default");
    assert(backendAdapter.backendType() == "mock");
    assert(backendAdapterResult.success);
    assert(backendAdapterResult.backendId == payload.backendId);
    assert(backendAdapterResult.recordingId == payload.recordingId);
    assert(backendAdapterResult.message == "backend adapter accepted payload");


    MockRecordingActionBackendExecutorAdapter mockBackendAdapter;

    auto mockBackendResult = mockBackendAdapter.execute(payload);

    assert(mockBackendAdapter.backendId() == "mock");
    assert(mockBackendAdapter.backendType() == "mock");
    assert(mockBackendResult.success);
    assert(mockBackendResult.backendId == "mock");
    assert(mockBackendResult.recordingId == payload.recordingId);
    assert(mockBackendResult.message == "mock backend executor accepted payload");


    RecordingActionBackendExecutorAdapterRegistry adapterRegistry;
    auto registeredMockAdapter =
        std::make_shared<MockRecordingActionBackendExecutorAdapter>();

    adapterRegistry.registerAdapter(registeredMockAdapter);

    auto foundMockAdapter = adapterRegistry.findAdapter("mock");
    auto missingMockAdapter = adapterRegistry.findAdapter("missing");

    assert(foundMockAdapter.found);
    assert(foundMockAdapter.backendId == "mock");
    assert(foundMockAdapter.adapter != nullptr);
    assert(foundMockAdapter.message == "backend executor adapter found");

    assert(!missingMockAdapter.found);
    assert(missingMockAdapter.backendId == "missing");
    assert(missingMockAdapter.adapter == nullptr);
    assert(missingMockAdapter.message == "backend executor adapter not found");

    assert(foundMockAdapter.adapter->backendId() == "mock");
    assert(foundMockAdapter.adapter->backendType() == "mock");

    auto registryMockResult = foundMockAdapter.adapter->execute(payload);

    assert(registryMockResult.success);
    assert(registryMockResult.backendId == "mock");
    assert(registryMockResult.recordingId == payload.recordingId);


    RecordingActionBackendExecutorAdapterResolutionService adapterResolutionService;

    auto resolvedAdapter =
        adapterResolutionService.resolve(foundMockAdapter);

    auto unresolvedAdapter =
        adapterResolutionService.resolve(missingMockAdapter);

    assert(resolvedAdapter.found);
    assert(resolvedAdapter.backendId == "mock");
    assert(resolvedAdapter.adapter != nullptr);
    assert(resolvedAdapter.message == "backend executor adapter found");

    assert(!unresolvedAdapter.found);
    assert(unresolvedAdapter.backendId == "missing");
    assert(unresolvedAdapter.adapter == nullptr);
    assert(
        unresolvedAdapter.message ==
        "backend executor adapter resolution failed");


    RecordingActionBackendExecutorAdapterDispatchService adapterDispatchService;

    auto adapterDispatchResult =
        adapterDispatchService.dispatch(
            resolvedAdapter,
            payload);

    auto missingAdapterDispatchResult =
        adapterDispatchService.dispatch(
            unresolvedAdapter,
            payload);

    assert(adapterDispatchResult.dispatched);
    assert(adapterDispatchResult.executionResult.success);
    assert(adapterDispatchResult.executionResult.backendId == "mock");
    assert(adapterDispatchResult.executionResult.recordingId == payload.recordingId);
    assert(
        adapterDispatchResult.reason ==
        "action dispatched to backend executor adapter");

    assert(!missingAdapterDispatchResult.dispatched);
    assert(
        missingAdapterDispatchResult.reason ==
        "no backend executor adapter resolved");


    struct TestRestfulApiHttpClient final : IHttpClient
    {
        HttpResponse execute(const HttpRequest& request) const override
        {
            called = true;
            lastRequest = request;

            HttpResponse response;
            response.statusCode = statusCode;
            response.body = responseBody;
            return response;
        }

        mutable bool called = false;
        mutable HttpRequest lastRequest;
        int statusCode = 200;
        std::string responseBody = "{}";
    };

    TestRestfulApiHttpClient restfulApiHttpClient;

    RestfulApiRecordingActionBackendConfig restfulApiConfig;
    restfulApiConfig.backendId = "restfulapi-default";
    restfulApiConfig.host = "127.0.0.1";
    restfulApiConfig.port = 8002;
    restfulApiConfig.basePath = "";

    RestfulApiRecordingActionBackendExecutorAdapter restfulApiAdapter(
        restfulApiConfig,
        restfulApiHttpClient);

    auto restfulApiUnsupportedResult = restfulApiAdapter.execute(payload);

    assert(restfulApiAdapter.backendId() == "restfulapi-default");
    assert(restfulApiAdapter.backendType() == "restfulapi");
    assert(restfulApiAdapter.config().backendId == "restfulapi-default");
    assert(restfulApiAdapter.config().host == "127.0.0.1");
    assert(restfulApiAdapter.config().port == 8002);
    assert(restfulApiAdapter.config().basePath == "");
    assert(!restfulApiAdapter.config().allowExecution);
    assert(!restfulApiAdapter.config().readOnly);
    assert(!restfulApiUnsupportedResult.success);
    assert(!restfulApiHttpClient.called);
    assert(restfulApiUnsupportedResult.backendId == "restfulapi-default");
    assert(restfulApiUnsupportedResult.recordingId == payload.recordingId);
    assert(
        restfulApiUnsupportedResult.message ==
        "restfulapi backend executor action not supported");
    assert(restfulApiUnsupportedResult.type == payload.type);
    assert(restfulApiUnsupportedResult.errors.size() == 1);
    assert(
        restfulApiUnsupportedResult.errors.at(0) ==
        "unsupported recording action type for restfulapi backend executor");

    RecordingActionJobPayload movePayload;
    movePayload.backendId = "restfulapi-default";
    movePayload.recordingId = "recording-001";
    movePayload.type = RecordingActionType::Move;
    movePayload.jobType = "MOVE";
    movePayload.dryRun = true;
    movePayload.parameters["targetPath"] = "/srv/vdr/video/archive";

    RestfulApiRecordingActionRequestBuilder requestBuilder;

    auto moveRequest =
        requestBuilder.buildMoveRequest(
            restfulApiConfig,
            movePayload);

    assert(moveRequest.method == "POST");
    assert(moveRequest.url == "/recordings/move.json");
    assert(moveRequest.headers.at("Accept") == "application/json");
    assert(moveRequest.headers.at("Content-Type") == "application/json");
    assert(
        moveRequest.body ==
        "{\"source\":\"recording-001\",\"target\":\"~srv~vdr~video~archive\",\"copy_only\":false}");

    restfulApiConfig.basePath = "/api";

    auto prefixedMoveRequest =
        requestBuilder.buildMoveRequest(
            restfulApiConfig,
            movePayload);

    assert(prefixedMoveRequest.url == "/api/recordings/move.json");

    restfulApiConfig.basePath = "";

    RecordingActionJobPayload renamePayload;
    renamePayload.backendId = "restfulapi-default";
    renamePayload.recordingId = "recording-001";
    renamePayload.type = RecordingActionType::Rename;
    renamePayload.jobType = "RENAME";
    renamePayload.dryRun = true;
    renamePayload.parameters["newName"] = "Evening News";

    auto renameRequest =
        requestBuilder.buildRenameRequest(
            restfulApiConfig,
            renamePayload);

    assert(renameRequest.method == "POST");
    assert(renameRequest.url == "/recordings/move.json");
    assert(renameRequest.headers.at("Accept") == "application/json");
    assert(renameRequest.headers.at("Content-Type") == "application/json");
    assert(
        renameRequest.body ==
        "{\"source\":\"recording-001\",\"target\":\"Evening_News\",\"copy_only\":false}");

    restfulApiConfig.basePath = "/api";

    auto prefixedRenameRequest =
        requestBuilder.buildRenameRequest(
            restfulApiConfig,
            renamePayload);

    assert(prefixedRenameRequest.url == "/api/recordings/move.json");

    restfulApiConfig.basePath = "";

    RecordingActionJobPayload deletePayload;
    deletePayload.backendId = "restfulapi-default";
    deletePayload.recordingId = "recording-001";
    deletePayload.type = RecordingActionType::Delete;
    deletePayload.jobType = "DELETE";
    deletePayload.dryRun = true;

    auto deleteRequest =
        requestBuilder.buildDeleteRequest(
            restfulApiConfig,
            deletePayload);

    assert(deleteRequest.method == "POST");
    assert(deleteRequest.url == "/recordings/delete.json");
    assert(deleteRequest.headers.at("Accept") == "application/json");
    assert(deleteRequest.headers.at("Content-Type") == "application/json");
    assert(
        deleteRequest.body ==
        "{\"file\":\"recording-001\"}");

    restfulApiConfig.basePath = "/api";

    auto prefixedDeleteRequest =
        requestBuilder.buildDeleteRequest(
            restfulApiConfig,
            deletePayload);

    assert(prefixedDeleteRequest.url == "/api/recordings/delete.json");

    restfulApiConfig.basePath = "";

    RecordingActionJobPayload pathMovePayload = movePayload;
    pathMovePayload.parameters["recordingPath"] =
        "Mystery/The_Village_-_Das_Dorf/2026-06-15.20.15.1-0.rec";

    auto pathMoveRequest =
        requestBuilder.buildMoveRequest(
            restfulApiConfig,
            pathMovePayload);

    assert(
        pathMoveRequest.body ==
        "{\"source\":\"Mystery/The_Village_-_Das_Dorf/2026-06-15.20.15.1-0.rec\",\"target\":\"~srv~vdr~video~archive\",\"copy_only\":false}");

    RecordingActionJobPayload pathRenamePayload = renamePayload;
    pathRenamePayload.parameters["recordingPath"] =
        "Mystery/The_Village_-_Das_Dorf/2026-06-15.20.15.1-0.rec";

    auto pathRenameRequest =
        requestBuilder.buildRenameRequest(
            restfulApiConfig,
            pathRenamePayload);

    assert(
        pathRenameRequest.body ==
        "{\"source\":\"Mystery/The_Village_-_Das_Dorf/2026-06-15.20.15.1-0.rec\",\"target\":\"Evening News\",\"copy_only\":false}");

    RecordingActionJobPayload folderMovePayload = movePayload;
    folderMovePayload.parameters["targetPath"] = "Oskar/Tagesschau";

    auto folderMoveRequest =
        requestBuilder.buildMoveRequest(
            restfulApiConfig,
            folderMovePayload);

    assert(
        folderMoveRequest.body ==
        "{\"source\":\"recording-001\",\"target\":\"Oskar~Tagesschau\",\"copy_only\":false}");

    RecordingActionJobPayload folderRenamePayload = renamePayload;
    folderRenamePayload.parameters["newName"] = "Oskar/Tagesschau";

    auto folderRenameRequest =
        requestBuilder.buildRenameRequest(
            restfulApiConfig,
            folderRenamePayload);

    assert(
        folderRenameRequest.body ==
        "{\"source\":\"recording-001\",\"target\":\"Oskar~Tagesschau\",\"copy_only\":false}");

    RecordingActionJobPayload pathDeletePayload = deletePayload;
    pathDeletePayload.parameters["recordingPath"] =
        "Mystery/The_Village_-_Das_Dorf/2026-06-15.20.15.1-0.rec";

    auto pathDeleteRequest =
        requestBuilder.buildDeleteRequest(
            restfulApiConfig,
            pathDeletePayload);

    assert(
        pathDeleteRequest.body ==
        "{\"file\":\"Mystery/The_Village_-_Das_Dorf/2026-06-15.20.15.1-0.rec\"}");

    TestRestfulApiHttpClient moveExecutionHttpClient;

    RestfulApiRecordingActionBackendExecutorAdapter moveExecutionAdapter(
        restfulApiConfig,
        moveExecutionHttpClient);

    auto moveExecutionResult =
        moveExecutionAdapter.execute(movePayload);

    assert(moveExecutionHttpClient.called);
    assert(moveExecutionHttpClient.lastRequest.method == "POST");
    assert(moveExecutionHttpClient.lastRequest.url == "/recordings/move.json");
    assert(moveExecutionResult.success);
    assert(moveExecutionResult.backendId == "restfulapi-default");
    assert(moveExecutionResult.recordingId == movePayload.recordingId);
    assert(
        moveExecutionResult.message ==
        "restfulapi backend executor request accepted");
    assert(moveExecutionResult.errors.empty());

    TestRestfulApiHttpClient renameExecutionHttpClient;

    RestfulApiRecordingActionBackendExecutorAdapter renameExecutionAdapter(
        restfulApiConfig,
        renameExecutionHttpClient);

    auto renameExecutionResult =
        renameExecutionAdapter.execute(renamePayload);

    assert(renameExecutionHttpClient.called);
    assert(renameExecutionHttpClient.lastRequest.url == "/recordings/move.json");
    assert(renameExecutionResult.success);

    TestRestfulApiHttpClient deleteExecutionHttpClient;

    RestfulApiRecordingActionBackendExecutorAdapter deleteExecutionAdapter(
        restfulApiConfig,
        deleteExecutionHttpClient);

    auto deleteExecutionResult =
        deleteExecutionAdapter.execute(deletePayload);

    assert(deleteExecutionHttpClient.called);
    assert(deleteExecutionHttpClient.lastRequest.url == "/recordings/delete.json");
    assert(deleteExecutionResult.success);

    TestRestfulApiHttpClient failureHttpClient;
    failureHttpClient.statusCode = 500;
    failureHttpClient.responseBody = "{\"error\":\"backend failure\"}";

    RestfulApiRecordingActionBackendExecutorAdapter failureAdapter(
        restfulApiConfig,
        failureHttpClient);

    auto failureResult =
        failureAdapter.execute(movePayload);

    assert(failureHttpClient.called);
    assert(!failureResult.success);
    assert(failureResult.backendId == "restfulapi-default");
    assert(failureResult.recordingId == movePayload.recordingId);
    assert(
        failureResult.message ==
        "restfulapi backend executor request failed");
    assert(failureResult.errors.size() == 2);
    assert(
        failureResult.errors.at(0) ==
        "restfulapi backend returned HTTP status 500");
    assert(
        failureResult.errors.at(1) ==
        "{\"error\":\"backend failure\"}");

    TestRestfulApiHttpClient emptyFailureHttpClient;
    emptyFailureHttpClient.statusCode = 404;
    emptyFailureHttpClient.responseBody = "";

    RestfulApiRecordingActionBackendExecutorAdapter emptyFailureAdapter(
        restfulApiConfig,
        emptyFailureHttpClient);

    auto emptyFailureResult =
        emptyFailureAdapter.execute(deletePayload);

    assert(emptyFailureHttpClient.called);
    assert(!emptyFailureResult.success);
    assert(emptyFailureResult.errors.size() == 1);
    assert(
        emptyFailureResult.errors.at(0) ==
        "restfulapi backend returned HTTP status 404");

    TestRestfulApiHttpClient unsupportedHttpClient;

    RestfulApiRecordingActionBackendExecutorAdapter unsupportedAdapter(
        restfulApiConfig,
        unsupportedHttpClient);

    RecordingActionJobPayload unsupportedPayload;
    unsupportedPayload.backendId = "restfulapi-default";
    unsupportedPayload.recordingId = "recording-unsupported";
    unsupportedPayload.type = RecordingActionType::MetadataRefresh;
    unsupportedPayload.jobType = "METADATA_REFRESH";
    unsupportedPayload.dryRun = true;

    auto unsupportedResult =
        unsupportedAdapter.execute(unsupportedPayload);

    assert(!unsupportedHttpClient.called);
    assert(!unsupportedResult.success);
    assert(unsupportedResult.type == RecordingActionType::MetadataRefresh);
    assert(unsupportedResult.backendId == "restfulapi-default");
    assert(unsupportedResult.recordingId == "recording-unsupported");
    assert(
        unsupportedResult.message ==
        "restfulapi backend executor action not supported");
    assert(unsupportedResult.errors.size() == 1);
    assert(
        unsupportedResult.errors.at(0) ==
        "unsupported recording action type for restfulapi backend executor");

    TestRestfulApiHttpClient missingRecordingIdHttpClient;

    RestfulApiRecordingActionBackendExecutorAdapter missingRecordingIdAdapter(
        restfulApiConfig,
        missingRecordingIdHttpClient);

    RecordingActionJobPayload missingRecordingIdPayload = deletePayload;
    missingRecordingIdPayload.recordingId = "";

    auto missingRecordingIdResult =
        missingRecordingIdAdapter.execute(missingRecordingIdPayload);

    assert(!missingRecordingIdHttpClient.called);
    assert(!missingRecordingIdResult.success);
    assert(
        missingRecordingIdResult.message ==
        "restfulapi backend executor payload invalid");
    assert(missingRecordingIdResult.errors.size() == 1);
    assert(missingRecordingIdResult.errors.at(0) == "recordingId is required");

    TestRestfulApiHttpClient missingMoveTargetHttpClient;

    RestfulApiRecordingActionBackendExecutorAdapter missingMoveTargetAdapter(
        restfulApiConfig,
        missingMoveTargetHttpClient);

    RecordingActionJobPayload missingMoveTargetPayload = movePayload;
    missingMoveTargetPayload.parameters.erase("targetPath");

    auto missingMoveTargetResult =
        missingMoveTargetAdapter.execute(missingMoveTargetPayload);

    assert(!missingMoveTargetHttpClient.called);
    assert(!missingMoveTargetResult.success);
    assert(
        missingMoveTargetResult.message ==
        "restfulapi backend executor payload invalid");
    assert(missingMoveTargetResult.errors.size() == 1);
    assert(
        missingMoveTargetResult.errors.at(0) ==
        "targetPath is required for move");

    TestRestfulApiHttpClient missingRenameNameHttpClient;

    RestfulApiRecordingActionBackendExecutorAdapter missingRenameNameAdapter(
        restfulApiConfig,
        missingRenameNameHttpClient);

    RecordingActionJobPayload missingRenameNamePayload = renamePayload;
    missingRenameNamePayload.parameters.erase("newName");

    auto missingRenameNameResult =
        missingRenameNameAdapter.execute(missingRenameNamePayload);

    assert(!missingRenameNameHttpClient.called);
    assert(!missingRenameNameResult.success);
    assert(
        missingRenameNameResult.message ==
        "restfulapi backend executor payload invalid");
    assert(missingRenameNameResult.errors.size() == 1);
    assert(
        missingRenameNameResult.errors.at(0) ==
        "newName is required for rename");

    TestRestfulApiHttpClient realMoveHttpClient;

    RestfulApiRecordingActionBackendExecutorAdapter realMoveAdapter(
        restfulApiConfig,
        realMoveHttpClient);

    RecordingActionJobPayload realMovePayload = movePayload;
    realMovePayload.dryRun = false;

    auto realMoveResult =
        realMoveAdapter.execute(realMovePayload);

    assert(!realMoveHttpClient.called);
    assert(!realMoveResult.success);
    assert(realMoveResult.backendId == "restfulapi-default");
    assert(realMoveResult.recordingId == realMovePayload.recordingId);
    assert(
        realMoveResult.message ==
        "restfulapi backend executor execution disabled");
    assert(realMoveResult.errors.size() == 1);
    assert(
        realMoveResult.errors.at(0) ==
        "real recording action execution is disabled by restfulapi backend config");

    TestRestfulApiHttpClient realRenameHttpClient;

    RestfulApiRecordingActionBackendExecutorAdapter realRenameAdapter(
        restfulApiConfig,
        realRenameHttpClient);

    RecordingActionJobPayload realRenamePayload = renamePayload;
    realRenamePayload.dryRun = false;

    auto realRenameResult =
        realRenameAdapter.execute(realRenamePayload);

    assert(!realRenameHttpClient.called);
    assert(!realRenameResult.success);
    assert(
        realRenameResult.message ==
        "restfulapi backend executor execution disabled");

    TestRestfulApiHttpClient realDeleteHttpClient;

    RestfulApiRecordingActionBackendExecutorAdapter realDeleteAdapter(
        restfulApiConfig,
        realDeleteHttpClient);

    RecordingActionJobPayload realDeletePayload = deletePayload;
    realDeletePayload.dryRun = false;

    auto realDeleteResult =
        realDeleteAdapter.execute(realDeletePayload);

    assert(!realDeleteHttpClient.called);
    assert(!realDeleteResult.success);
    assert(
        realDeleteResult.message ==
        "restfulapi backend executor execution disabled");

    TestRestfulApiHttpClient enabledExecutionHttpClient;

    RestfulApiRecordingActionBackendConfig enabledExecutionConfig =
        restfulApiConfig;
    enabledExecutionConfig.allowExecution = true;

    RestfulApiRecordingActionBackendExecutorAdapter enabledExecutionAdapter(
        enabledExecutionConfig,
        enabledExecutionHttpClient);

    RecordingActionJobPayload enabledExecutionPayload = movePayload;
    enabledExecutionPayload.dryRun = false;

    auto enabledExecutionResult =
        enabledExecutionAdapter.execute(enabledExecutionPayload);

    assert(enabledExecutionHttpClient.called);
    assert(enabledExecutionHttpClient.lastRequest.method == "POST");
    assert(enabledExecutionHttpClient.lastRequest.url == "/recordings/move.json");
    assert(enabledExecutionHttpClient.lastRequest.body ==
        "{\"source\":\"recording-001\",\"target\":\"~srv~vdr~video~archive\",\"copy_only\":false}");
    assert(enabledExecutionResult.success);
    assert(
        enabledExecutionResult.message ==
        "restfulapi backend executor request accepted");

    TestRestfulApiHttpClient readOnlyHttpClient;

    RestfulApiRecordingActionBackendConfig readOnlyConfig =
        enabledExecutionConfig;
    readOnlyConfig.readOnly = true;

    RestfulApiRecordingActionBackendExecutorAdapter readOnlyAdapter(
        readOnlyConfig,
        readOnlyHttpClient);

    RecordingActionJobPayload readOnlyPayload = movePayload;
    readOnlyPayload.dryRun = false;

    auto readOnlyResult =
        readOnlyAdapter.execute(readOnlyPayload);

    assert(!readOnlyHttpClient.called);
    assert(!readOnlyResult.success);
    assert(readOnlyResult.backendId == "restfulapi-default");
    assert(readOnlyResult.recordingId == readOnlyPayload.recordingId);
    assert(
        readOnlyResult.message ==
        "restfulapi backend executor backend is read-only");
    assert(readOnlyResult.errors.size() == 1);
    assert(
        readOnlyResult.errors.at(0) ==
        "recording action execution is blocked by read-only backend config");

    TestRestfulApiHttpClient readOnlyDryRunHttpClient;

    RestfulApiRecordingActionBackendExecutorAdapter readOnlyDryRunAdapter(
        readOnlyConfig,
        readOnlyDryRunHttpClient);

    RecordingActionJobPayload readOnlyDryRunPayload = renamePayload;
    readOnlyDryRunPayload.dryRun = true;

    auto readOnlyDryRunResult =
        readOnlyDryRunAdapter.execute(readOnlyDryRunPayload);

    assert(!readOnlyDryRunHttpClient.called);
    assert(!readOnlyDryRunResult.success);
    assert(
        readOnlyDryRunResult.message ==
        "restfulapi backend executor backend is read-only");

    std::cout
        << "Recording action RestfulAPI VDR folder target encoding OK"
        << std::endl;




















    return 0;
}

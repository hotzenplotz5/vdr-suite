#include "RecordingActionRequest.h"
#include "RecordingActionUtils.h"
#include "RecordingActionValidationResult.h"
#include "RecordingActionPlan.h"
#include "RecordingActionJobPayload.h"
#include "RecordingActionCapabilityRequirements.h"
#include "RecordingActionCapabilityEvaluationResult.h"
#include "RecordingActionPermissionEvaluationResult.h"
#include "RecordingActionExecutionReadinessResult.h"

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

    std::cout << "Recording action execution readiness model OK" << std::endl;




    return 0;
}

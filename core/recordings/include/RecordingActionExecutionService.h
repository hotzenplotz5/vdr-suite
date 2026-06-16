#pragma once

#include "IRecordingActionExecutor.h"
#include "RecordingActionBackendExecutorAdapterDispatchService.h"
#include "RecordingActionBackendExecutorAdapterRegistry.h"
#include "RecordingActionBackendPolicy.h"
#include "RecordingActionExecutionResult.h"
#include "RecordingActionJobPayloadFactory.h"
#include "RecordingActionRequest.h"
#include "RecordingActionSafetyService.h"
#include "RecordingActionValidationService.h"

class RecordingActionExecutionService
{
public:
    RecordingActionSafetyResult evaluateSafety(
        RecordingActionType action,
        const RecordingActionSafetyContext& context) const
    {
        return safetyService_.evaluate(action, context);
    }

    RecordingActionSafetyResult evaluateSafety(
        RecordingActionType action,
        const RecordingActionSafetyContext& context,
        const RecordingActionCapabilitySet& capabilitySet) const
    {
        return safetyService_.evaluateWithCapabilities(
            action,
            context,
            capabilitySet);
    }

    RecordingActionSafetyResult evaluateSafety(
        const RecordingActionRequest& request,
        const RecordingActionSafetyContext& context,
        const RecordingActionBackendExecutorAdapterRegistry& registry) const
    {
        return evaluateSafety(
            request.type,
            context,
            registry.capabilitiesForBackend(request.backendId));
    }

    RecordingActionSafetyResult evaluateSafety(
        const RecordingActionRequest& request,
        const RecordingActionBackendPolicy& policy) const
    {
        RecordingActionSafetyContext context;
        context.dryRun = request.dryRun;
        context.backendAvailable = policy.backendAvailable;
        context.backendReadOnly = policy.readOnly;
        context.executionAllowed = policy.executionAllowed;

        return safetyService_.evaluateWithCapabilitiesAndPermissions(
            request.type,
            context,
            policy.capabilities,
            policy.permissions);
    }

    RecordingActionExecutionResult execute(
        const RecordingActionRequest& request,
        IRecordingActionExecutor& executor) const
    {
        const RecordingActionValidationResult validation =
            validationService_.validate(request);

        if (!validation.valid)
        {
            return validationFailure(request, validation);
        }

        const RecordingActionJobPayload payload =
            payloadFactory_.create(request, validation);

        RecordingActionExecutionResult result =
            executor.execute(payload);

        appendValidationWarnings(result, validation);

        return result;
    }

    RecordingActionExecutionResult execute(
        const RecordingActionRequest& request,
        const RecordingActionBackendExecutorAdapterRegistry& registry) const
    {
        const RecordingActionValidationResult validation =
            validationService_.validate(request);

        if (!validation.valid)
        {
            return validationFailure(request, validation);
        }

        const RecordingActionJobPayload payload =
            payloadFactory_.create(request, validation);

        const RecordingActionBackendExecutorAdapterLookupResult resolvedAdapter =
            registry.findAdapter(payload.backendId);

        if (!resolvedAdapter.found || !resolvedAdapter.adapter)
        {
            RecordingActionExecutionResult result =
                RecordingActionExecutionResult::failed(
                    payload.type,
                    payload.recordingId,
                    payload.backendId,
                    resolvedAdapter.message,
                    {resolvedAdapter.message}
                );

            appendValidationWarnings(result, validation);

            return result;
        }

        const RecordingActionDispatchResult dispatchResult =
            backendDispatchService_.dispatch(resolvedAdapter, payload);

        if (!dispatchResult.dispatched)
        {
            RecordingActionExecutionResult result =
                RecordingActionExecutionResult::failed(
                    payload.type,
                    payload.recordingId,
                    payload.backendId,
                    dispatchResult.reason,
                    {dispatchResult.reason}
                );

            appendValidationWarnings(result, validation);

            return result;
        }

        RecordingActionExecutionResult result =
            dispatchResult.executionResult;

        appendValidationWarnings(result, validation);

        return result;
    }

private:
    static RecordingActionExecutionResult validationFailure(
        const RecordingActionRequest& request,
        const RecordingActionValidationResult& validation)
    {
        return RecordingActionExecutionResult::failed(
            request.type,
            request.recordingId,
            request.backendId,
            "recording action validation failed",
            validation.errors
        );
    }

    static void appendValidationWarnings(
        RecordingActionExecutionResult& result,
        const RecordingActionValidationResult& validation)
    {
        if (!validation.warnings.empty())
        {
            result.warnings.insert(
                result.warnings.end(),
                validation.warnings.begin(),
                validation.warnings.end()
            );
        }
    }

    RecordingActionSafetyService safetyService_;
    RecordingActionValidationService validationService_;
    RecordingActionJobPayloadFactory payloadFactory_;
    RecordingActionBackendExecutorAdapterDispatchService backendDispatchService_;
};

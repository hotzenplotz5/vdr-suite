#pragma once

#include "SearchTimerCreateRequest.h"
#include "SearchTimerDeleteRequest.h"
#include "SearchTimerUpdateRequest.h"
#include "SearchTimerWorkflowExecutionPlan.h"

class SearchTimerWorkflowCommandRequestMapper
{
public:
    bool canBuildCreateRequest(
        const SearchTimerWorkflowExecutionPlan& plan) const
    {
        return plan.valid() &&
            plan.primaryStep() ==
                SearchTimerWorkflowExecutionStep::Create;
    }

    bool canBuildUpdateRequest(
        const SearchTimerWorkflowExecutionPlan& plan) const
    {
        return plan.valid() &&
            plan.primaryStep() ==
                SearchTimerWorkflowExecutionStep::Update;
    }

    bool canBuildDeleteRequest(
        const SearchTimerWorkflowExecutionPlan& plan) const
    {
        return plan.valid() &&
            plan.primaryStep() ==
                SearchTimerWorkflowExecutionStep::Delete;
    }

    SearchTimerCreateRequest buildCreateRequest(
        const SearchTimerWorkflowExecutionPlan& plan) const
    {
        SearchTimerCreateRequest request;
        request.backendId = plan.backendId();
        request.name = plan.name();
        request.query = plan.query();
        request.active = plan.active();
        return request;
    }

    SearchTimerUpdateRequest buildUpdateRequest(
        const SearchTimerWorkflowExecutionPlan& plan) const
    {
        SearchTimerUpdateRequest request;
        request.backendId = plan.backendId();
        request.backendNativeId = plan.backendNativeId();
        request.name = plan.name();
        request.query = plan.query();
        request.active = plan.active();
        return request;
    }

    SearchTimerDeleteRequest buildDeleteRequest(
        const SearchTimerWorkflowExecutionPlan& plan) const
    {
        SearchTimerDeleteRequest request;
        request.backendId = plan.backendId();
        request.backendNativeId = plan.backendNativeId();
        return request;
    }
};

#pragma once

#include "SearchTimerWorkflowRequest.h"

#include <string>

enum class SearchTimerWorkflowExecutionStep
{
    None,
    List,
    Preview,
    Create,
    Readback,
    Update,
    Delete
};

class SearchTimerWorkflowExecutionPlan
{
public:
    static SearchTimerWorkflowExecutionPlan none()
    {
        return SearchTimerWorkflowExecutionPlan();
    }

    static SearchTimerWorkflowExecutionPlan fromRequest(
        const SearchTimerWorkflowRequest& request)
    {
        SearchTimerWorkflowExecutionPlan plan;

        plan.operation_ = request.operation();
        plan.backendId_ = request.backendId();
        plan.backendNativeId_ = request.backendNativeId();
        plan.name_ = request.name();
        plan.query_ = request.query();
        plan.active_ = request.active();
        plan.valid_ = request.isValid();
        plan.readOnly_ = request.isReadOnly();
        plan.writeOperation_ = request.isWriteOperation();

        if (!plan.valid_)
        {
            return plan;
        }

        if (request.operation() == SearchTimerWorkflowOperation::List)
        {
            plan.primaryStep_ = SearchTimerWorkflowExecutionStep::List;
        }
        else if (request.operation() == SearchTimerWorkflowOperation::Preview)
        {
            plan.primaryStep_ = SearchTimerWorkflowExecutionStep::Preview;
        }
        else if (request.operation() == SearchTimerWorkflowOperation::Create)
        {
            plan.primaryStep_ = SearchTimerWorkflowExecutionStep::Create;
            plan.followUpStep_ = SearchTimerWorkflowExecutionStep::Readback;
        }
        else if (request.operation() == SearchTimerWorkflowOperation::Readback)
        {
            plan.primaryStep_ = SearchTimerWorkflowExecutionStep::Readback;
        }
        else if (request.operation() == SearchTimerWorkflowOperation::Update)
        {
            plan.primaryStep_ = SearchTimerWorkflowExecutionStep::Update;
            plan.followUpStep_ = SearchTimerWorkflowExecutionStep::Readback;
        }
        else if (request.operation() == SearchTimerWorkflowOperation::Delete)
        {
            plan.primaryStep_ = SearchTimerWorkflowExecutionStep::Delete;
        }

        return plan;
    }

    bool valid() const
    {
        return valid_;
    }

    bool hasExecutionWork() const
    {
        return valid_ &&
            primaryStep_ != SearchTimerWorkflowExecutionStep::None;
    }

    SearchTimerWorkflowOperation operation() const
    {
        return operation_;
    }

    SearchTimerWorkflowExecutionStep primaryStep() const
    {
        return primaryStep_;
    }

    SearchTimerWorkflowExecutionStep followUpStep() const
    {
        return followUpStep_;
    }

    bool hasFollowUpStep() const
    {
        return followUpStep_ != SearchTimerWorkflowExecutionStep::None;
    }

    bool readOnly() const
    {
        return readOnly_;
    }

    bool writeOperation() const
    {
        return writeOperation_;
    }

    bool requiresExplicitOperatorConfirmation() const
    {
        return writeOperation_;
    }

    bool requiresBackendReadback() const
    {
        return followUpStep_ == SearchTimerWorkflowExecutionStep::Readback;
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    const std::string& backendNativeId() const
    {
        return backendNativeId_;
    }

    const std::string& name() const
    {
        return name_;
    }

    const std::string& query() const
    {
        return query_;
    }

    bool active() const
    {
        return active_;
    }

private:
    bool valid_ = false;
    bool readOnly_ = true;
    bool writeOperation_ = false;
    bool active_ = true;
    SearchTimerWorkflowOperation operation_ =
        SearchTimerWorkflowOperation::Unknown;
    SearchTimerWorkflowExecutionStep primaryStep_ =
        SearchTimerWorkflowExecutionStep::None;
    SearchTimerWorkflowExecutionStep followUpStep_ =
        SearchTimerWorkflowExecutionStep::None;
    std::string backendId_;
    std::string backendNativeId_;
    std::string name_;
    std::string query_;
};

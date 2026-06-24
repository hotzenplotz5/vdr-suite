#pragma once

#include <string>

enum class SearchTimerWorkflowOperation
{
    Unknown,
    List,
    Preview,
    Create,
    Readback,
    Update,
    Delete
};

class SearchTimerWorkflowRequest
{
public:
    static SearchTimerWorkflowRequest list(
        const std::string& backendId = "")
    {
        SearchTimerWorkflowRequest request;
        request.operation_ = SearchTimerWorkflowOperation::List;
        request.backendId_ = backendId;
        return request;
    }

    static SearchTimerWorkflowRequest preview(
        const std::string& backendId,
        const std::string& name,
        const std::string& query)
    {
        SearchTimerWorkflowRequest request;
        request.operation_ = SearchTimerWorkflowOperation::Preview;
        request.backendId_ = backendId;
        request.name_ = name;
        request.query_ = query;
        return request;
    }

    static SearchTimerWorkflowRequest create(
        const std::string& backendId,
        const std::string& name,
        const std::string& query,
        bool active = true)
    {
        SearchTimerWorkflowRequest request;
        request.operation_ = SearchTimerWorkflowOperation::Create;
        request.backendId_ = backendId;
        request.name_ = name;
        request.query_ = query;
        request.active_ = active;
        return request;
    }

    static SearchTimerWorkflowRequest readback(
        const std::string& backendId,
        const std::string& backendNativeId)
    {
        SearchTimerWorkflowRequest request;
        request.operation_ = SearchTimerWorkflowOperation::Readback;
        request.backendId_ = backendId;
        request.backendNativeId_ = backendNativeId;
        return request;
    }

    static SearchTimerWorkflowRequest update(
        const std::string& backendId,
        const std::string& backendNativeId,
        const std::string& name,
        const std::string& query,
        bool active = true)
    {
        SearchTimerWorkflowRequest request;
        request.operation_ = SearchTimerWorkflowOperation::Update;
        request.backendId_ = backendId;
        request.backendNativeId_ = backendNativeId;
        request.name_ = name;
        request.query_ = query;
        request.active_ = active;
        return request;
    }

    static SearchTimerWorkflowRequest remove(
        const std::string& backendId,
        const std::string& backendNativeId)
    {
        SearchTimerWorkflowRequest request;
        request.operation_ = SearchTimerWorkflowOperation::Delete;
        request.backendId_ = backendId;
        request.backendNativeId_ = backendNativeId;
        return request;
    }

    SearchTimerWorkflowOperation operation() const
    {
        return operation_;
    }

    bool hasOperation() const
    {
        return operation_ != SearchTimerWorkflowOperation::Unknown;
    }

    bool hasBackend() const
    {
        return !backendId_.empty();
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    bool hasBackendNativeId() const
    {
        return !backendNativeId_.empty();
    }

    const std::string& backendNativeId() const
    {
        return backendNativeId_;
    }

    bool hasName() const
    {
        return !name_.empty();
    }

    const std::string& name() const
    {
        return name_;
    }

    bool hasQuery() const
    {
        return !query_.empty();
    }

    const std::string& query() const
    {
        return query_;
    }

    bool active() const
    {
        return active_;
    }

    bool isReadOnly() const
    {
        return operation_ == SearchTimerWorkflowOperation::List ||
            operation_ == SearchTimerWorkflowOperation::Preview ||
            operation_ == SearchTimerWorkflowOperation::Readback;
    }

    bool isWriteOperation() const
    {
        return operation_ == SearchTimerWorkflowOperation::Create ||
            operation_ == SearchTimerWorkflowOperation::Update ||
            operation_ == SearchTimerWorkflowOperation::Delete;
    }

    bool requiresBackend() const
    {
        return operation_ == SearchTimerWorkflowOperation::Preview ||
            operation_ == SearchTimerWorkflowOperation::Create ||
            operation_ == SearchTimerWorkflowOperation::Readback ||
            operation_ == SearchTimerWorkflowOperation::Update ||
            operation_ == SearchTimerWorkflowOperation::Delete;
    }

    bool requiresBackendNativeId() const
    {
        return operation_ == SearchTimerWorkflowOperation::Readback ||
            operation_ == SearchTimerWorkflowOperation::Update ||
            operation_ == SearchTimerWorkflowOperation::Delete;
    }

    bool requiresName() const
    {
        return operation_ == SearchTimerWorkflowOperation::Preview ||
            operation_ == SearchTimerWorkflowOperation::Create ||
            operation_ == SearchTimerWorkflowOperation::Update;
    }

    bool requiresQuery() const
    {
        return operation_ == SearchTimerWorkflowOperation::Preview ||
            operation_ == SearchTimerWorkflowOperation::Create ||
            operation_ == SearchTimerWorkflowOperation::Update;
    }

    bool wantsReadbackAfterWrite() const
    {
        return operation_ == SearchTimerWorkflowOperation::Create ||
            operation_ == SearchTimerWorkflowOperation::Update;
    }

    bool isValid() const
    {
        if (!hasOperation())
        {
            return false;
        }

        if (requiresBackend() && !hasBackend())
        {
            return false;
        }

        if (requiresBackendNativeId() && !hasBackendNativeId())
        {
            return false;
        }

        if (requiresName() && !hasName())
        {
            return false;
        }

        if (requiresQuery() && !hasQuery())
        {
            return false;
        }

        return true;
    }

private:
    SearchTimerWorkflowOperation operation_ =
        SearchTimerWorkflowOperation::Unknown;
    std::string backendId_;
    std::string backendNativeId_;
    std::string name_;
    std::string query_;
    bool active_ = true;
};

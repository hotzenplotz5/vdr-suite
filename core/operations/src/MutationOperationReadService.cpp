#include "MutationOperationReadService.h"

#include "MutationOperationRepository.h"

namespace vdrsuite::operations
{

namespace
{

MutationOperationReadResult result(
    MutationOperationReadStatus status,
    const MutationOperation& operation = {})
{
    MutationOperationReadResult value;
    value.status = status;
    value.operation = operation;
    return value;
}

}

MutationOperationReadService::MutationOperationReadService(
    MutationOperationRepository& repository)
    : repository_(repository)
{
}

MutationOperationReadResult MutationOperationReadService::findForActor(
    const std::string& operationId,
    const std::string& actorId) const
{
    if (operationId.empty() || actorId.empty())
    {
        return result(MutationOperationReadStatus::invalid);
    }

    const MutationOperationRepositoryResult found =
        repository_.findById(operationId);

    if (found.status == MutationOperationRepositoryStatus::invalid)
    {
        return result(MutationOperationReadStatus::invalid);
    }

    if (found.status == MutationOperationRepositoryStatus::notFound)
    {
        return result(MutationOperationReadStatus::notFound);
    }

    if (found.status != MutationOperationRepositoryStatus::ok)
    {
        return result(MutationOperationReadStatus::storageError);
    }

    if (found.operation.actorId != actorId)
    {
        return result(MutationOperationReadStatus::notFound);
    }

    return result(
        MutationOperationReadStatus::ok,
        found.operation);
}

}

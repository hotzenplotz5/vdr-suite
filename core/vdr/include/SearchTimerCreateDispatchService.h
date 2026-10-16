#pragma once

#include "MutationOperation.h"

#include <cstdint>
#include <string>

class ISearchTimerCommandExecutor;
class ISearchTimerDataSource;
namespace vdrsuite::operations { class MutationOperationRepository; }

enum class SearchTimerCreateDispatchStatus
{
    verified,
    outcomeUnknown,
    notReady,
    denied,
    stale,
    conflict,
    unavailable,
};

struct SearchTimerCreateDispatchRequest
{
    std::string operationId;
    std::string actorId;
    std::string backendId;
    std::uint64_t backendGeneration = 0U;
    std::int64_t now = 0;
    bool operatorConfirmed = false;
    bool backendWriteAllowed = false;
    bool executorOptIn = false;
};

struct SearchTimerCreateDispatchResult
{
    SearchTimerCreateDispatchStatus status =
        SearchTimerCreateDispatchStatus::unavailable;
    vdrsuite::operations::MutationOperation operation;
};

// Separate from admission and NOT registered in daemon/Public-v1 yet.
// A strictly one-shot command guarded by a durable accepted->dispatching CAS;
// even crash/timeout/invalid readback NEVER induces a second backend POST.
class SearchTimerCreateDispatchService
{
public:
    explicit SearchTimerCreateDispatchService(
        vdrsuite::operations::MutationOperationRepository& operations);

    SearchTimerCreateDispatchResult dispatchOnce(
        const SearchTimerCreateDispatchRequest& request,
        ISearchTimerCommandExecutor& executor,
        ISearchTimerDataSource& readback);
private:
    vdrsuite::operations::MutationOperationRepository& operations_;
};

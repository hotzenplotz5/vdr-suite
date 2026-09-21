#include "MockVdrTimerActionExecutor.h"

VdrTimerActionResult MockVdrTimerActionExecutor::execute(
    VdrTimerActionType type,
    const VdrTimerOperationRequest& request)
{
    callCount_ += 1;
    lastActionType_ = type;
    lastRequest_ = request;

    if (failNext_)
    {
        failNext_ = false;

        return VdrTimerActionResult::failed(
            type,
            request.timerId,
            request.backendId,
            failMessage_,
            failErrors_);
    }

    return VdrTimerActionResult::ok(
        type,
        request.timerId,
        request.backendId,
        successMessageFor(type));
}

int MockVdrTimerActionExecutor::callCount() const
{
    return callCount_;
}

VdrTimerActionType MockVdrTimerActionExecutor::lastActionType() const
{
    return lastActionType_;
}

VdrTimerOperationRequest MockVdrTimerActionExecutor::lastRequest() const
{
    return lastRequest_;
}

void MockVdrTimerActionExecutor::failNext(
    const std::string& message,
    const std::vector<std::string>& errors)
{
    failNext_ = true;
    failMessage_ = message;
    failErrors_ = errors;
}

std::string MockVdrTimerActionExecutor::successMessageFor(
    VdrTimerActionType type) const
{
    switch (type)
    {
        case VdrTimerActionType::Create:
            return "Timer created";
        case VdrTimerActionType::Update:
            return "Timer updated";
        case VdrTimerActionType::Delete:
            return "Timer deleted";
        case VdrTimerActionType::Toggle:
            return "Timer toggled";
        default:
            return "Timer action executed";
    }
}

#pragma once

#include "IVdrTimerActionExecutor.h"

class MockVdrTimerActionExecutor : public IVdrTimerActionExecutor
{
public:
    VdrTimerActionResult execute(
        VdrTimerActionType type,
        const VdrTimerOperationRequest& request) override;

    int callCount() const;
    VdrTimerActionType lastActionType() const;
    VdrTimerOperationRequest lastRequest() const;

    void failNext(
        const std::string& message,
        const std::vector<std::string>& errors);

private:
    int callCount_ = 0;
    VdrTimerActionType lastActionType_ = VdrTimerActionType::Unknown;
    VdrTimerOperationRequest lastRequest_;

    bool failNext_ = false;
    std::string failMessage_;
    std::vector<std::string> failErrors_;

    std::string successMessageFor(VdrTimerActionType type) const;
};

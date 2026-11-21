#pragma once

#include "RecordingActionRequest.h"
#include "RecordingActionValidationResult.h"

#include <functional>
#include <string>
#include <utility>

class RecordingActionValidationService
{
public:
    using RequestGuard =
        std::function<std::string(const RecordingActionRequest& request)>;

    void setRequestGuard(RequestGuard guard)
    {
        requestGuard_ = std::move(guard);
    }

    RecordingActionValidationResult validate(
        const RecordingActionRequest& request) const;

private:
    RequestGuard requestGuard_;
};

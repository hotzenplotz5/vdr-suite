#pragma once

#include "IRecordingActionExecutor.h"
#include "RecordingActionCapabilityContract.h"

#include <string>

class IRecordingActionBackendExecutorAdapter : public IRecordingActionExecutor
{
public:
    ~IRecordingActionBackendExecutorAdapter() override = default;

    virtual std::string backendId() const = 0;
    virtual std::string backendType() const = 0;
    virtual RecordingActionCapabilitySet capabilities() const = 0;
};

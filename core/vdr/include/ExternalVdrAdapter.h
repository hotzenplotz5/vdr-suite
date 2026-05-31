#pragma once

#include "VdrConfig.h"
#include "VdrStatus.h"

#include <string>

class ExternalVdrAdapter
{
public:
    ExternalVdrAdapter();
    explicit ExternalVdrAdapter(const VdrConfig& config);

    VdrStatus getStatus() const;

private:
    bool enabled_;
    std::string mode_;
    std::string host_;
    int port_;
    std::string state_;
};

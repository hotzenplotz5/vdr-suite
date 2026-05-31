#pragma once

#include "VdrStatus.h"

#include <string>

class ExternalVdrAdapter
{
public:
    ExternalVdrAdapter();

    VdrStatus getStatus() const;

private:
    bool enabled_;
    std::string mode_;
    std::string host_;
    int port_;
    std::string state_;
};

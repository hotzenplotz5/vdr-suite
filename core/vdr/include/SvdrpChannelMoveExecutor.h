
#pragma once

#include "IVdrChannelMoveExecutor.h"

#include <string>

class SvdrpChannelMoveExecutor final : public IVdrChannelMoveExecutor
{
public:
    explicit SvdrpChannelMoveExecutor(
        std::string command = "svdrpsend",
        int timeoutSeconds = 5);

    VdrChannelMoveResult moveChannel(
        const VdrChannelMoveRequest& request) override;

private:
    std::string command_;
    int timeoutSeconds_;
};

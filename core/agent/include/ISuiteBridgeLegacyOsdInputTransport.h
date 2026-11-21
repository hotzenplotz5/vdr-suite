#pragma once

#include "LegacyOsdInputDomain.h"
#include "ISuiteBridgeLocalTransport.h"

namespace vdrsuite::agent
{

class ISuiteBridgeLegacyOsdInputTransport
{
public:
    virtual ~ISuiteBridgeLegacyOsdInputTransport() = default;

    virtual bool legacyOsdInputAvailable() = 0;

    virtual SuiteBridgeCommandReply executeLegacyOsdInput(
        const LegacyOsdInputCommand& command,
        const std::string& requestFingerprint) = 0;
};

}

#pragma once

#include "ISuiteBridgeLocalTransport.h"
#include "SuiteBridgeHandshake.h"
#include "SuiteBridgeLocalContractParser.h"

namespace vdrsuite::agent
{

class SuiteBridgeHandshakeService
{
public:
    explicit SuiteBridgeHandshakeService(
        ISuiteBridgeLocalTransport& transport);

    SuiteBridgeHandshakeResult perform();

private:
    ISuiteBridgeLocalTransport& transport_;
    SuiteBridgeLocalContractParser parser_;
};

}

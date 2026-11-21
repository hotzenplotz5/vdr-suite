#pragma once

#include "ISuiteBridgeHbbtvTransport.h"

#include <cstdint>
#include <string>

struct SuiteBridgeHbbtvRuntimeResolution
{
    bool payloadValid = false;
    std::string error;
    std::string operation;
    std::string result;
    std::uint8_t resultCode = 0;
    std::string state;
    std::uint8_t stateCode = 0;
    std::string sessionId;
    std::string channelId;
    std::uint32_t applicationId = 0;
    std::uint64_t descriptorRevision = 0;
    std::string action;
};

class IHbbtvRuntimeControl
{
public:
    virtual ~IHbbtvRuntimeControl() = default;

    virtual SuiteBridgeHbbtvRuntimeResolution control(
        const SuiteBridgeHbbtvRuntimeRequest& request) = 0;
};

class SuiteBridgeHbbtvRuntimeResolver final : public IHbbtvRuntimeControl
{
public:
    explicit SuiteBridgeHbbtvRuntimeResolver(
        ISuiteBridgeHbbtvTransport& transport);

    SuiteBridgeHbbtvRuntimeResolution control(
        const SuiteBridgeHbbtvRuntimeRequest& request) override;

private:
    ISuiteBridgeHbbtvTransport& transport_;
};

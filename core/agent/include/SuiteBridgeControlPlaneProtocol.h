#pragma once

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace vdrsuite::agent::control
{

constexpr std::uint32_t Magic = 0x56534350U;
constexpr std::uint16_t ProtocolMajor = 1;
constexpr std::uint16_t ProtocolMinor = 0;
constexpr std::size_t RequestHeaderBytes = 32;
constexpr std::size_t ResponseHeaderBytes = 32;
constexpr std::size_t MaximumRequestPayloadBytes = 2048;
constexpr std::size_t MaximumResponsePayloadBytes = 131072;
constexpr std::size_t MaximumRequestFrameBytes =
    RequestHeaderBytes + MaximumRequestPayloadBytes;
constexpr std::size_t MaximumResponseFrameBytes =
    ResponseHeaderBytes + MaximumResponsePayloadBytes;

enum class Operation : std::uint16_t
{
    LiveCapability = 1,
    LiveOpen = 2,
    LiveStatus = 3,
    LiveClose = 4,
    NativeProbeCapability = 5,
    NativeProbeExecute = 6,
    NativeProbeReadback = 7,
    CapabilityDiscovery = 8,
    OsdSnapshot = 9,
    OsdInput = 10,
    HbbtvDiscovery = 11,
    HbbtvRuntime = 12,
    HbbtvPresentation = 13,
    HbbtvMedia = 14,
    TeletextCapability = 15,
    TeletextPage = 16,
};

enum class ServiceClass : std::uint16_t
{
    CriticalControl = 1,
    InteractiveControlRead = 2,
    ExternalPluginInteractive = 3,
};

enum class Result : std::uint16_t
{
    Success = 0,
    NativeRejected = 1,
    StaleRejected = 2,
    Overloaded = 3,
    DeadlineExpired = 4,
    ProviderUnavailable = 5,
    ProtocolMismatch = 6,
    PeerRejected = 7,
    InternalFailure = 8,
};

struct Request
{
    std::uint16_t major = ProtocolMajor;
    std::uint16_t minor = ProtocolMinor;
    Operation operation = Operation::LiveCapability;
    std::uint64_t requestId = 0;
    std::uint64_t deadlineNanoseconds = 0;
    std::string payload;
};

struct Response
{
    std::uint16_t major = ProtocolMajor;
    std::uint16_t minor = ProtocolMinor;
    Result result = Result::InternalFailure;
    std::uint64_t requestId = 0;
    std::int32_t replyCode = 0;
    std::string payload;
};

inline const char* operationName(Operation operation)
{
    switch (operation)
    {
        case Operation::LiveCapability: return "live-capability";
        case Operation::LiveOpen: return "live-open";
        case Operation::LiveStatus: return "live-status";
        case Operation::LiveClose: return "live-close";
        case Operation::NativeProbeCapability: return "native-probe-capability";
        case Operation::NativeProbeExecute: return "native-probe-execute";
        case Operation::NativeProbeReadback: return "native-probe-readback";
        case Operation::CapabilityDiscovery: return "capability-discovery";
        case Operation::OsdSnapshot: return "osd-snapshot";
        case Operation::OsdInput: return "osd-input";
        case Operation::HbbtvDiscovery: return "hbbtv-discovery";
        case Operation::HbbtvRuntime: return "hbbtv-runtime";
        case Operation::HbbtvPresentation: return "hbbtv-presentation";
        case Operation::HbbtvMedia: return "hbbtv-media";
        case Operation::TeletextCapability: return "teletext-capability";
        case Operation::TeletextPage: return "teletext-page";
    }
    return "unknown";
}

inline bool knownOperation(Operation operation)
{
    const auto value = static_cast<std::uint16_t>(operation);
    return value >= 1 && value <= 16;
}

inline ServiceClass serviceClass(Operation operation)
{
    switch (operation)
    {
        case Operation::CapabilityDiscovery:
        case Operation::OsdSnapshot:
        case Operation::OsdInput:
            return ServiceClass::InteractiveControlRead;
        case Operation::HbbtvDiscovery:
        case Operation::HbbtvRuntime:
        case Operation::HbbtvPresentation:
        case Operation::HbbtvMedia:
        case Operation::TeletextCapability:
        case Operation::TeletextPage:
            return ServiceClass::ExternalPluginInteractive;
        default:
            return ServiceClass::CriticalControl;
    }
}

inline std::uint64_t monotonicNowNanoseconds()
{
    timespec value{};
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0)
        return 0;
    return static_cast<std::uint64_t>(value.tv_sec) * 1000000000ULL +
        static_cast<std::uint64_t>(value.tv_nsec);
}

inline void append16(std::vector<std::uint8_t>& out, std::uint16_t value)
{
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
    out.push_back(static_cast<std::uint8_t>(value & 0xff));
}

inline void append32(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xff));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xff));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
    out.push_back(static_cast<std::uint8_t>(value & 0xff));
}

inline void append64(std::vector<std::uint8_t>& out, std::uint64_t value)
{
    append32(out, static_cast<std::uint32_t>(value >> 32));
    append32(out, static_cast<std::uint32_t>(value));
}

inline bool read16(
    const std::vector<std::uint8_t>& data,
    std::size_t offset,
    std::uint16_t& value)
{
    if (offset + 2 > data.size())
        return false;
    value = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8) |
        data[offset + 1]);
    return true;
}

inline bool read32(
    const std::vector<std::uint8_t>& data,
    std::size_t offset,
    std::uint32_t& value)
{
    if (offset + 4 > data.size())
        return false;
    value =
        (static_cast<std::uint32_t>(data[offset]) << 24) |
        (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
        (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
        data[offset + 3];
    return true;
}

inline bool read64(
    const std::vector<std::uint8_t>& data,
    std::size_t offset,
    std::uint64_t& value)
{
    std::uint32_t high = 0;
    std::uint32_t low = 0;
    if (!read32(data, offset, high) ||
        !read32(data, offset + 4, low))
        return false;
    value = (static_cast<std::uint64_t>(high) << 32) | low;
    return true;
}

inline bool encodeRequest(
    const Request& request,
    std::vector<std::uint8_t>& out)
{
    if (!knownOperation(request.operation) ||
        request.requestId == 0 ||
        request.deadlineNanoseconds == 0 ||
        request.payload.size() > MaximumRequestPayloadBytes)
        return false;

    out.clear();
    out.reserve(RequestHeaderBytes + request.payload.size());
    append32(out, Magic);
    append16(out, request.major);
    append16(out, request.minor);
    append16(out, static_cast<std::uint16_t>(request.operation));
    append16(out, static_cast<std::uint16_t>(serviceClass(request.operation)));
    append64(out, request.requestId);
    append64(out, request.deadlineNanoseconds);
    append32(out, static_cast<std::uint32_t>(request.payload.size()));
    out.insert(out.end(), request.payload.begin(), request.payload.end());
    return true;
}

inline bool decodeRequest(
    const std::vector<std::uint8_t>& data,
    Request& request,
    std::string& reason)
{
    if (data.size() < RequestHeaderBytes ||
        data.size() > MaximumRequestFrameBytes)
    {
        reason = "request_frame_size";
        return false;
    }

    std::uint32_t magic = 0;
    std::uint16_t operation = 0;
    std::uint16_t wireClass = 0;
    std::uint32_t payloadSize = 0;
    if (!read32(data, 0, magic) ||
        !read16(data, 4, request.major) ||
        !read16(data, 6, request.minor) ||
        !read16(data, 8, operation) ||
        !read16(data, 10, wireClass) ||
        !read64(data, 12, request.requestId) ||
        !read64(data, 20, request.deadlineNanoseconds) ||
        !read32(data, 28, payloadSize))
    {
        reason = "request_header";
        return false;
    }

    request.operation = static_cast<Operation>(operation);
    if (magic != Magic)
    {
        reason = "magic";
        return false;
    }
    if (request.major != ProtocolMajor || request.minor > ProtocolMinor)
    {
        reason = "version";
        return false;
    }
    if (!knownOperation(request.operation) ||
        wireClass != static_cast<std::uint16_t>(
            serviceClass(request.operation)))
    {
        reason = "operation_registry";
        return false;
    }
    if (request.requestId == 0 || request.deadlineNanoseconds == 0)
    {
        reason = "identity_deadline";
        return false;
    }
    if (payloadSize > MaximumRequestPayloadBytes ||
        data.size() != RequestHeaderBytes + payloadSize)
    {
        reason = "request_payload_size";
        return false;
    }

    request.payload.assign(
        reinterpret_cast<const char*>(
            data.data() + RequestHeaderBytes),
        payloadSize);
    return true;
}

inline bool encodeResponse(
    const Response& response,
    std::vector<std::uint8_t>& out)
{
    if (response.requestId == 0 ||
        response.payload.size() > MaximumResponsePayloadBytes)
        return false;

    out.clear();
    out.reserve(ResponseHeaderBytes + response.payload.size());
    append32(out, Magic);
    append16(out, response.major);
    append16(out, response.minor);
    append16(out, static_cast<std::uint16_t>(response.result));
    append16(out, 0);
    append64(out, response.requestId);
    append32(out, static_cast<std::uint32_t>(response.replyCode));
    append32(out, static_cast<std::uint32_t>(response.payload.size()));
    append32(out, 0);
    out.insert(out.end(), response.payload.begin(), response.payload.end());
    return true;
}

inline bool decodeResponse(
    const std::vector<std::uint8_t>& data,
    Response& response,
    std::string& reason)
{
    if (data.size() < ResponseHeaderBytes ||
        data.size() > MaximumResponseFrameBytes)
    {
        reason = "response_frame_size";
        return false;
    }

    std::uint32_t magic = 0;
    std::uint16_t result = 0;
    std::uint16_t reserved = 0;
    std::uint32_t replyCode = 0;
    std::uint32_t payloadSize = 0;
    std::uint32_t tailReserved = 0;
    if (!read32(data, 0, magic) ||
        !read16(data, 4, response.major) ||
        !read16(data, 6, response.minor) ||
        !read16(data, 8, result) ||
        !read16(data, 10, reserved) ||
        !read64(data, 12, response.requestId) ||
        !read32(data, 20, replyCode) ||
        !read32(data, 24, payloadSize) ||
        !read32(data, 28, tailReserved))
    {
        reason = "response_header";
        return false;
    }
    if (magic != Magic ||
        response.major != ProtocolMajor ||
        response.minor > ProtocolMinor ||
        reserved != 0 ||
        tailReserved != 0)
    {
        reason = "response_protocol";
        return false;
    }
    if (payloadSize > MaximumResponsePayloadBytes ||
        data.size() != ResponseHeaderBytes + payloadSize)
    {
        reason = "response_payload_size";
        return false;
    }

    response.result = static_cast<Result>(result);
    response.replyCode = static_cast<std::int32_t>(replyCode);
    response.payload.assign(
        reinterpret_cast<const char*>(
            data.data() + ResponseHeaderBytes),
        payloadSize);
    return true;
}

} // namespace vdrsuite::agent::control

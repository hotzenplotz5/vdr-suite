#pragma once
// Shared, read-only validation boundary: VDR's video root is never a browser path.
#include "ISuiteBridgeLocalTransport.h"

#include <filesystem>
#include <string>
#include <utility>

namespace vdrsuite::agent
{

struct SuiteBridgeVideoDirectoryDiscoveryResult
{
    bool authoritativeReply = false; // A successful VPATH 1 answer, even if invalid.
    std::string videoDirectory;
};

class SuiteBridgeVideoDirectoryDiscovery final
{
public:
    static std::string validRoot(const std::string& raw)
    {
        if (raw.empty() || raw.size() > 4096U)
            return {};

        for (const unsigned char character : raw)
            if (character < 0x20U || character == 0x7FU)
                return {};

        const std::filesystem::path path(raw);
        if (!path.is_absolute())
            return {};
        for (const auto& component : path)
            if (component == "..")
                return {};

        const std::filesystem::path normalized = path.lexically_normal();
        if (!normalized.is_absolute() || normalized == normalized.root_path())
            return {};
        std::string result = normalized.string();
        while (result.size() > 1U && result.back() == '/')
            result.pop_back();
        return result;
    }

    static SuiteBridgeVideoDirectoryDiscoveryResult discover(
        ISuiteBridgeLocalTransport& transport)
    {
        const SuiteBridgeCommandReply reply =
            transport.execute(SuiteBridgeLocalCommand::VideoDirectory);

        if (!reply.transportSucceeded())
            return {}; // Transport offline: preserve legacy Ubuntu behavior.
        if (reply.replyCode == 451 || reply.replyCode == 504)
            return {true, {}}; // Known command, unusable root/schema: deny writes.
        if (reply.replyCode != 900)
            return {}; // Older SuiteBridge lacks the VPATH command.

        return {true, validRoot(reply.payload)};
    }
};

} // namespace vdrsuite::agent

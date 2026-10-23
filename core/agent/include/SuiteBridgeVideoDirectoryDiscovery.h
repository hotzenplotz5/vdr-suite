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
        return normalized.string();
    }

    static SuiteBridgeVideoDirectoryDiscoveryResult discover(
        ISuiteBridgeLocalTransport& transport)
    {
        const SuiteBridgeCommandReply reply =
            transport.execute(SuiteBridgeLocalCommand::VideoDirectory);

        if (!reply.transportSucceeded() || reply.replyCode != 900)
            return {}; // Old plugins and offline VDRs keep legacy behavior.

        return {true, validRoot(reply.payload)};
    }
};

} // namespace vdrsuite::agent

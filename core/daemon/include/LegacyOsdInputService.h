#pragma once

#include "BackendAgentCommand.h"
#include "LegacyOsdInputDomain.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

class BackendAgentCommandRepository;
class BackendAgentRepository;
class LegacyOsdSessionService;
class OsdControllerLeaseService;
class OsdViewerBindingService;

class LegacyOsdInputService final : public ILegacyOsdInputService
{
public:
    static constexpr std::int64_t MaximumDeadlineLeadSeconds = 3;
    static constexpr std::uint64_t MaximumAcceptedCommandsPerSecond = 12;
    static constexpr std::size_t MaximumRateWindows = 128;

    using NowProvider = std::function<std::int64_t()>;

    LegacyOsdInputService(
        LegacyOsdSessionService& sessionService,
        OsdViewerBindingService& viewerService,
        OsdControllerLeaseService& controllerService,
        BackendAgentRepository& agentRepository,
        BackendAgentCommandRepository& commandRepository,
        NowProvider nowProvider = {});

    LegacyOsdInputResult submit(const LegacyOsdInputCommand& command) override;

    bool revalidateReceipt(
        const BackendAgentCommandReceipt& receipt,
        std::string& reasonCode);

private:
    struct RateWindow
    {
        std::int64_t second = 0;
        std::uint64_t accepted = 0;
    };

    bool authorityCurrent(
        const LegacyOsdInputCommand& command,
        std::string& reasonCode);
    bool sameSemanticCommand(
        const LegacyOsdInputCommand& left,
        const LegacyOsdInputCommand& right) const;
    bool rateAllowed(
        const std::string& leaseId,
        std::int64_t now);

    LegacyOsdSessionService& sessionService_;
    OsdViewerBindingService& viewerService_;
    OsdControllerLeaseService& controllerService_;
    BackendAgentRepository& agentRepository_;
    BackendAgentCommandRepository& commandRepository_;
    NowProvider nowProvider_;
    std::mutex rateMutex_;
    std::unordered_map<std::string, RateWindow> rateWindows_;
};

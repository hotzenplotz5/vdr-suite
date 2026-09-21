#pragma once

#include "BackendAgentCommand.h"
#include "LegacyOsdInputDomain.h"

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

struct LegacyOsdInputResult
{
    bool accepted = false;
    bool idempotent = false;
    std::string category;
    std::string error;
    std::string agentCommandId;
};

class LegacyOsdInputService
{
public:
    static constexpr std::int64_t MaximumDeadlineLeadSeconds = 3;
    static constexpr std::uint64_t MaximumAcceptedCommandsPerSecond = 12;

    using NowProvider = std::function<std::int64_t()>;

    LegacyOsdInputService(
        LegacyOsdSessionService& sessionService,
        OsdViewerBindingService& viewerService,
        OsdControllerLeaseService& controllerService,
        BackendAgentRepository& agentRepository,
        BackendAgentCommandRepository& commandRepository,
        NowProvider nowProvider = {});

    LegacyOsdInputResult submit(const LegacyOsdInputCommand& command);

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

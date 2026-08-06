#include "AccountabilityEventRepository.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "Database.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
RequestSecurityContext administrator(const std::string& backendId)
{
    RequestSecurityContext context;
    context.requestId = backendAgentGenerateOpaqueId("req_", 8);
    context.correlationId = context.requestId;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = ActorIdentity{
        "system:backend-agent-command-admin",
        ActorType::System,
        "Backend Agent command administration utility",
        true};
    context.device = DeviceIdentity{"device:backend-agent-command-admin", true};
    context.credential = CredentialIdentity{
        "credential:backend-agent-command-admin", true, false, false};
    context.grants.push_back(PermissionGrant{"role.admin", backendId});
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return context;
}

std::string escape(const std::string& value)
{
    std::ostringstream output;
    for (unsigned char character : value)
    {
        if (character == '\\') output << "\\\\";
        else if (character == '"') output << "\\\"";
        else if (character >= 0x20U) output << static_cast<char>(character);
    }
    return output.str();
}

void usage()
{
    std::cerr
        << "usage: vdr-suite-backend-agent-command-admin "
           "[--database PATH] [--backend ID] "
           "(--status | --enqueue-probe | --enqueue-native-probe "
           "[--deadline-seconds N] | --replay COMMAND_ID | "
           "--arm-lost-receipt-response | --arm-lost-result-response)"
        << std::endl;
}
}

int main(int argc, char** argv)
{
    std::string databasePath = "/var/lib/vdr-suite/vdr-suite.db";
    std::string backendId = "default";
    std::string replayId;
    int deadlineSeconds = 300;
    enum class Action
    {
        None,
        Status,
        EnqueueLegacyProbe,
        EnqueueNativeProbe,
        Replay,
        FaultReceipt,
        FaultResult
    };
    Action action = Action::None;

    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument == "--database" && index + 1 < argc)
            databasePath = argv[++index];
        else if (argument == "--backend" && index + 1 < argc)
            backendId = argv[++index];
        else if (argument == "--deadline-seconds" && index + 1 < argc)
            deadlineSeconds = std::atoi(argv[++index]);
        else if (argument == "--status")
            action = action == Action::None ? Action::Status : Action::None;
        else if (argument == "--enqueue-probe")
            action = action == Action::None
                ? Action::EnqueueLegacyProbe : Action::None;
        else if (argument == "--enqueue-native-probe")
            action = action == Action::None
                ? Action::EnqueueNativeProbe : Action::None;
        else if (argument == "--replay" && index + 1 < argc)
        {
            action = action == Action::None ? Action::Replay : Action::None;
            replayId = argv[++index];
        }
        else if (argument == "--arm-lost-receipt-response")
            action = action == Action::None
                ? Action::FaultReceipt : Action::None;
        else if (argument == "--arm-lost-result-response")
            action = action == Action::None
                ? Action::FaultResult : Action::None;
        else
        {
            usage();
            return 64;
        }
    }

    if (action == Action::None ||
        !backendAgentCommandSafeIdentifier(backendId) ||
        deadlineSeconds < 30 || deadlineSeconds > 3600 ||
        (action == Action::Replay &&
         !backendAgentCommandSafeIdentifier(replayId)))
    {
        usage();
        return 64;
    }

    Database database;
    if (!database.open(databasePath))
    {
        std::cerr << "failed to open Backend Agent database" << std::endl;
        return 74;
    }
    AccountabilityEventRepository accountability(database);
    BackendAgentRepository agents(database);
    BackendAgentCommandRepository commands(database);
    if (!accountability.ensureSchema() || !agents.ensureSchema() ||
        !commands.ensureSchema())
    {
        std::cerr << "failed to initialize Backend Agent command repositories"
                  << std::endl;
        return 74;
    }
    BackendAgentCommandDeliveryService service(
        commands, agents, accountability);
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (action == Action::Status)
    {
        const auto summary = service.summaryForBackend(backendId);
        std::cout << "{\"present\":"
                  << (summary.present ? "true" : "false");
        if (summary.present)
        {
            std::cout
                << ",\"commandId\":\"" << escape(summary.commandId)
                << "\",\"commandType\":\"" << escape(summary.commandType)
                << "\",\"state\":\"" << escape(summary.state)
                << "\",\"receiptCategory\":\""
                << escape(summary.receiptCategory)
                << "\",\"resultCategory\":\""
                << escape(summary.resultCategory)
                << "\",\"dispatchState\":\""
                << escape(summary.dispatchState)
                << "\",\"verificationState\":\""
                << escape(summary.verificationState)
                << "\",\"backendGeneration\":"
                << summary.backendGeneration
                << ",\"claimEpoch\":" << summary.claimEpoch
                << ",\"deliveryCount\":" << summary.deliveryCount
                << ",\"receiptReplayCount\":"
                << summary.receiptReplayCount
                << ",\"resultReplayCount\":"
                << summary.resultReplayCount
                << ",\"deadline\":" << summary.deadline;
        }
        std::cout << "}" << std::endl;
        return 0;
    }

    std::string reason;
    const auto context = administrator(backendId);
    if (action == Action::EnqueueLegacyProbe ||
        action == Action::EnqueueNativeProbe)
    {
        const auto assignment = action == Action::EnqueueNativeProbe
            ? service.assignNativeProbe(
                context, backendId, now, now + deadlineSeconds, reason)
            : service.assignProbe(
                context, backendId, now, now + deadlineSeconds, reason);
        if (!assignment)
        {
            std::cerr << reason << std::endl;
            return 1;
        }
        std::cout
            << "{\"commandId\":\"" << escape(assignment->commandId)
            << "\",\"operationId\":\"" << escape(assignment->operationId)
            << "\",\"jobId\":\"" << escape(assignment->jobId)
            << "\",\"attemptId\":\"" << escape(assignment->attemptId)
            << "\",\"claimEpoch\":" << assignment->claimEpoch
            << ",\"requestFingerprint\":\""
            << escape(assignment->requestFingerprint) << "\"}"
            << std::endl;
        return 0;
    }

    const bool accepted = action == Action::Replay
        ? service.requestReplay(
            context, backendId, replayId, now, reason)
        : service.armFault(
            context,
            backendId,
            action == Action::FaultReceipt ? "receipt" : "result",
            now,
            reason);
    if (!accepted)
    {
        std::cerr << reason << std::endl;
        return 1;
    }
    std::cout << "{\"accepted\":true,\"reasonCode\":\""
              << escape(reason) << "\"}" << std::endl;
    return 0;
}

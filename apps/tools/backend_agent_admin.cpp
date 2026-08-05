#include "AccountabilityEventRepository.h"
#include "BackendAgentLifecycle.h"
#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
RequestSecurityContext localAdministrator(const std::string& backendId)
{
    RequestSecurityContext context;
    context.requestId = backendAgentGenerateOpaqueId("req_", 8);
    context.correlationId = context.requestId;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = ActorIdentity{
        "system:backend-agent-admin", ActorType::System,
        "Backend Agent administration utility", true};
    context.device = DeviceIdentity{"device:backend-agent-admin", true};
    context.credential = CredentialIdentity{
        "credential:backend-agent-admin", true, false, false};
    context.grants.push_back(PermissionGrant{"role.admin", backendId});
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return context;
}

bool safeReason(const std::string& value)
{
    return !value.empty() && value.size() <= 256 &&
        std::none_of(value.begin(), value.end(), [](unsigned char character) {
            return character < 0x20 || character == 0x7f;
        });
}

std::string jsonEscape(const std::string& value)
{
    std::ostringstream escaped;
    for (unsigned char character : value)
    {
        switch (character)
        {
            case '\\': escaped << "\\\\"; break;
            case '"': escaped << "\\\""; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (character < 0x20)
                {
                    static const char* Hex = "0123456789abcdef";
                    escaped << "\\u00" << Hex[(character >> 4) & 0xf]
                            << Hex[character & 0xf];
                }
                else escaped << static_cast<char>(character);
        }
    }
    return escaped.str();
}

std::string jsonArray(const std::vector<std::string>& values)
{
    std::ostringstream output;
    output << '[';
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index != 0) output << ',';
        output << '"' << jsonEscape(values[index]) << '"';
    }
    output << ']';
    return output.str();
}

void printStatus(const BackendAgentStatus& status)
{
    std::cout << "{\"present\":" << (status.present ? "true" : "false");
    if (status.present)
    {
        std::cout << ",\"agentId\":\"" << jsonEscape(status.agentId)
                  << "\",\"backendId\":\"" << jsonEscape(status.backendId)
                  << "\",\"state\":\""
                  << backendAgentConnectionStateName(status.state)
                  << "\",\"backendGeneration\":" << status.backendGeneration
                  << ",\"heartbeatSequence\":" << status.heartbeatSequence
                  << ",\"capabilityRevision\":" << status.capabilityRevision
                  << ",\"lastHeartbeatAt\":" << status.lastHeartbeatAt
                  << ",\"leaseExpiresAt\":" << status.leaseExpiresAt
                  << ",\"readOnly\":"
                  << (status.capabilities.readOnly ? "true" : "false")
                  << ",\"adapters\":" << jsonArray(status.capabilities.adapters)
                  << ",\"observationDomains\":"
                  << jsonArray(status.capabilities.observationDomains);
    }
    std::cout << "}" << std::endl;
}
}

int main(int argc, char** argv)
{
    std::string databasePath = "/var/lib/vdr-suite/vdr-suite.db";
    std::string backendId = "default";
    std::string reason = "operator-revoked";
    bool statusAction = false;
    bool revokeAction = false;

    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument == "--database" && index + 1 < argc) databasePath = argv[++index];
        else if (argument == "--backend" && index + 1 < argc) backendId = argv[++index];
        else if (argument == "--reason" && index + 1 < argc) reason = argv[++index];
        else if (argument == "--status") statusAction = true;
        else if (argument == "--revoke") revokeAction = true;
        else
        {
            std::cerr << "usage: vdr-suite-backend-agent-admin "
                         "[--database PATH] [--backend ID] "
                         "(--status | --revoke [--reason TEXT])" << std::endl;
            return 64;
        }
    }

    if (statusAction == revokeAction ||
        !BackendAgentLifecycleService::safeIdentifier(backendId) ||
        (revokeAction && !safeReason(reason)))
    {
        std::cerr << "invalid Backend Agent administration request" << std::endl;
        return 64;
    }

    Database database;
    if (!database.open(databasePath))
    {
        std::cerr << "failed to open Backend Agent database" << std::endl;
        return 74;
    }
    SecurityIdentityRepository identities(database);
    SecurityIdentityProvisioningRepository provisioning(database);
    CredentialVerifierRepository verifiers(database);
    AccountabilityEventRepository accountability(database);
    BackendAgentRepository agents(database);
    if (!identities.ensureSchema() || !verifiers.ensureSchema() ||
        !accountability.ensureSchema() || !agents.ensureSchema() ||
        !provisioning.ensureTechnicalIdentity(
            "system:backend-agent-admin", ActorType::System,
            "Backend Agent administration utility",
            "device:backend-agent-admin", "Local Agent administration utility",
            "credential:backend-agent-admin", "local-system"))
    {
        std::cerr << "failed to initialize Backend Agent administration repositories"
                  << std::endl;
        return 74;
    }

    BackendRegistry registry;
    BackendNode backend;
    backend.backendId = backendId;
    backend.backendName = backendId;
    backend.accessMode = "read-only";
    backend.enabled = true;
    registry.addBackend(backend);
    BackendRegistryService registryService(registry);
    BackendAgentLifecycleService service(
        database, agents, registryService, provisioning, identities,
        verifiers, accountability);

    const std::int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (statusAction)
    {
        printStatus(service.statusForBackend(backendId, now));
        return 0;
    }

    const auto active = agents.findAgentForBackend(backendId);
    if (!active.has_value() || active->revoked)
    {
        std::cerr << "no active Backend Agent for backend " << backendId << std::endl;
        return 1;
    }
    std::string reasonCode;
    if (!service.revoke(
            localAdministrator(backendId), active->agentId, reason, now, reasonCode))
    {
        std::cerr << "failed to revoke Backend Agent: " << reasonCode << std::endl;
        return 1;
    }
    std::cout << "Backend Agent revoked for backend " << backendId << std::endl;
    return 0;
}

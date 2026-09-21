#include "AccountabilityEventRepository.h"
#include "BackendAgentClient.h"
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
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
bool parseTtl(const std::string& value, std::int64_t& ttl)
{
    if (value.empty() || value.size() > 6) return false;
    char* end = nullptr;
    errno = 0;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (errno != 0 || end == nullptr || *end != '\0' || parsed < 300 || parsed > 86400)
        return false;
    ttl = parsed;
    return true;
}

RequestSecurityContext localAdministrator(const std::string& backendId)
{
    RequestSecurityContext context;
    context.requestId = backendAgentGenerateOpaqueId("req_", 8);
    context.correlationId = context.requestId;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = ActorIdentity{
        "system:backend-agent-enrollment", ActorType::System,
        "Backend Agent enrollment utility", true};
    context.device = DeviceIdentity{"device:backend-agent-enrollment", true};
    context.credential = CredentialIdentity{
        "credential:backend-agent-enrollment", true, false, false};
    context.grants.push_back(PermissionGrant{"role.admin", backendId});
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return context;
}
}

int main(int argc, char** argv)
{
    std::string databasePath = "/var/lib/vdr-suite/vdr-suite.db";
    std::string backendId = "default";
    std::string outputPath;
    std::int64_t ttl = 900;

    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument == "--database" && index + 1 < argc) databasePath = argv[++index];
        else if (argument == "--backend" && index + 1 < argc) backendId = argv[++index];
        else if (argument == "--output" && index + 1 < argc) outputPath = argv[++index];
        else if (argument == "--ttl-seconds" && index + 1 < argc)
        {
            if (!parseTtl(argv[++index], ttl))
            {
                std::cerr << "invalid enrollment TTL" << std::endl;
                return 64;
            }
        }
        else
        {
            std::cerr << "usage: vdr-suite-backend-agent-enroll "
                         "[--database PATH] [--backend ID] --output PATH "
                         "[--ttl-seconds 300..86400]" << std::endl;
            return 64;
        }
    }

    if (outputPath.empty() || !BackendAgentLifecycleService::safeIdentifier(backendId))
    {
        std::cerr << "invalid enrollment output or backend" << std::endl;
        return 64;
    }
    struct stat existing{};
    if (lstat(outputPath.c_str(), &existing) == 0 || errno != ENOENT)
    {
        std::cerr << "enrollment output already exists or cannot be inspected" << std::endl;
        return 73;
    }

    Database database;
    if (!database.open(databasePath))
    {
        std::cerr << "failed to open enrollment database" << std::endl;
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
            "system:backend-agent-enrollment", ActorType::System,
            "Backend Agent enrollment utility",
            "device:backend-agent-enrollment", "Local enrollment utility",
            "credential:backend-agent-enrollment", "local-system"))
    {
        std::cerr << "failed to initialize enrollment repositories" << std::endl;
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

    BackendAgentEnrollmentPackage package;
    package.enrollmentId = backendAgentGenerateOpaqueId("enr_", 16);
    package.backendId = backendId;
    package.enrollmentToken = backendAgentGenerateOpaqueId("ent_", 24);
    const std::string tokenHash = backendAgentHashSecret(package.enrollmentToken);
    const std::string pendingPath = outputPath + ".pending";
    std::string reason;
    if (package.enrollmentId.empty() || package.enrollmentToken.size() < 32 ||
        tokenHash.empty() ||
        !writeBackendAgentEnrollmentPackageAtomically(pendingPath, package, reason))
    {
        std::cerr << "failed to prepare enrollment package: " << reason << std::endl;
        return 74;
    }

    const std::int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (!service.createEnrollment(
            localAdministrator(backendId), package.enrollmentId, backendId,
            tokenHash, now + ttl, now, reason))
    {
        unlink(pendingPath.c_str());
        std::cerr << "failed to create controlled enrollment: " << reason << std::endl;
        return 1;
    }
    if (rename(pendingPath.c_str(), outputPath.c_str()) != 0)
    {
        unlink(pendingPath.c_str());
        std::string revocationReason;
        service.revokeEnrollment(
            localAdministrator(backendId), package.enrollmentId,
            "enrollment-package-finalization-failed", now, revocationReason);
        std::cerr << "failed to finalize enrollment package" << std::endl;
        return 74;
    }
    if (chmod(outputPath.c_str(), 0600) != 0)
    {
        unlink(outputPath.c_str());
        std::string revocationReason;
        service.revokeEnrollment(
            localAdministrator(backendId), package.enrollmentId,
            "enrollment-package-protection-failed", now, revocationReason);
        std::cerr << "failed to protect enrollment package" << std::endl;
        return 74;
    }

    std::fill(package.enrollmentToken.begin(), package.enrollmentToken.end(), '\0');
    std::cout << "Controlled Backend Agent enrollment package created for backend "
              << backendId << " at " << outputPath << std::endl;
    return 0;
}

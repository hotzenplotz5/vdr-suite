#pragma once

class BackendAccessPolicy;
class BackendAgentLifecycleService;
class BackendAgentRepository;
class BackendAgentCommandRepository;
class BackendAgentCommandDeliveryService;
class BackendRegistryService;
class Database;
class SecurityIdentityRepository;

bool configureDaemonLegacyOsdRuntime(
    Database& database,
    SecurityIdentityRepository& identityRepository,
    BackendAgentLifecycleService& lifecycleService,
    BackendAgentRepository& agentRepository,
    BackendAgentCommandRepository& commandRepository,
    BackendAgentCommandDeliveryService& commandDeliveryService,
    BackendRegistryService& backendRegistryService,
    BackendAccessPolicy& backendAccessPolicy);

void resetDaemonLegacyOsdRuntime();

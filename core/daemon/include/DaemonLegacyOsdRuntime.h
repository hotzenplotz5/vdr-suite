#pragma once

class BackendAccessPolicy;
class BackendAgentLifecycleService;
class BackendRegistryService;
class Database;
class SecurityIdentityRepository;

bool configureDaemonLegacyOsdRuntime(
    Database& database,
    SecurityIdentityRepository& identityRepository,
    BackendAgentLifecycleService& lifecycleService,
    BackendRegistryService& backendRegistryService,
    BackendAccessPolicy& backendAccessPolicy);

void resetDaemonLegacyOsdRuntime();

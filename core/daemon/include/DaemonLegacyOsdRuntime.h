#pragma once

class BackendAgentLifecycleService;
class Database;
class SecurityIdentityRepository;

bool configureDaemonLegacyOsdRuntime(
    Database& database,
    SecurityIdentityRepository& identityRepository,
    BackendAgentLifecycleService& lifecycleService);

void resetDaemonLegacyOsdRuntime();

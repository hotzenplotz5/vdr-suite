#pragma once

#include <memory>
#include <string>
#include <vector>

class BackendAgentLifecycleService;
class Database;
class EmbeddedBackendLifecycleService;
class BackendRegistryService;
class HbbtvApplicationSessionService;
class HbbtvControlPlaneReadService;
class VdrSnapshotReadService;
struct BackendRuntimeContext;

bool configureDaemonHbbtvRuntime(
    Database& database,
    const std::string& defaultDatabasePath,
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    EmbeddedBackendLifecycleService& embeddedBackendLifecycleService,
    BackendAgentLifecycleService& backendAgentLifecycleService,
    std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts);

void resetDaemonHbbtvRuntime();

HbbtvControlPlaneReadService* daemonHbbtvControlPlaneReadService();
HbbtvApplicationSessionService* daemonHbbtvApplicationSessionService();

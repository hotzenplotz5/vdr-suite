#pragma once

#include <memory>
#include <string>
#include <vector>

class EmbeddedBackendLifecycleService;
class BackendRegistryService;
class HbbtvApplicationSessionService;
class HbbtvControlPlaneReadService;
class VdrSnapshotReadService;
struct BackendRuntimeContext;

bool configureDaemonHbbtvRuntime(
    const std::string& defaultDatabasePath,
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    EmbeddedBackendLifecycleService& embeddedBackendLifecycleService,
    std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts);

void resetDaemonHbbtvRuntime();

HbbtvControlPlaneReadService* daemonHbbtvControlPlaneReadService();
HbbtvApplicationSessionService* daemonHbbtvApplicationSessionService();

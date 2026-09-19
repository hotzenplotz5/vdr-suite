#pragma once

#include <memory>
#include <vector>

class Database;
class EmbeddedBackendLifecycleService;
class BackendRegistryService;
class HbbtvApplicationSessionService;
class HbbtvControlPlaneReadService;
class VdrSnapshotReadService;
struct BackendRuntimeContext;

bool configureDaemonHbbtvRuntime(
    Database& database,
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    EmbeddedBackendLifecycleService& embeddedBackendLifecycleService,
    std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts);

void resetDaemonHbbtvRuntime();

HbbtvControlPlaneReadService* daemonHbbtvControlPlaneReadService();
HbbtvApplicationSessionService* daemonHbbtvApplicationSessionService();

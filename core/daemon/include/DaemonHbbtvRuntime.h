#pragma once

#include <memory>
#include <vector>

class EmbeddedBackendLifecycleService;
class BackendRegistryService;
class HbbtvControlPlaneReadService;
class VdrSnapshotReadService;
struct BackendRuntimeContext;

bool configureDaemonHbbtvRuntime(
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    EmbeddedBackendLifecycleService& embeddedBackendLifecycleService,
    std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts);

void resetDaemonHbbtvRuntime();

HbbtvControlPlaneReadService* daemonHbbtvControlPlaneReadService();

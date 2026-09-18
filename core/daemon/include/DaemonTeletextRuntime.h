#pragma once

#include <memory>
#include <vector>

class EmbeddedBackendLifecycleService;
class BackendRegistryService;
class TeletextControlPlaneReadService;
class VdrSnapshotReadService;
struct BackendRuntimeContext;

bool configureDaemonTeletextRuntime(
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    EmbeddedBackendLifecycleService& embeddedBackendLifecycleService,
    std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts);

void resetDaemonTeletextRuntime();

TeletextControlPlaneReadService* daemonTeletextControlPlaneReadService();

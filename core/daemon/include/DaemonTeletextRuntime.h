#pragma once

#include <memory>
#include <vector>

class BackendAgentLifecycleService;
class Database;
class EmbeddedBackendLifecycleService;
class BackendRegistryService;
class TeletextControlPlaneReadService;
class VdrSnapshotReadService;
struct BackendRuntimeContext;

bool configureDaemonTeletextRuntime(
    Database& database,
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    EmbeddedBackendLifecycleService& embeddedBackendLifecycleService,
    BackendAgentLifecycleService& backendAgentLifecycleService,
    std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts);

void resetDaemonTeletextRuntime();

TeletextControlPlaneReadService* daemonTeletextControlPlaneReadService();

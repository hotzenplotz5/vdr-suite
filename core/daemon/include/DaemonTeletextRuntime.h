#pragma once

#include <memory>
#include <vector>

class BackendAgentLifecycleService;
class BackendRegistryService;
class TeletextControlPlaneReadService;
class VdrSnapshotReadService;
struct BackendRuntimeContext;

bool configureDaemonTeletextRuntime(
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    BackendAgentLifecycleService& backendAgentLifecycleService,
    std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts);

void resetDaemonTeletextRuntime();

TeletextControlPlaneReadService* daemonTeletextControlPlaneReadService();

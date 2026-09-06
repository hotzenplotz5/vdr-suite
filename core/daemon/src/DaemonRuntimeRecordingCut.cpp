#include "DaemonRuntimeRecordingCut.h"

#include "BackendAgentCommandDelivery.h"
#include "DaemonRecordingCutReconciliation.h"
#include "SuiteBridgeRecordingCutStateResolver.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace
{
std::mutex recordingCutReconciliationMutex;
std::condition_variable recordingCutReconciliationCv;
bool recordingCutReconciliationStopRequested = false;
std::thread recordingCutReconciliationThread;
const std::vector<std::unique_ptr<BackendRuntimeContext>>*
    recordingCutReconciliationRuntimeContexts = nullptr;
BackendAgentCommandRepository* recordingCutReconciliationCommands = nullptr;

std::int64_t nowSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

SuiteBridgeRecordingCutStateResolver* recordingCutStateResolverForBackend(
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& runtimeContexts,
    const std::string& backendId)
{
    for (const auto& backendRuntimeContext : runtimeContexts)
    {
        if (!backendRuntimeContext || backendRuntimeContext->backendId != backendId)
            continue;
        if (!backendRuntimeContext->suiteBridgeAgentRuntime)
            return nullptr;

        const auto health = backendRuntimeContext->suiteBridgeAgentRuntime->health();
        if (!health.running || !health.observation.hasDiscovery)
            return nullptr;

        return backendRuntimeContext->ensureRecordingCutStateResolver();
    }
    return nullptr;
}

void reconcileRecordingCutsOnce(
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& runtimeContexts,
    BackendAgentCommandRepository& commands)
{
    const auto candidates = commands.recordingCutReconciliationCandidates();
    for (const auto& candidate : candidates)
    {
        SuiteBridgeRecordingCutStateResolver* const resolver =
            recordingCutStateResolverForBackend(
                runtimeContexts,
                candidate.assignment.backendId);
        if (resolver == nullptr) continue;

        const VdrRecordingNativeCutState state =
            resolver->resolve(candidate.recordingKey);
        if (!daemonRecordingCutResultMatches(
                candidate.recordingKey,
                candidate.editedRecordingKey,
                state))
        {
            continue;
        }

        BackendAgentRecordingCutVerification verification;
        std::string reasonCode;
        commands.verifyRecordingCutResult(
            candidate.assignment.commandId,
            candidate.assignment.requestFingerprint,
            candidate.recordingKey,
            candidate.expectedMarksRevision,
            candidate.editedRecordingKey,
            nowSeconds(),
            verification,
            reasonCode);
    }
}

void stopRecordingCutReconciliation()
{
    {
        std::lock_guard<std::mutex> lock(recordingCutReconciliationMutex);
        recordingCutReconciliationStopRequested = true;
    }
    recordingCutReconciliationCv.notify_all();
    if (recordingCutReconciliationThread.joinable())
        recordingCutReconciliationThread.join();

    std::lock_guard<std::mutex> lock(recordingCutReconciliationMutex);
    recordingCutReconciliationRuntimeContexts = nullptr;
    recordingCutReconciliationCommands = nullptr;
}

void startRecordingCutReconciliation(
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& runtimeContexts,
    BackendAgentCommandRepository& commands)
{
    stopRecordingCutReconciliation();
    {
        std::lock_guard<std::mutex> lock(recordingCutReconciliationMutex);
        recordingCutReconciliationStopRequested = false;
        recordingCutReconciliationRuntimeContexts = &runtimeContexts;
        recordingCutReconciliationCommands = &commands;
    }

    recordingCutReconciliationThread = std::thread([]() {
        std::unique_lock<std::mutex> lock(recordingCutReconciliationMutex);
        while (!recordingCutReconciliationStopRequested)
        {
            const auto* runtimeContexts =
                recordingCutReconciliationRuntimeContexts;
            BackendAgentCommandRepository* const commands =
                recordingCutReconciliationCommands;
            lock.unlock();
            if (runtimeContexts != nullptr && commands != nullptr)
                reconcileRecordingCutsOnce(*runtimeContexts, *commands);
            lock.lock();

            recordingCutReconciliationCv.wait_for(
                lock,
                std::chrono::seconds(1),
                []() { return recordingCutReconciliationStopRequested; });
        }
    });
}
}

bool configureDaemonRecordingCutRuntime(
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts,
    BackendAgentCommandRepository& backendAgentCommandRepository)
{
    if (!backendAgentCommandRepository.ensureRecordingCutReconciliationSchema())
        return false;
    startRecordingCutReconciliation(
        backendRuntimeContexts,
        backendAgentCommandRepository);
    return true;
}

void resetDaemonRecordingCutRuntime()
{
    stopRecordingCutReconciliation();
}

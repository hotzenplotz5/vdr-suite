#pragma once

#include <functional>
#include <memory>
#include <string>
#include <sys/types.h>
#include <vector>

enum class VdrRecordingIndexUpdateState
{
    NotStarted,
    Running,
    Succeeded,
    Failed
};

struct VdrRecordingIndexUpdateResult
{
    VdrRecordingIndexUpdateState state =
        VdrRecordingIndexUpdateState::NotStarted;
    std::string reasonCode;

    bool running() const
    {
        return state == VdrRecordingIndexUpdateState::Running;
    }

    bool succeeded() const
    {
        return state == VdrRecordingIndexUpdateState::Succeeded;
    }
};

class VdrRecordingIndexUpdater
{
public:
    using WorkerSpawner = std::function<pid_t(
        const std::string& recordingDirectory,
        uid_t ownerUid,
        gid_t ownerGid)>;
    using WorkerWaiter = std::function<int(pid_t pid)>;

    explicit VdrRecordingIndexUpdater(
        WorkerSpawner workerSpawner = {},
        WorkerWaiter workerWaiter = {});

    VdrRecordingIndexUpdateResult ensure(
        const std::string& recordingDirectory,
        const std::vector<std::string>& sourceSegments);

    VdrRecordingIndexUpdateResult status(
        const std::string& recordingDirectory) const;

private:
    struct SharedState;

    std::shared_ptr<SharedState> state_;
    WorkerSpawner workerSpawner_;
    WorkerWaiter workerWaiter_;

    static pid_t spawnAsOwner(
        const std::string& recordingDirectory,
        uid_t ownerUid,
        gid_t ownerGid);

    static int waitForWorker(pid_t pid);
};

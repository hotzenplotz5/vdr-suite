#pragma once

#include "LocalMediaShrinkAgentExecutor.h"
#include "MediaImport.h"
#include "MediaToolsImportRunLock.h"

#include <mutex>
#include <string>
#include <utility>

namespace vdrsuite::mediatools {

// Production-only guard around the isolated HEVC engine. It prevents a
// package maintainer from acquiring the same fixed exclusive lock while an
// HEVC stage/publish and its ffmpeg/vdr subprocesses are still executing.
// The guard never grants Core authorization, rights or installed provenance.
class ProtectedMediaShrinkAgentExecutor final : public IMediaShrinkAgentExecutor {
public:
    ProtectedMediaShrinkAgentExecutor(
        LocalMediaShrinkAgentExecutor& inner,
        NativeProcessRunner& runner,
        std::string trustedInstallRoot)
        : inner_(inner), runner_(runner),
          root_(std::move(trustedInstallRoot)) {}

    bool available(const std::string& commandType) override
    {
        std::lock_guard<std::mutex> guard(mutex_);
        return inner_.available(commandType) &&
            MediaToolsImportRunLock::tryAcquire(
                root_, MediaToolsImportRunLock::Mode::sharedExecution).has_value();
    }

    bool authorize(const BackendAgentCommandAssignment& assignment,
                   const std::string& backendId,
                   std::uint64_t generation) override
    {
        std::lock_guard<std::mutex> guard(mutex_);
        // Deny before accepting a command if the root-owned lock is missing,
        // unsafe or held exclusively by package maintenance.
        if (!inner_.available(assignment.commandType) ||
            !MediaToolsImportRunLock::tryAcquire(
                root_, MediaToolsImportRunLock::Mode::sharedExecution).has_value())
            return false;
        return inner_.authorize(assignment, backendId, generation);
    }

    MediaShrinkAgentOutcome execute(
        const BackendAgentCommandAssignment& assignment) override
    {
        std::lock_guard<std::mutex> guard(mutex_);
        auto shared = MediaToolsImportRunLock::tryAcquire(
            root_, MediaToolsImportRunLock::Mode::sharedExecution);
        if (!shared || shared->borrowedDescriptorForChild() < 0)
            return MediaShrinkAgentOutcome::rejected;

        // The SAME lock spans Core re-authorization, durable mutation fences,
        // encode/index/publish, and every fork/exec child. In the child the
        // inherited FD_CLOEXEC bit is cleared before execv; a parent crash
        // therefore does not free the lock while ffmpeg remains alive.
        runner_.bindMaintenanceLockFd(shared->borrowedDescriptorForChild());
        struct ResetRunner {
            NativeProcessRunner& runner;
            ~ResetRunner() { runner.bindMaintenanceLockFd(-1); }
        } reset{runner_};
        return inner_.execute(assignment);
    }

    std::string takeReviewId() override
    {
        std::lock_guard<std::mutex> guard(mutex_);
        return inner_.takeReviewId();
    }

private:
    LocalMediaShrinkAgentExecutor& inner_;
    NativeProcessRunner& runner_;
    const std::string root_;
    std::mutex mutex_;
};

} // namespace vdrsuite::mediatools

#pragma once

#include "LocalMediaShrinkReviewStore.h"
#include "MediaShrinkAgentExecutor.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace vdrsuite::mediatools {

// Authenticated, path-resolved facts issued by the Core-owned operation
// authority. These NEVER originate in command payloads or browser JSON.
// The Core callback must recheck the actor's exact mutation grant, reviewed
// addon activation, recording snapshot, live Agent lease and command fence.
struct LocalMediaShrinkAuthority {
    std::string actorId;
    std::string backendId;
    std::string operationId;
    std::string recordingId;
    std::string sourceFingerprint;
    std::uint64_t backendGeneration = 0;
    fs::path videoRoot;
    fs::path relativeRecording;
};

// Actual HEVC encoder / non-destructive publisher, behind an independent
// injected Core authorizer and the durable, opaque Agent-local review store.
// Construction does not activate an addon: the shipped Agent must explicitly
// install and review a trusted composition before injecting this executor.
class LocalMediaShrinkAgentExecutor final : public IMediaShrinkAgentExecutor {
public:
    using LiveCapability = std::function<bool(const std::string&)>;
    using CoreAuthorize = std::function<std::optional<LocalMediaShrinkAuthority>(
        const BackendAgentCommandAssignment&)>;
    using RunnerFactory = std::function<ProcessRunner&()>;

    LocalMediaShrinkAgentExecutor(
        LocalMediaShrinkReviewStore& reviews, LiveCapability available,
        CoreAuthorize authorize, RunnerFactory runner);

    bool available(const std::string& commandType) override;
    bool authorize(const BackendAgentCommandAssignment& assignment,
                   const std::string& backendId,
                   std::uint64_t generation) override;
    MediaShrinkAgentOutcome execute(
        const BackendAgentCommandAssignment& assignment) override;
    std::string takeReviewId() override;

private:
    LocalMediaShrinkReviewStore& reviews_;
    LiveCapability availability_;
    CoreAuthorize authorize_;
    RunnerFactory runner_;
    std::optional<BackendAgentCommandAssignment> assignment_;
    std::optional<LocalMediaShrinkAuthority> authority_;
    bool consumed_ = false;
    std::string reviewId_;
};
} // namespace vdrsuite::mediatools

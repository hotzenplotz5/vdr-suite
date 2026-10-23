#include "MediaShrinkCommandFence.h"
#include "MediaShrinkAgentExecutor.h"

#include <cassert>
#include <iostream>
#include <string>

using vdrsuite::addons::MediaShrinkCommandFenceResult;
using vdrsuite::addons::inspectMediaShrinkCommandFence;
using vdrsuite::operations::MutationOperation;
using vdrsuite::operations::MutationOperationPayload;
using vdrsuite::operations::MutationOperationState;
using vdrsuite::operations::MutationOperationVerificationPolicy;

namespace {
constexpr const char* rec = "rec_0123456789abcdef0123456789abcdef";
constexpr const char* review = "shk_abcdef0123456789abcdef0123456789";

struct Fixture {
    MutationOperation op;
    MutationOperationPayload payload;
    BackendAgentCommandAssignment command;

    Fixture() {
        op.operationId = "op_0123456789abcdef0123456789abcdef";
        op.actorId = "actor-a";
        op.backendId = "backend-a";
        op.backendGeneration = 7;
        op.resourceType = "Recording";
        op.resourceId = rec;
        op.actionFamily = "media.shrink.stage";
        op.expectedRevision = "rec-rev-1";
        op.expectedResourceFingerprint = "source_fingerprint_1";
        op.requestFingerprint = "request_fingerprint_1";
        op.verificationPolicy =
            MutationOperationVerificationPolicy::readbackRequired;
        op.state = MutationOperationState::dispatching;
        op.requestedAt = 100;
        op.deadline = 2000;
        payload.operationId = op.operationId;
        payload.payloadType =
            vdrsuite::mediatools::kMediaShrinkStageCommandType;
        payload.payloadVersion = 1;
        payload.payload =
            std::string("{\"recordingId\":\"") + rec +
            "\",\"encoder\":\"software\",\"maxHeight\":720,\"nightMode\":false}";
        payload.payloadFingerprint = op.requestFingerprint;

        command.present = true;
        command.operationId = op.operationId;
        command.backendId = op.backendId;
        command.backendGeneration = op.backendGeneration;
        command.commandType = payload.payloadType;
        command.payloadVersion = 1;
        command.payload = payload.payload;
        command.verificationPolicy = "readback_required";
        command.commandId = "cmd_a";
        command.agentId = "agent-a";
        command.agentInstanceId = "agent-instance-a";
        command.jobId = "job-a";
        command.attemptId = "att-a";
        command.claimEpoch = 1;
        command.requestFingerprint = "fp1_dummy";
        command.assignedAt = 110;
        command.deadline = 500;
    }
    MediaShrinkCommandFenceResult inspect() const {
        return inspectMediaShrinkCommandFence(op, payload, command, 150);
    }
    void publish() {
        op.actionFamily = "media.shrink.publish";
        op.resourceType = "MediaShrinkReview";
        op.resourceId = review;
        op.expectedRevision = "1";
        op.verificationPolicy =
            MutationOperationVerificationPolicy::reconciliationRequired;
        payload.payloadType =
            vdrsuite::mediatools::kMediaShrinkPublishCommandType;
        payload.payload = std::string("{\"reviewId\":\"") +
            review + "\",\"revision\":1}";
        command.commandType = payload.payloadType;
        command.payload = payload.payload;
        command.verificationPolicy = "reconciliation_required";
    }
};
}

int main() {
    using Status = MediaShrinkCommandFenceResult;
    Fixture f;
    assert(f.inspect() == Status::candidateOnly);
    auto changed = f;
    changed.op.actorId.clear();
    assert(changed.inspect() == Status::operationMismatch);
    changed = f;
    changed.op.backendGeneration = 8;
    assert(changed.inspect() == Status::staleAuthority);
    changed = f;
    changed.op.state = MutationOperationState::succeeded;
    assert(changed.inspect() == Status::operationMismatch);
    changed = f;
    changed.op.resultReference = "shk_existing";
    assert(changed.inspect() == Status::operationMismatch);
    changed = f;
    changed.command.agentInstanceId.clear();
    assert(changed.inspect() == Status::invalidAssignment);
    changed = f;
    changed.command.deadline = 2100;
    assert(changed.inspect() == Status::staleAuthority);
    changed = f;
    changed.command.payload += "garbage";
    assert(changed.inspect() == Status::payloadMismatch);
    changed = f;
    changed.payload.payload += "garbage";
    changed.command.payload = changed.payload.payload;
    assert(changed.inspect() == Status::payloadMismatch);
    changed = f;
    changed.command.payload =
        "{\"recordingId\":\"../../etc/passwd\",\"encoder\":\"software\","
        "\"maxHeight\":720,\"nightMode\":false}";
    changed.payload.payload = changed.command.payload;
    assert(changed.inspect() == Status::payloadMismatch);
    changed = f;
    changed.command.payloadVersion = 2;
    assert(changed.inspect() == Status::invalidAssignment);
    changed = f;
    changed.payload.payloadFingerprint = "another_immutable_payload_fingerprint";
    assert(changed.inspect() == Status::candidateOnly);
    changed.payload.payloadFingerprint.clear();
    assert(changed.inspect() == Status::payloadMismatch);

    assert(vdrsuite::mediatools::validShrinkReviewEvidence(
        f.command, review));
    assert(!vdrsuite::mediatools::validShrinkReviewEvidence(
        f.command, "/tmp/fake.rec"));
    f.publish();
    assert(f.inspect() == Status::candidateOnly);
    assert(vdrsuite::mediatools::validShrinkReviewEvidence(
        f.command, review));
    assert(!vdrsuite::mediatools::validShrinkReviewEvidence(
        f.command, "shk_0123456789abcdef0123456789abcdef"));

    changed = f;
    changed.op.expectedRevision = "2";
    assert(changed.inspect() == Status::operationMismatch);
    changed = f;
    changed.op.backendId = "backend-b";
    assert(changed.inspect() == Status::operationMismatch);
    changed = f;
    changed.command.verificationPolicy = "readback_required";
    assert(changed.inspect() == Status::operationMismatch);
    changed = f;
    changed.command.payload =
        std::string("{\"reviewId\":\"") + review + "\",\"revision\":2}";
    changed.payload.payload = changed.command.payload;
    assert(changed.inspect() == Status::operationMismatch);
    changed = f;
    changed.command.payload =
        std::string("{\"reviewId\":\"") + review +
        "\",\"revision\":1,\"path\":\"/tmp/evil\"}";
    changed.payload.payload = changed.command.payload;
    assert(changed.inspect() == Status::payloadMismatch);
    changed = f;
    changed.op.state = MutationOperationState::outcomeUnknown;
    assert(changed.inspect() == Status::operationMismatch);

    std::cout << "MEDIA_SHRINK_CANONICAL_AGENT_FENCE=PASS\n";
}

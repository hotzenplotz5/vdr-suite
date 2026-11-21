#include "SearchTimerNativeCasContract.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
constexpr const char* kGen =
    "sg_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
constexpr const char* kRevision =
    "sr_bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
constexpr const char* kNewRevision =
    "sr_cccccccccccccccccccccccccccccccc";

SearchTimerNativeCasRequest request()
{
    SearchTimerNativeCasRequest value;
    value.action = SearchTimerNativeCasAction::activate;
    value.operationId = "op_11111111111111111111111111111111";
    value.backendId = "default";
    value.publicSearchTimerId = "st_22222222222222222222222222222222";
    value.nativeId = "0";
    value.expectedGeneration = kGen;
    value.expectedRevision = kRevision;
    value.requestFingerprint = "sha256:" + std::string(64U, 'd');
    value.durableOperationClaimed = true;
    value.backendWriteAuthorized = true;
    value.operatorConfirmed = true;
    return value;
}

struct FakeNative : ISearchTimerNativeCasProvider
{
    bool hasCapability = true;
    bool throwOnCapability = false;
    bool throwOnWrite = false;
    int attempts = 0;
    SearchTimerNativeCasReply reply;

    bool supportsAtomicGenerationCasV1() const override
    {
        if (throwOnCapability) throw std::runtime_error("unavailable");
        return hasCapability;
    }

    SearchTimerNativeCasReply compareAndWrite(
        const SearchTimerNativeCasRequest& req) override
    {
        ++attempts;
        if (throwOnWrite) throw std::runtime_error("lost reply");
        assert(req.nativeId == "0");
        return reply;
    }

    void success()
    {
        reply.status = SearchTimerNativeCasStatus::applied;
        reply.confirmedGeneration = kGen;
        reply.resultingRevision = kNewRevision;
        reply.atomicCompareAndWrite = true;
        reply.noWriteAttested = false;
    }
};

void assertNoUnsafeDispatch()
{
    FakeNative native;
    native.success();
    native.hasCapability = false;
    auto req = request();
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::unsupported);
    assert(native.attempts == 0);

    native.hasCapability = true;
    req.backendWriteAuthorized = false;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::denied);
    req = request();
    req.operatorConfirmed = false;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::denied);
    req = request();
    req.durableOperationClaimed = false;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::denied);
    assert(native.attempts == 0);

    req = request(); req.expectedGeneration.clear();
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::invalid);
    req = request(); req.expectedRevision.clear();
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::invalid);
    req = request(); req.publicSearchTimerId = "0";
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::invalid);
    req = request(); req.nativeId = "00";
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::invalid);
    req = request(); req.nativeId = "2147483648";
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::invalid);
    req = request(); req.changePayload = "ignored options";
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::invalid);
    req = request(); req.backendId = "other/backend";
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::invalid);
    assert(native.attempts == 0);
}

void assertNativeCasAttestation()
{
    FakeNative native;
    native.success();
    auto req = request();
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::applied);
    assert(native.attempts == 1);

    // Native numeric ID zero can represent an identically configured
    // replacement rule. The provider alone decides under its rule lock.
    native.reply.status = SearchTimerNativeCasStatus::staleGeneration;
    native.reply.confirmedGeneration = "sg_eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
    native.reply.resultingRevision.clear();
    // The negative status by itself does not prove zero native mutation.
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::outcomeUnknown);
    native.reply.noWriteAttested = true;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::staleGeneration);
    assert(native.attempts == 3);

    native.reply.status = SearchTimerNativeCasStatus::staleRevision;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::staleRevision);
    native.reply.status = SearchTimerNativeCasStatus::notFound;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::notFound);
    // Contradictory no-write attestation carrying a resulting revision.
    native.reply.resultingRevision = kNewRevision;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::outcomeUnknown);

    native.success();
    native.reply.noWriteAttested = true;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::outcomeUnknown);
    native.success();
    native.reply.confirmedGeneration =
        "sg_ffffffffffffffffffffffffffffffff";
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::outcomeUnknown);
    native.success();
    native.reply.resultingRevision = kRevision;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::outcomeUnknown);
    native.success();
    native.reply.atomicCompareAndWrite = false;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::outcomeUnknown);

    native.success();
    native.throwOnWrite = true;
    const int before = native.attempts;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::outcomeUnknown);
    assert(native.attempts == before + 1); // never retries
}

void assertActionsAndCapabilities()
{
    FakeNative native;
    native.success();
    auto req = request();
    req.action = SearchTimerNativeCasAction::remove;
    native.reply.resultingRevision.clear();
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::applied);
    req.action = SearchTimerNativeCasAction::deactivate;
    native.reply.resultingRevision = kNewRevision;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::applied);

    req.action = SearchTimerNativeCasAction::update;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::invalid);
    req.changePayload = "opaque durable payload";
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::applied);
    req.action = static_cast<SearchTimerNativeCasAction>(100);
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::invalid);

    native.throwOnCapability = true;
    req = request();
    const int before = native.attempts;
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::unavailable);
    assert(native.attempts == before);
}

struct StatefulNative final : ISearchTimerNativeCasProvider
{
    // This is an isolated model of the required epgsearch-owner lock, NOT
    // a production epgsearch provider. It uses native numeric ID 0.
    std::string generation = kGen;
    std::string revision = kRevision;
    std::string ruleText = "identical rule";
    bool exists = true;
    bool active = false;
    bool dropReceiptAfterWrite = false;
    int writes = 0;

    bool supportsAtomicGenerationCasV1() const override { return true; }

    void recreateWithIdenticalContent()
    {
        // Same native ID, revision and settings: only a persistent native
        // generation distinguishes the new rule from the deleted one.
        exists = true;
        generation = "sg_eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee";
        revision = kRevision;
        ruleText = "identical rule";
    }

    SearchTimerNativeCasReply compareAndWrite(
        const SearchTimerNativeCasRequest& req) override
    {
        SearchTimerNativeCasReply reply;
        reply.atomicCompareAndWrite = true;
        reply.confirmedGeneration = generation;
        if (!exists || req.nativeId != "0")
        {
            reply.status = SearchTimerNativeCasStatus::notFound;
            reply.noWriteAttested = true;
            return reply;
        }
        if (req.expectedGeneration != generation)
        {
            reply.status = SearchTimerNativeCasStatus::staleGeneration;
            reply.noWriteAttested = true;
            return reply;
        }
        if (req.expectedRevision != revision)
        {
            reply.status = SearchTimerNativeCasStatus::staleRevision;
            reply.noWriteAttested = true;
            return reply;
        }
        ++writes;
        if (req.action == SearchTimerNativeCasAction::remove)
            exists = false;
        else
        {
            if (req.action == SearchTimerNativeCasAction::update)
                ruleText = req.changePayload;
            else
                active = req.action == SearchTimerNativeCasAction::activate;
            revision = kNewRevision;
            reply.resultingRevision = revision;
        }
        reply.status = SearchTimerNativeCasStatus::applied;
        if (dropReceiptAfterWrite)
            throw std::runtime_error("native applied, transport reply lost");
        return reply;
    }
};

void assertSameNumericIdAndIdenticalRuleReuse()
{
    StatefulNative native;
    auto req = request();
    native.recreateWithIdenticalContent();
    const SearchTimerNativeCasAction actions[] = {
        SearchTimerNativeCasAction::update,
        SearchTimerNativeCasAction::activate,
        SearchTimerNativeCasAction::deactivate,
        SearchTimerNativeCasAction::remove,
    };
    for (const auto action : actions)
    {
        req.action = action;
        req.changePayload = action == SearchTimerNativeCasAction::update
            ? "replacement settings" : "";
        assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
            SearchTimerNativeCasStatus::staleGeneration);
        assert(native.writes == 0);
        assert(native.exists);
        assert(native.ruleText == "identical rule");
        assert(!native.active);
    }
}

void assertNativeStaleRevisionAndLostAppliedReceipt()
{
    StatefulNative native;
    auto req = request();
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::applied);
    assert(native.writes == 1);
    assert(native.active);

    // Even a second attempt with the same native generation cannot
    // silently apply against a revised rule.
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::staleRevision);
    assert(native.writes == 1);

    // An independent native/OSD edit also invalidates an old revision.
    req.expectedRevision = kNewRevision;
    native.revision = "sr_ffffffffffffffffffffffffffffffff";
    assert(SearchTimerNativeCasContract::invokeOnce(req, native) ==
        SearchTimerNativeCasStatus::staleRevision);
    assert(native.writes == 1);

    StatefulNative lostReply;
    lostReply.dropReceiptAfterWrite = true;
    req = request();
    assert(SearchTimerNativeCasContract::invokeOnce(req, lostReply) ==
        SearchTimerNativeCasStatus::outcomeUnknown);
    // The attempted native write may have succeeded: no automatic retry.
    assert(lostReply.writes == 1);
    assert(lostReply.active);
}

} // namespace

int main()
{
    assertNoUnsafeDispatch();
    assertNativeCasAttestation();
    assertActionsAndCapabilities();
    assertSameNumericIdAndIdenticalRuleReuse();
    assertNativeStaleRevisionAndLostAppliedReceipt();
    std::cout << "SearchTimer native generation CAS contract: PASS (mock only)\n";
}

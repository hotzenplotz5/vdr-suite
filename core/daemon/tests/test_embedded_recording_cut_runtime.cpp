#include "EmbeddedRecordingCutRuntime.h"

#include "BackendAgentRecordingCut.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <map>

using namespace vdrsuite::agent;

namespace
{
const std::string sourceKey(32, 'a');
const std::string marksRevision(32, 'b');
const std::string editedKey(32, 'd');

struct Resolver final :
    IVdrRecordingNativeCutStateResolver
{
    bool complete = false;

    VdrRecordingNativeCutState resolve(
        const std::string& requested) override
    {
        VdrRecordingNativeCutState state;
        state.availability =
            VdrRecordingNativeCutStateAvailability::
                Available;
        state.found = true;
        state.recordingKey = requested;
        state.marksReadable = true;
        state.marksRevision = marksRevision;
        state.marksFilePresent = true;
        state.markCount = 2;
        state.sequenceCount = 1;
        state.editedRecordingKey = editedKey;

        if (complete)
        {
            state.ready = false;
            state.reason =
                "edited-destination-exists";
            state.editedDestinationExists = true;
            state.editedRecordingFound = true;
        }
        else
        {
            state.ready = true;
            state.reason = "ready";
        }

        return state;
    }
};

struct Transport final :
    IBackendAgentRecordingCutTransport
{
    bool available = true;
    bool reject = false;
    bool loseReply = false;
    bool stale = false;

    int calls = 0;
    int effects = 0;

    std::map<
        std::string,
        BackendAgentRecordingCutTransportReply>
        ledger;

    bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string&) override
    {
        facts = {
            kBackendAgentRecordingCutProviderId,
            kBackendAgentRecordingCutProviderKind,
            "epoch-1",
            1,
            2,
            available,
            {kBackendAgentRecordingCutCapability}};

        return available;
    }

    BackendAgentRecordingCutTransportReply
    startCut(
        const BackendAgentRecordingCutTransportRequest&
            request) override
    {
        ++calls;

        if (stale)
        {
            return {
                BackendAgentRecordingCutTransportDisposition::
                    rejectedWithoutEffect,
                "ncut:vdr:stale:" +
                    request.command.commandId};
        }

        std::string reason;

        assert(
            backendAgentRecordingCutValidCommand(
                request.command,
                reason));

        assert(
            request.localStartingPersistedAt > 0);

        assert(
            request.command.requestFingerprint.rfind(
                "fp1_",
                0) == 0);

        const auto existing =
            ledger.find(
                request.command.commandId);

        if (existing != ledger.end())
            return existing->second;

        BackendAgentRecordingCutTransportReply reply;

        if (reject)
        {
            reply.disposition =
                BackendAgentRecordingCutTransportDisposition::
                    rejectedWithoutEffect;

            reply.evidenceReference =
                "ncut:vdr:cut-queue-rejected:" +
                request.command.commandId;
        }
        else
        {
            ++effects;

            reply.disposition =
                BackendAgentRecordingCutTransportDisposition::
                    acceptedUnverified;

            reply.evidenceReference =
                "ncut:vdr:queued:" +
                editedKey + ":" +
                request.command.commandId;
        }

        ledger.emplace(
            request.command.commandId,
            reply);

        if (loseReply)
            return {};

        return reply;
    }
};

RecordingCutStartRequest request(
    const std::string& operationId)
{
    RecordingCutStartRequest value;
    value.backendId = "default";
    value.operationId = operationId;
    value.operationRevision = "1";
    value.recordingKey = sourceKey;
    value.expectedMarksRevision =
        marksRevision;

    return value;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));

    Resolver resolver;
    Transport transport;

    EmbeddedRecordingCutRuntime runtime(
        database,
        "default",
        transport,
        resolver);

    assert(runtime.ensureSchema());

    auto cut = request("cut-one");

    auto result =
        runtime.dispatch(cut);

    assert(result.accepted);
    assert(!result.replayed);
    assert(!result.verified);
    assert(transport.calls == 1);
    assert(transport.effects == 1);

    result = runtime.dispatch(cut);

    assert(result.accepted);
    assert(result.replayed);
    assert(!result.verified);
    assert(transport.calls == 1);
    assert(transport.effects == 1);

    resolver.complete = true;

    result = runtime.dispatch(cut);

    assert(result.accepted);
    assert(result.replayed);
    assert(result.verified);
    assert(result.editedRecordingKey == editedKey);
    assert(transport.calls == 1);

    auto conflict = cut;
    conflict.expectedMarksRevision =
        std::string(32, 'c');

    assert(
        runtime.dispatch(conflict).reasonCode ==
        "recording_cut_assignment_conflict");

    assert(transport.calls == 1);

    auto probe =
        request("missing-replay");

    probe.replayOnly = true;

    assert(
        runtime.dispatch(probe).reasonCode ==
        "recording_cut_assignment_not_found");

    resolver.complete = false;
    transport.available = false;

    assert(
        !runtime.dispatch(
            request("no-provider")).accepted);

    transport.available = true;
    transport.reject = true;

    auto rejected =
        request("rejected");

    assert(
        runtime.dispatch(rejected).reasonCode ==
        "recording_cut_rejected");

    const int rejectedCalls =
        transport.calls;

    assert(
        !runtime.dispatch(rejected).accepted);

    assert(
        transport.calls ==
        rejectedCalls);

    transport.reject = false;
    transport.loseReply = true;

    auto lost =
        request("lost-reply");

    result =
        runtime.dispatch(lost);

    assert(result.accepted);
    assert(!result.verified);

    const int effectsAfterLostReply =
        transport.effects;

    transport.loseReply = false;
    resolver.complete = true;

    {
        EmbeddedRecordingCutRuntime restarted(
            database,
            "default",
            transport,
            resolver);

        result =
            restarted.dispatch(lost);

        assert(result.accepted);
        assert(result.replayed);
        assert(result.verified);

        assert(
            transport.effects ==
            effectsAfterLostReply);
    }

    resolver.complete = false;
    transport.loseReply = true;

    auto uncertain =
        request("provider-restart");

    result =
        runtime.dispatch(uncertain);

    assert(result.accepted);
    assert(!result.verified);

    transport.loseReply = false;
    transport.stale = true;

    result =
        runtime.dispatch(uncertain);

    assert(result.accepted);
    assert(result.replayed);
    assert(!result.verified);

    const int uncertainCalls =
        transport.calls;

    result =
        runtime.dispatch(uncertain);

    assert(result.accepted);
    assert(!result.verified);

    assert(
        transport.calls ==
        uncertainCalls);

    transport.stale = false;

    const std::string filename =
        ".build/embedded-cut-restart-test.db";

    std::remove(filename.c_str());

    {
        Database disk;
        assert(disk.open(filename));

        Resolver diskResolver;
        Transport diskTransport;

        EmbeddedRecordingCutRuntime diskRuntime(
            disk,
            "default",
            diskTransport,
            diskResolver);

        assert(diskRuntime.ensureSchema());

        auto diskRequest =
            request("disk-restart");

        assert(
            diskRuntime.dispatch(
                diskRequest).accepted);

        assert(diskTransport.effects == 1);
    }

    {
        Database reopened;
        assert(reopened.open(filename));

        Resolver diskResolver;
        diskResolver.complete = true;

        Transport diskTransport;

        EmbeddedRecordingCutRuntime recovered(
            reopened,
            "default",
            diskTransport,
            diskResolver);

        auto diskRequest =
            request("disk-restart");

        const auto recoveredResult =
            recovered.dispatch(
                diskRequest);

        assert(recoveredResult.accepted);
        assert(recoveredResult.replayed);
        assert(recoveredResult.verified);

        /*
         * Persisted accepted state reconciles through VDR
         * readback. No second native cut may be issued.
         */
        assert(diskTransport.effects == 0);

        reopened.close();
    }

    std::remove(filename.c_str());

    assert(database.execute(
        "DROP TABLE "
        "embedded_recording_cut_commands;"));

    assert(
        !runtime.dispatch(
            request("database-failure")).accepted);

    std::cout
        << "embedded cut: durable journal, exact replay, "
           "lost-reply recovery, restart readback and "
           "fail-closed provider restart ok\n";
}

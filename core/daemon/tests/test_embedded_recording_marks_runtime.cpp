#include "EmbeddedRecordingMarksRuntime.h"
#include "BackendAgentRecordingMarksModify.h"
#include "VdrRecordingNativeIdentity.h"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>

using namespace vdrsuite::agent;
const std::string key(32, 'a'), before(32, 'b'), after(32, 'c');
struct Resolver final : IVdrRecordingNativeMarksResolver
{
    std::string revision = before;
    VdrRecordingNativeMarks resolve(const std::string& requested) override
    {
        VdrRecordingNativeMarks marks;
        marks.recordingKey = requested;
        marks.found = true;
        marks.availability = VdrRecordingNativeMarksAvailability::Available;
        marks.marksRevision = revision;
        marks.framesPerSecond = 25;
        return marks;
    }
};
struct Transport final : IBackendAgentRecordingMarksModifyTransport
{
    Resolver& resolver;
    bool available = true, reject = false, loseReply = false, mismatch = false;
    int calls = 0, effects = 0;
    bool stale = false;
    std::map<std::string, BackendAgentRecordingMarksModifyTransportReply> ledger;
    std::string firstCommand;
    explicit Transport(Resolver& r) : resolver(r) {}
    bool discoverProvider(BackendAgentLocalProviderFacts& f, std::string&) override
    {
        f = {kBackendAgentRecordingMarksModifyProviderId,
            kBackendAgentRecordingMarksModifyProviderKind, "epoch-1", 1, 2,
            available, {kBackendAgentRecordingMarksModifyCapability}};
        return available;
    }
    BackendAgentRecordingMarksModifyTransportReply modifyMarks(
        const BackendAgentRecordingMarksModifyTransportRequest& r) override
    {
        ++calls;
        if (stale) return {BackendAgentRecordingMarksModifyTransportDisposition::rejectedWithoutEffect,
            "nmarks:stale:" + r.command.commandId};
        std::string reason;
        assert(backendAgentRecordingMarksModifyValidCommand(r.command, reason));
        assert(r.localStartingPersistedAt > 0);
        assert(r.command.requestFingerprint.rfind("fp1_", 0) == 0);
        if (ledger.count(r.command.commandId)) return ledger.at(r.command.commandId);
        firstCommand = r.command.commandId;
        BackendAgentRecordingMarksModifyTransportReply reply;
        reply.disposition = reject ? BackendAgentRecordingMarksModifyTransportDisposition::rejectedWithoutEffect
            : BackendAgentRecordingMarksModifyTransportDisposition::acceptedUnverified;
        reply.evidenceReference = "nmarks:vdr:postrev:" + after + ":" + r.command.commandId;
        ledger[r.command.commandId] = reply;
        if (!reject) { ++effects; resolver.revision = mismatch ? before : after; }
        if (loseReply) return {};
        return reply;
    }
};
RecordingMarksMutationRequest request(const std::string& id)
{
    RecordingMarksMutationRequest r;
    r.backendId = "default"; r.operationId = id; r.operationRevision = "1";
    r.recordingKey = key; r.expectedMarksRevision = before;
    r.targetFrame = 250;
    return r;
}
int main()
{
    Database db;
    assert(db.open(":memory:"));
    Resolver resolver;
    Transport transport(resolver);
    EmbeddedRecordingMarksRuntime runtime(db, "default", transport, resolver);
    assert(runtime.ensureSchema());
    auto r = request("add-one");
    auto result = runtime.dispatch(r);
    assert(result.accepted && result.verified && !result.replayed && transport.effects == 1);
    result = runtime.dispatch(r);
    assert(result.verified && result.replayed && transport.calls == 1);
    r.targetFrame = 300;
    assert(runtime.dispatch(r).reasonCode == "recording_marks_modify_assignment_conflict");
    assert(transport.calls == 1);
    r = request("unknown-probe"); r.replayOnly = true;
    assert(runtime.dispatch(r).reasonCode == "recording_marks_modify_assignment_not_found");
    assert(transport.calls == 1);
    r = request("wrong-backend"); r.backendId = "another";
    assert(!runtime.dispatch(r).accepted && transport.calls == 1);
    transport.available = false;
    assert(!runtime.dispatch(request("no-provider")).accepted && transport.calls == 1);
    transport.available = true; transport.reject = true;
    r = request("rejected");
    assert(runtime.dispatch(r).reasonCode == "recording_marks_modify_rejected");
    int calls = transport.calls;
    assert(!runtime.dispatch(r).accepted && transport.calls == calls);
    transport.reject = false; transport.mismatch = true;
    r = request("wrong-readback");
    result = runtime.dispatch(r);
    assert(result.accepted && !result.verified);
    calls = transport.calls;
    result = runtime.dispatch(r);
    assert(!result.verified && transport.calls == calls);
    resolver.revision = after;
    assert(runtime.dispatch(r).verified && transport.calls == calls);
    transport.mismatch = false; transport.loseReply = true;
    r = request("lost-reply");
    result = runtime.dispatch(r);
    assert(result.accepted && !result.verified);
    int effects = transport.effects;
    {
        // Recreate the owner over its durable journal, as after daemon restart.
        EmbeddedRecordingMarksRuntime restarted(db, "default", transport, resolver);
        result = restarted.dispatch(r);
        assert(result.verified && result.replayed && transport.effects == effects);
    }
    transport.loseReply = false;
    {
        const std::string filename = ".build/embedded-marks-restart-test.db";
        std::remove(filename.c_str());
        auto crashRequest = request("disk-restart");
        {
            Database disk;
            assert(disk.open(filename));
            EmbeddedRecordingMarksRuntime diskRuntime(disk, "default", transport, resolver);
            assert(diskRuntime.ensureSchema());
            transport.loseReply = true;
            assert(!diskRuntime.dispatch(crashRequest).verified);
        }
        transport.loseReply = false;
        Database reopened;
        assert(reopened.open(filename));
        EmbeddedRecordingMarksRuntime recovered(reopened, "default", transport, resolver);
        assert(recovered.dispatch(crashRequest).verified);
        crashRequest.operationId = "vdr-restarted";
        transport.loseReply = true;
        assert(!recovered.dispatch(crashRequest).verified);
        transport.stale = true;
        result = recovered.dispatch(crashRequest);
        assert(result.accepted && !result.verified);
        calls = transport.calls;
        result = recovered.dispatch(crashRequest);
        assert(result.accepted && !result.verified && transport.calls == calls);
        transport.stale = false; transport.loseReply = false;
        reopened.close();
        std::remove(filename.c_str());
    }
    for (auto kind : {RecordingMarksMutationKind::Delete, RecordingMarksMutationKind::Move,
            RecordingMarksMutationKind::Reset, RecordingMarksMutationKind::Replace})
    {
        r = request("kind-" + std::to_string(static_cast<int>(kind)));
        r.kind = kind; r.targetFrame = -1;
        if (kind == RecordingMarksMutationKind::Delete || kind == RecordingMarksMutationKind::Move)
            r.sourceFrame = 250;
        if (kind == RecordingMarksMutationKind::Move) r.targetFrame = 500;
        if (kind == RecordingMarksMutationKind::Replace) r.replacementFrames = {100, 200};
        assert(runtime.dispatch(r).verified);
    }
    r = request("parallel");
    effects = transport.effects;
    std::thread a([&] { assert(runtime.dispatch(r).verified); });
    std::thread b([&] { assert(runtime.dispatch(r).verified); });
    a.join(); b.join();
    assert(transport.effects == effects + 1);

    // Production API -> embedded dispatcher -> native provider -> readback.
    VdrRecording recording;
    recording.id = "7"; recording.backendId = "default"; recording.backendNativeId = "native-test-recording";
    auto& api = RecordingMarksApiRuntime::instance();
    assert(api.configure([&](const std::string&) { return std::vector<VdrRecording>{recording}; },
        [&](const std::string&) { return RecordingMarksBackendAccess{RecordingMarksBackendAvailability::Available, &resolver}; },
        [](const std::string&) { RecordingMarksBackendWriteAccess a; a.allowed = true; return a; },
        [&](const RecordingMarksMutationRequest& req) { return runtime.dispatch(req); }));
    resolver.revision = before;
    const std::string body = "{\"backendId\":\"default\",\"recordingId\":\"7\",\"operationId\":\"api-one\","
        "\"operationRevision\":\"1\",\"expectedMarksRevision\":\"" + before + "\",\"kind\":\"add\",\"targetFrame\":250}";
    ApiResponse response;
    assert(api.tryHandlePost("/api/vdr/recordings/marks", body, response));
    assert(response.statusCode == 200 && response.body.find("\"verification\":\"verified\"") != std::string::npos);
    calls = transport.calls;
    assert(api.tryHandlePost("/api/vdr/recordings/marks", body, response));
    assert(response.statusCode == 200 && transport.calls == calls);
    api.reset();
    assert(db.execute("DROP TABLE embedded_recording_marks_commands;"));
    assert(!runtime.dispatch(request("database-failure")).accepted && transport.calls == calls);
    std::cout << "embedded marks: native dispatch, immediate verification, exact replay, conflict, restart, lost reply, concurrency, API and failure gates ok\n";
}

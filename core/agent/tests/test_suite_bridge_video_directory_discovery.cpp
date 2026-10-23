#include "SuiteBridgeVideoDirectoryDiscovery.h"

#include <cassert>
#include <string>

using namespace vdrsuite::agent;

namespace
{
class StubTransport final : public ISuiteBridgeLocalTransport
{
public:
    SuiteBridgeCommandReply reply;
    unsigned int calls = 0;
    SuiteBridgeLocalCommand lastCommand = SuiteBridgeLocalCommand::Snapshot;

    SuiteBridgeCommandReply execute(SuiteBridgeLocalCommand command) override
    {
        ++calls;
        lastCommand = command;
        return reply;
    }
};

void testNativeRoot()
{
    StubTransport transport;
    transport.reply = {SuiteBridgeTransportStatus::Success, 900, "/data/tv/", {}};
    const auto result = SuiteBridgeVideoDirectoryDiscovery::discover(transport);
    assert(transport.calls == 1);
    assert(transport.lastCommand == SuiteBridgeLocalCommand::VideoDirectory);
    assert(result.authoritativeReply);
    assert(result.videoDirectory == "/data/tv");
}

void testLegacyAndOffline()
{
    StubTransport transport;
    transport.reply = {SuiteBridgeTransportStatus::Success, 500, "unknown", {}};
    auto result = SuiteBridgeVideoDirectoryDiscovery::discover(transport);
    assert(!result.authoritativeReply && result.videoDirectory.empty());

    transport.reply = {SuiteBridgeTransportStatus::Timeout, 0, {}, "timeout"};
    result = SuiteBridgeVideoDirectoryDiscovery::discover(transport);
    assert(!result.authoritativeReply && result.videoDirectory.empty());
}

void testMaliciousOrInvalidNativeReplyFailsClosed()
{
    const std::string invalid[] = {
        "", "/", "data/tv", "/data/../etc", "/data/tv\nspoofed",
        "/data/tv\rspoofed", std::string("/data/tv\0spoofed", sizeof("/data/tv\0spoofed") - 1U), "/data/tv\tbad",
        "/data/tv\x7f" "bad", std::string(4097, 'x')
    };
    StubTransport transport;
    for (const auto& path : invalid)
    {
        transport.reply = {SuiteBridgeTransportStatus::Success, 900, path, {}};
        const auto result = SuiteBridgeVideoDirectoryDiscovery::discover(transport);
        assert(result.authoritativeReply);
        assert(result.videoDirectory.empty());
    }
    assert(SuiteBridgeVideoDirectoryDiscovery::validRoot(
        "/mnt/storage/recordings/") == "/mnt/storage/recordings");
}
}

int main()
{
    testNativeRoot();
    testLegacyAndOffline();
    testMaliciousOrInvalidNativeReplyFailsClosed();
    return 0;
}

#include "SuiteBridgeMetadataReplyCache.h"

#include <cassert>
#include <chrono>
#include <string>
#include <thread>

using namespace vdrsuite::agent;

namespace
{
SuiteBridgeMetadataCommandReply successfulReply(const std::string& payload)
{
    SuiteBridgeMetadataCommandReply reply;
    reply.transportSucceeded = true;
    reply.replyCode = 250;
    reply.payload = payload;
    return reply;
}
}

int main()
{
    SuiteBridgeMetadataCommandReply cached;
    SuiteBridgeMetadataReplyCache cache(2, std::chrono::milliseconds(40));

    cache.store("channel-a", "100", successfulReply("event-100"));
    assert(cache.find("channel-a", "100", cached));
    assert(cached.payload == "event-100");
    assert(!cache.find("channel-a", "101", cached));
    assert(!cache.find("channel-b", "100", cached));

    SuiteBridgeMetadataCommandReply failed;
    failed.replyCode = 451;
    failed.payload = "failed";
    cache.store("channel-a", "failed", failed);
    assert(!cache.find("channel-a", "failed", cached));

    cache.store("channel-a", "101", successfulReply("event-101"));
    assert(cache.find("channel-a", "100", cached));
    cache.store("channel-a", "102", successfulReply("event-102"));

    assert(cache.size() == 2);
    assert(cache.find("channel-a", "100", cached));
    assert(!cache.find("channel-a", "101", cached));
    assert(cache.find("channel-a", "102", cached));

    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    assert(!cache.find("channel-a", "100", cached));
    assert(!cache.find("channel-a", "102", cached));

    SuiteBridgeMetadataReplyCache disabled(0, std::chrono::seconds(5));
    disabled.store("channel-a", "200", successfulReply("disabled"));
    assert(!disabled.find("channel-a", "200", cached));

    return 0;
}

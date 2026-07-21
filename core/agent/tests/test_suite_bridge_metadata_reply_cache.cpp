#include "SuiteBridgeMetadataReplyCache.h"

#include <cassert>
#include <chrono>
#include <string>
#include <thread>

using namespace vdrsuite::agent;

namespace
{

SuiteBridgeMetadataCommandReply successfulReply(
    const std::string& payload)
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

    SuiteBridgeMetadataReplyCache cache(
        2,
        std::chrono::milliseconds(40));

    cache.store(
        "S19.2E-1-1011-11100",
        "100",
        successfulReply("event-100"));

    assert(cache.find(
        "S19.2E-1-1011-11100",
        "100",
        cached));
    assert(cached.payload == "event-100");

    assert(!cache.find(
        "S19.2E-1-1011-11100",
        "101",
        cached));
    assert(!cache.find(
        "S19.2E-1-1011-11101",
        "100",
        cached));

    SuiteBridgeMetadataCommandReply failed;
    failed.replyCode = 451;
    failed.payload = "failed";
    cache.store("S19.2E-1-1011-11100", "failed", failed);
    assert(!cache.find(
        "S19.2E-1-1011-11100",
        "failed",
        cached));

    cache.store(
        "S19.2E-1-1011-11100",
        "101",
        successfulReply("event-101"));

    assert(cache.find(
        "S19.2E-1-1011-11100",
        "100",
        cached));

    cache.store(
        "S19.2E-1-1011-11100",
        "102",
        successfulReply("event-102"));

    assert(cache.size() == 2);
    assert(cache.find(
        "S19.2E-1-1011-11100",
        "100",
        cached));
    assert(!cache.find(
        "S19.2E-1-1011-11100",
        "101",
        cached));
    assert(cache.find(
        "S19.2E-1-1011-11100",
        "102",
        cached));

    std::this_thread::sleep_for(
        std::chrono::milliseconds(60));

    assert(!cache.find(
        "S19.2E-1-1011-11100",
        "100",
        cached));
    assert(!cache.find(
        "S19.2E-1-1011-11100",
        "102",
        cached));

    SuiteBridgeMetadataReplyCache disabled(
        0,
        std::chrono::seconds(5));

    disabled.store(
        "S19.2E-1-1011-11100",
        "200",
        successfulReply("disabled"));

    assert(!disabled.find(
        "S19.2E-1-1011-11100",
        "200",
        cached));

    return 0;
}

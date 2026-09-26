#include "SuiteBridgeSvdrpTransport.h"

#include "SuiteBridgeEpgTypeSnapshotPayloadParser.h"

#include <string>

namespace vdrsuite::agent
{
SuiteBridgeEpgTypeSnapshotTransportPage
SuiteBridgeSvdrpTransport::requestEpgTypeSnapshot(
    std::int64_t fromTime,
    std::int64_t untilTime,
    std::uint64_t offset,
    std::size_t limit)
{
    SuiteBridgeEpgTypeSnapshotTransportPage page;
    if (fromTime <= 0 || untilTime <= fromTime ||
        untilTime - fromTime > 72 * 60 * 60 ||
        offset > 1000000 || limit == 0 || limit > 64)
        return page;

    const SuiteBridgeCommandReply reply = executeRequest(
        "PLUG suitebridge ETYPES " + std::to_string(fromTime) + " " +
        std::to_string(untilTime) + " " + std::to_string(offset) + " " +
        std::to_string(limit) + "\r\n");

    page.replyCode = reply.replyCode;
    switch (reply.transportStatus) {
        case SuiteBridgeTransportStatus::Success:
            page.transportStatus = SuiteBridgeReadTransportStatus::Success;
            break;
        case SuiteBridgeTransportStatus::Unavailable:
            page.transportStatus = SuiteBridgeReadTransportStatus::Unavailable;
            break;
        case SuiteBridgeTransportStatus::Timeout:
            page.transportStatus = SuiteBridgeReadTransportStatus::Timeout;
            break;
        case SuiteBridgeTransportStatus::Failed:
            page.transportStatus = SuiteBridgeReadTransportStatus::Failed;
            break;
    }
    page.transportSucceeded =
        page.transportStatus == SuiteBridgeReadTransportStatus::Success &&
        reply.replyCode == 250;
    if (!page.transportSucceeded) return page;

    page.payloadValid = detail::parseEpgTypeSnapshotPayload(
        reply.payload, fromTime, untilTime, offset, limit, page);
    if (!page.payloadValid) page.items.clear();
    return page;
}
} // namespace vdrsuite::agent

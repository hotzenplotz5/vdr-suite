#include "SearchTimerNativeCasLocalProvider.h"

#include <charconv>
#include <climits>
#include <sstream>
#include <vector>
#include <utility>

namespace {
SearchTimerNativeCasReply unknown() {
    SearchTimerNativeCasReply reply;
    reply.status = SearchTimerNativeCasStatus::outcomeUnknown;
    return reply;
}
SearchTimerNativeCasReply denied() {
    SearchTimerNativeCasReply reply;
    reply.status = SearchTimerNativeCasStatus::unavailable;
    return reply;
}
const char *actionName(SearchTimerNativeCasAction action) {
    switch (action) {
        case SearchTimerNativeCasAction::update: return "update";
        case SearchTimerNativeCasAction::activate: return "activate";
        case SearchTimerNativeCasAction::deactivate: return "deactivate";
        case SearchTimerNativeCasAction::remove: return "delete";
    }
    return nullptr;
}
bool integerId(const std::string& input, int& out) {
    if (input.empty() || input.size() > 10U ||
        (input.size() > 1U && input.front() == '0')) return false;
    unsigned int parsed = 0;
    const auto converted = std::from_chars(input.data(),
        input.data() + input.size(), parsed);
    if (converted.ec != std::errc{} ||
        converted.ptr != input.data() + input.size() ||
        parsed > static_cast<unsigned int>(INT_MAX))
        return false;
    out = static_cast<int>(parsed);
    return true;
}
std::vector<std::string> tokens(const std::string& input) {
    std::vector<std::string> out;
    std::istringstream reader(input);
    std::string token;
    while (reader >> token) {
        out.push_back(std::move(token));
        if (out.size() > 8U) break;
    }
    return out;
}
}

SearchTimerNativeCasLocalProvider::SearchTimerNativeCasLocalProvider(
        std::string localBackendId,
        vdrsuite::agent::SuiteBridgeLocalControlTransport& transport)
    : localBackendId_(std::move(localBackendId)), transport_(transport)
{
}

bool SearchTimerNativeCasLocalProvider::supportsAtomicGenerationCasV1() const
{
    if (localBackendId_.empty()) return false;
    const auto response = transport_.discoverSearchTimerCas();
    return response.transportSucceeded() && response.replyCode == 900 &&
        response.payload == "s1cas-cap/1 enabled";
}

SearchTimerNativeCasReply SearchTimerNativeCasLocalProvider::compareAndWrite(
        const SearchTimerNativeCasRequest& request)
{
    // Never confuse a local native rule ID with a remote backend rule ID.
    if (request.backendId != localBackendId_)
        return denied();
    if (!SearchTimerNativeCasContract::validNativeEvidence(request))
        return denied();

    int nativeId = -1;
    const char* action = actionName(request.action);
    if (!action || !integerId(request.nativeId, nativeId)) return denied();

    // Server-side dispatch already owns an immutable operation claim.
    // The local transport performs exactly one send, never a fallback.
    const auto response = transport_.executeSearchTimerCas(
        action, request.operationId, request.requestFingerprint,
        nativeId, request.expectedGeneration, request.expectedRevision,
        request.changePayload);
    if (!response.transportSucceeded()) return unknown();

    const auto parts = tokens(response.payload);
    if (parts.size() != 8U || parts[0] != "s1cas-result/1" ||
        parts[1] != request.operationId || parts[2] != action ||
        (parts[4] != "0" && parts[4] != "1") ||
        (parts[5] != "0" && parts[5] != "1"))
        return unknown();

    SearchTimerNativeCasReply reply;
    reply.atomicCompareAndWrite = parts[4] == "1";
    reply.noWriteAttested = parts[5] == "1";
    reply.confirmedGeneration = parts[6] == "-" ? "" : parts[6];
    reply.resultingRevision = parts[7] == "-" ? "" : parts[7];

    if (response.replyCode == 900 && parts[3] == "applied_verified")
        reply.status = SearchTimerNativeCasStatus::applied;
    else if (response.replyCode == 555 && parts[3] == "stale_generation")
        reply.status = SearchTimerNativeCasStatus::staleGeneration;
    else if (response.replyCode == 555 && parts[3] == "stale_revision")
        reply.status = SearchTimerNativeCasStatus::staleRevision;
    else if (response.replyCode == 555 && parts[3] == "not_found")
        reply.status = SearchTimerNativeCasStatus::notFound;
    else if (response.replyCode == 556 && parts[3] == "unavailable")
        reply.status = SearchTimerNativeCasStatus::unavailable;
    else
        return unknown();
    // All other native statuses are deliberately fail-closed; in particular
    // an invalid/malformed or not-inactive write attempt is not safe to retry.
    return reply;
}

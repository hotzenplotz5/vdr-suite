#include "SearchTimerNativeCasContract.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstddef>
#include <string>

namespace
{
bool idChar(char ch)
{
    return (ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') ||
        ch == '.' || ch == '-' || ch == '_';
}

bool backend(const std::string& id)
{
    return !id.empty() && id.size() <= 128U &&
        id != "." && id != ".." && id != "*" &&
        std::all_of(id.begin(), id.end(), idChar);
}

bool lowerHex(const std::string& token,
    const char* prefix, std::size_t hexDigits)
{
    const std::string head(prefix);
    if (token.size() != head.size() + hexDigits ||
        token.compare(0, head.size(), head) != 0)
        return false;
    return std::all_of(token.begin() + head.size(), token.end(),
        [](char c) {
            return (c >= '0' && c <= '9') ||
                (c >= 'a' && c <= 'f');
        });
}

bool nativeId(const std::string& id)
{
    if (id.empty() || id.size() > 10U ||
        (id.size() > 1U && id.front() == '0'))
        return false;
    unsigned long long parsed = 0ULL;
    for (char c : id)
    {
        if (c < '0' || c > '9') return false;
        parsed = parsed * 10ULL + static_cast<unsigned>(c - '0');
    }
    return parsed <= static_cast<unsigned long long>(INT_MAX);
}

bool valid(const SearchTimerNativeCasRequest& request)
{
    if (!backend(request.backendId) ||
        !lowerHex(request.operationId, "op_", 32U) ||
        !lowerHex(request.publicSearchTimerId, "st_", 32U) ||
        !lowerHex(request.expectedGeneration, "sg_", 32U) ||
        !lowerHex(request.expectedRevision, "sr_", 32U) ||
        !lowerHex(request.requestFingerprint, "sha256:", 64U) ||
        !nativeId(request.nativeId) ||
        request.changePayload.size() > 8192U)
        return false;

    switch (request.action)
    {
        case SearchTimerNativeCasAction::update:
            return !request.changePayload.empty();
        case SearchTimerNativeCasAction::activate:
        case SearchTimerNativeCasAction::deactivate:
        case SearchTimerNativeCasAction::remove:
            return request.changePayload.empty();
    }
    return false;
}
} // namespace

SearchTimerNativeCasStatus SearchTimerNativeCasContract::invokeOnce(
    const SearchTimerNativeCasRequest& request,
    ISearchTimerNativeCasProvider& provider)
{
    // A caller must first prove backend/actor authorization, explicit user
    // confirmation and its exclusive durable operation claim. None of these
    // fields can be substituted with an epgsearch numeric-ID lookup.
    if (!request.durableOperationClaimed ||
        !request.backendWriteAuthorized || !request.operatorConfirmed)
        return SearchTimerNativeCasStatus::denied;
    if (!valid(request)) return SearchTimerNativeCasStatus::invalid;

    try
    {
        // A legacy EDIS/MODS/DELS adapter MUST return false. A snapshot
        // fingerprint or a preflight GET cannot replace native atomic CAS.
        if (!provider.supportsAtomicGenerationCasV1())
            return SearchTimerNativeCasStatus::unsupported;
    }
    catch (...)
    {
        return SearchTimerNativeCasStatus::unavailable;
    }

    SearchTimerNativeCasReply reply;
    try
    {
        reply = provider.compareAndWrite(request);
    }
    catch (...)
    {
        // Network failure after the call may mean the native write happened.
        // Never make a second call or report a clean negative outcome.
        return SearchTimerNativeCasStatus::outcomeUnknown;
    }

    // All conclusions require proof from the *native* rule lock. In
    // particular a provider saying "applied" without its generation is
    // ambiguous, not success.
    if (!reply.atomicCompareAndWrite)
        return SearchTimerNativeCasStatus::outcomeUnknown;

    switch (reply.status)
    {
        case SearchTimerNativeCasStatus::applied:
            if (reply.confirmedGeneration != request.expectedGeneration)
                return SearchTimerNativeCasStatus::outcomeUnknown;
            if (request.action != SearchTimerNativeCasAction::remove &&
                (!lowerHex(reply.resultingRevision, "sr_", 32U) ||
                 reply.resultingRevision == request.expectedRevision))
                return SearchTimerNativeCasStatus::outcomeUnknown;
            if (request.action == SearchTimerNativeCasAction::remove &&
                !reply.resultingRevision.empty())
                return SearchTimerNativeCasStatus::outcomeUnknown;
            return SearchTimerNativeCasStatus::applied;
        case SearchTimerNativeCasStatus::staleGeneration:
        case SearchTimerNativeCasStatus::staleRevision:
        case SearchTimerNativeCasStatus::notFound:
            // The provider must attest that no write occurred under its
            // own lock when returning a negative CAS result.
            return reply.status;
        case SearchTimerNativeCasStatus::unsupported:
        case SearchTimerNativeCasStatus::unavailable:
        case SearchTimerNativeCasStatus::outcomeUnknown:
            return SearchTimerNativeCasStatus::outcomeUnknown;
        case SearchTimerNativeCasStatus::invalid:
        case SearchTimerNativeCasStatus::denied:
            return SearchTimerNativeCasStatus::outcomeUnknown;
    }
    return SearchTimerNativeCasStatus::outcomeUnknown;
}

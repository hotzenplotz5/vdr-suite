#pragma once

// Contract-only EPG -> TimerIntent admission preflight.
// This has NO public HTTP route and performs NO native timer mutation.
// Inputs named observed* must be supplied by an authoritative Suite-side
// EPG observation resolver, never copied from a Device request.
#include <cstddef>
#include <cstdint>
#include <string>

namespace vdrsuite::public_epg_timer
{
struct Selection
{
    std::string backendId;
    std::string channelId;
    std::string eventId;
    std::string ifMatch;
    std::string idempotencyKey;
};

struct AuthoritativeObservation
{
    std::string backendId;
    std::string channelId;
    std::string nativeEventId;
    std::string sourceId;
    std::string resourceEtag;
    std::int64_t startTime = 0;
    std::int64_t endTime = 0;
    std::uint64_t backendGeneration = 0;
    bool exactNativeIdentity = false;
    bool observationCurrent = false;
};

enum class Decision
{
    invalidSelection,
    missingPrecondition,
    invalidIdempotencyKey,
    forbidden,
    readOnlyBackend,
    noAuthoritativeObservation,
    scopeMismatch,
    revisionConflict,
    eventExpired,
    ready
};

inline bool boundedId(const std::string& value, std::size_t maximum)
{
    if (value.empty() || value.size() > maximum) return false;
    for (unsigned char c : value)
    {
        if (!((c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') ||
              c == '-' || c == '_' || c == '.' || c == ':'))
            return false;
    }
    return true;
}

inline bool strongEtag(const std::string& etag)
{
    if (etag.size() < 3 || etag.size() > 258 ||
        etag.front() != '"' || etag.back() != '"')
        return false;
    for (std::size_t i = 1; i + 1 < etag.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(etag[i]);
        if (c < 0x21 || c > 0x7e || c == '"')
            return false;
    }
    return true;
}

inline bool validIdempotencyKey(const std::string& key)
{
    if (key.empty() || key.size() > 160) return false;
    for (unsigned char c : key)
        if (c < 0x21 || c > 0x7e) return false;
    return true;
}

// Both permission and write policy are separate, trusted server-side
// decisions. There is no optimistic trust in Device-declared capabilities.
// "ready" only permits the future durable TimerIntent admission service
// to evaluate the request; it is NOT proof that a VDR timer exists.
inline Decision preflight(
    const Selection& requested,
    const AuthoritativeObservation& observed,
    bool actorHasTimersCreateGrant,
    bool backendAllowsMutation,
    std::int64_t now)
{
    if (!boundedId(requested.backendId, 128) ||
        !boundedId(requested.channelId, 256) ||
        !boundedId(requested.eventId, 256))
        return Decision::invalidSelection;
    if (!strongEtag(requested.ifMatch))
        return Decision::missingPrecondition;
    if (!validIdempotencyKey(requested.idempotencyKey))
        return Decision::invalidIdempotencyKey;
    if (!actorHasTimersCreateGrant)
        return Decision::forbidden;
    if (!backendAllowsMutation)
        return Decision::readOnlyBackend;
    if (!observed.exactNativeIdentity || !observed.observationCurrent ||
        observed.sourceId.empty() || observed.sourceId.size() > 160 ||
        observed.backendGeneration == 0 ||
        observed.startTime <= 0 || observed.endTime <= observed.startTime ||
        !boundedId(observed.nativeEventId, 256) ||
        !strongEtag(observed.resourceEtag))
        return Decision::noAuthoritativeObservation;
    if (observed.backendId != requested.backendId ||
        observed.channelId != requested.channelId ||
        observed.nativeEventId != requested.eventId)
        return Decision::scopeMismatch;
    if (observed.resourceEtag != requested.ifMatch)
        return Decision::revisionConflict;
    if (now <= 0 || observed.endTime <= now)
        return Decision::eventExpired;
    return Decision::ready;
}
} // namespace vdrsuite::public_epg_timer

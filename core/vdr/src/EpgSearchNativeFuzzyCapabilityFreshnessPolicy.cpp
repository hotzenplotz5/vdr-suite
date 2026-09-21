#include "EpgSearchNativeFuzzyCapabilityFreshnessPolicy.h"

EpgSearchNativeFuzzyCapabilityFreshnessPolicy::EpgSearchNativeFuzzyCapabilityFreshnessPolicy(
    long long maxAgeSeconds)
    : maxAgeSeconds_(maxAgeSeconds)
{
}

long long EpgSearchNativeFuzzyCapabilityFreshnessPolicy::maxAgeSeconds() const
{
    return maxAgeSeconds_;
}

EpgSearchNativeFuzzyCapabilityFreshnessDecision
EpgSearchNativeFuzzyCapabilityFreshnessPolicy::decide(
    long long ageSeconds) const
{
    EpgSearchNativeFuzzyCapabilityFreshnessDecision decision;
    decision.ageSeconds = ageSeconds;
    decision.maxAgeSeconds = maxAgeSeconds_;

    if (ageSeconds < 0)
    {
        decision.fresh = false;
        decision.status = "future-timestamp";
        decision.reason = "persisted probe timestamp is in the future";
        return decision;
    }

    if (ageSeconds > maxAgeSeconds_)
    {
        decision.fresh = false;
        decision.status = "stale";
        decision.reason = "persisted probe result is older than the freshness policy allows";
        return decision;
    }

    decision.fresh = true;
    decision.status = "fresh";
    decision.reason = "persisted probe result is within the freshness window";
    return decision;
}

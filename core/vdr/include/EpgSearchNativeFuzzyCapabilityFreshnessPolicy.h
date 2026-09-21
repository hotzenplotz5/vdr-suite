#pragma once

#include <string>

struct EpgSearchNativeFuzzyCapabilityFreshnessDecision
{
    bool fresh = false;
    long long ageSeconds = 0;
    long long maxAgeSeconds = 0;
    std::string status;
    std::string reason;
};

class EpgSearchNativeFuzzyCapabilityFreshnessPolicy
{
public:
    explicit EpgSearchNativeFuzzyCapabilityFreshnessPolicy(
        long long maxAgeSeconds = 7LL * 24LL * 60LL * 60LL);

    long long maxAgeSeconds() const;

    EpgSearchNativeFuzzyCapabilityFreshnessDecision decide(
        long long ageSeconds) const;

private:
    long long maxAgeSeconds_;
};

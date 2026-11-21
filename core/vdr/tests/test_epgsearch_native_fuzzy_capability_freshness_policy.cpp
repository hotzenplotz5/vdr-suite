#include "EpgSearchNativeFuzzyCapabilityFreshnessPolicy.h"

#include <cassert>
#include <iostream>

int main()
{
    EpgSearchNativeFuzzyCapabilityFreshnessPolicy policy(60);

    const auto fresh = policy.decide(60);
    assert(fresh.fresh);
    assert(fresh.status == "fresh");

    const auto stale = policy.decide(61);
    assert(!stale.fresh);
    assert(stale.status == "stale");

    const auto future = policy.decide(-1);
    assert(!future.fresh);
    assert(future.status == "future-timestamp");

    std::cout
        << "test_epgsearch_native_fuzzy_capability_freshness_policy passed"
        << std::endl;

    return 0;
}

#include "DomainRefreshPolicy.h"
#include "CapabilityResolver.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <iostream>

static void test_lightweight_domains_allow_automatic_full_refresh()
{
    DomainRefreshPolicy policy;

    assert(policy.allowsAutomaticFullRefresh(RefreshDomain::Status) == true);
    assert(policy.allowsAutomaticFullRefresh(RefreshDomain::Channels) == true);
    assert(policy.allowsAutomaticFullRefresh(RefreshDomain::Recordings) == true);
    assert(policy.allowsAutomaticFullRefresh(RefreshDomain::Timers) == true);
}

static void test_events_domain_is_heavy()
{
    DomainRefreshPolicy policy;

    assert(policy.isHeavyDomain(RefreshDomain::Events) == true);
    assert(policy.allowsAutomaticFullRefresh(RefreshDomain::Events) == false);
    assert(policy.requiresSelectiveRefresh(RefreshDomain::Events) == true);
    assert(policy.allowsSelectiveEventRefresh() == true);
}

static void test_events_domain_can_fallback_to_full_refresh()
{
    DomainRefreshPolicy policy;

    policy.setAllowSelectiveEventRefresh(false);

    assert(policy.isHeavyDomain(RefreshDomain::Events) == true);
    assert(policy.allowsAutomaticFullRefresh(RefreshDomain::Events) == true);
    assert(policy.requiresSelectiveRefresh(RefreshDomain::Events) == false);
    assert(policy.allowsSelectiveEventRefresh() == false);
}

static void test_policy_from_capability_resolver_allows_selective_events()
{
    VdrCapabilitySet capabilities;
    capabilities.eventsSelectiveRead = true;

    CapabilityResolver resolver(capabilities);

    DomainRefreshPolicy policy =
        DomainRefreshPolicy::fromCapabilityResolver(resolver);

    assert(policy.allowsSelectiveEventRefresh() == true);
    assert(policy.allowsAutomaticFullRefresh(RefreshDomain::Events) == false);
    assert(policy.requiresSelectiveRefresh(RefreshDomain::Events) == true);
}

static void test_policy_from_capability_resolver_disables_selective_events()
{
    VdrCapabilitySet capabilities;
    capabilities.eventsSelectiveRead = false;

    CapabilityResolver resolver(capabilities);

    DomainRefreshPolicy policy =
        DomainRefreshPolicy::fromCapabilityResolver(resolver);

    assert(policy.allowsSelectiveEventRefresh() == false);
    assert(policy.allowsAutomaticFullRefresh(RefreshDomain::Events) == true);
    assert(policy.requiresSelectiveRefresh(RefreshDomain::Events) == false);
}

static void test_lightweight_domains_are_not_heavy()
{
    DomainRefreshPolicy policy;

    assert(policy.isHeavyDomain(RefreshDomain::Status) == false);
    assert(policy.isHeavyDomain(RefreshDomain::Channels) == false);
    assert(policy.isHeavyDomain(RefreshDomain::Recordings) == false);
    assert(policy.isHeavyDomain(RefreshDomain::Timers) == false);
}

int main()
{
    test_lightweight_domains_allow_automatic_full_refresh();
    test_events_domain_is_heavy();
    test_events_domain_can_fallback_to_full_refresh();
    test_policy_from_capability_resolver_allows_selective_events();
    test_policy_from_capability_resolver_disables_selective_events();
    test_lightweight_domains_are_not_heavy();

    std::cout
        << "test_domain_refresh_policy passed"
        << std::endl;

    return 0;
}

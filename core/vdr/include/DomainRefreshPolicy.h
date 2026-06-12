#ifndef DOMAIN_REFRESH_POLICY_H
#define DOMAIN_REFRESH_POLICY_H

enum class RefreshDomain {
    Status,
    Channels,
    Recordings,
    Timers,
    Events
};

class DomainRefreshPolicy {
public:
    bool allowsAutomaticFullRefresh(RefreshDomain domain) const
    {
        switch (domain) {
        case RefreshDomain::Status:
        case RefreshDomain::Channels:
        case RefreshDomain::Recordings:
        case RefreshDomain::Timers:
            return true;
        case RefreshDomain::Events:
            return !allowSelectiveEventRefresh_;
        }

        return false;
    }

    bool requiresSelectiveRefresh(RefreshDomain domain) const
    {
        if (domain == RefreshDomain::Events) {
            return allowSelectiveEventRefresh_;
        }

        return !allowsAutomaticFullRefresh(domain);
    }

    void setAllowSelectiveEventRefresh(bool allowed)
    {
        allowSelectiveEventRefresh_ = allowed;
    }

    bool allowsSelectiveEventRefresh() const
    {
        return allowSelectiveEventRefresh_;
    }

    bool isHeavyDomain(RefreshDomain domain) const
    {
        switch (domain) {
        case RefreshDomain::Events:
            return true;
        case RefreshDomain::Status:
        case RefreshDomain::Channels:
        case RefreshDomain::Recordings:
        case RefreshDomain::Timers:
            return false;
        }

        return false;
    }
private:
    bool allowSelectiveEventRefresh_ = true;
};

#endif

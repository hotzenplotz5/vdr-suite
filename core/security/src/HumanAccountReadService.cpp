#include "HumanAccountReadService.h"

HumanAccountReadService::HumanAccountReadService(
    HumanAccountRepository& repository)
    : repository_(repository)
{
}

HumanAccountLookupResult HumanAccountReadService::find(
    const std::string& accountId) const
{
    return repository_.findByAccountId(accountId);
}

HumanAccountListResult HumanAccountReadService::list() const
{
    return repository_.listAll();
}

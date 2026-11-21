#pragma once

#include "HumanAccountRepository.h"

#include <string>

class HumanAccountReadService
{
public:
    explicit HumanAccountReadService(
        HumanAccountRepository& repository);

    HumanAccountLookupResult find(
        const std::string& accountId) const;

    HumanAccountListResult list() const;

private:
    HumanAccountRepository& repository_;
};

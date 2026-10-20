#pragma once

#include "AccountabilityEvent.h"

#include <vector>

class Database;

class AccountabilityEventRepository
{
public:
    explicit AccountabilityEventRepository(Database& database);

    bool ensureSchema();
    const Database* databaseIdentity() const noexcept { return &database_; }
    bool append(const AccountabilityEvent& event);
    std::vector<AccountabilityEvent> listAll() const;

private:
    Database& database_;
};

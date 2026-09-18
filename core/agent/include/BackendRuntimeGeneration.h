#pragma once

#include <cstdint>
#include <string>

class Database;

struct BackendRuntimeGenerationAllocation
{
    bool accepted = false;
    std::uint64_t generation = 0;
    std::string reasonCode;
};

class BackendRuntimeGenerationRepository
{
public:
    explicit BackendRuntimeGenerationRepository(Database& database);

    bool ensureSchema();

    BackendRuntimeGenerationAllocation allocate(
        const std::string& backendId,
        std::int64_t now);

    BackendRuntimeGenerationAllocation allocateInCurrentTransaction(
        const std::string& backendId,
        std::int64_t now);

private:
    Database& database_;
};

#pragma once

#include "DashboardController.h"

#include <memory>
#include <mutex>
#include <string>

class BackendRegistryService;
class Database;
class GlobalSearchController;
class GlobalSearchRepository;
class GlobalSearchService;

class GlobalSearchApiRuntime
{
public:
    static GlobalSearchApiRuntime& instance();

    bool configure(
        Database& database,
        BackendRegistryService& backendRegistryService);

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response) const;

    bool configured() const;
    void reset();

private:
    GlobalSearchApiRuntime() = default;

    mutable std::mutex mutex_;
    std::unique_ptr<GlobalSearchRepository> writerRepository_;
    std::unique_ptr<Database> readDatabase_;
    std::unique_ptr<GlobalSearchRepository> readRepository_;
    std::unique_ptr<GlobalSearchService> service_;
    std::unique_ptr<GlobalSearchController> controller_;
};

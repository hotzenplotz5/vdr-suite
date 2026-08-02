#include "GlobalSearchApiRuntime.h"

#include "BackendRegistryService.h"
#include "Database.h"
#include "GlobalSearchController.h"
#include "GlobalSearchRepository.h"
#include "GlobalSearchService.h"
#include "RestQueryParameters.h"

#include <cstdint>
#include <memory>
#include <string>

namespace
{
std::string requestPath(const std::string& requestTarget)
{
    const std::size_t query = requestTarget.find('?');
    return requestTarget.substr(0, query);
}

std::string requestQuery(const std::string& requestTarget)
{
    const std::size_t query = requestTarget.find('?');
    return query == std::string::npos ? std::string() : requestTarget.substr(query + 1);
}

std::int64_t parseInt64(const std::string& value, std::int64_t fallback)
{
    if (value.empty()) return fallback;
    try
    {
        std::size_t parsed = 0;
        const long long result = std::stoll(value, &parsed);
        return parsed == value.size() ? static_cast<std::int64_t>(result) : fallback;
    }
    catch (...) { return fallback; }
}

ApiResponse unavailableResponse()
{
    ApiResponse response;
    response.statusCode = 503;
    response.contentType = "application/json";
    response.body = "{\"error\":\"global search runtime is not configured\"}";
    return response;
}
}

GlobalSearchApiRuntime& GlobalSearchApiRuntime::instance()
{
    static GlobalSearchApiRuntime runtime;
    return runtime;
}

bool GlobalSearchApiRuntime::configure(
    Database& database,
    BackendRegistryService& backendRegistryService)
{
    auto writerRepository = std::make_unique<GlobalSearchRepository>(database);
    if (!writerRepository->ensureSchema()) return false;

    std::unique_ptr<Database> readDatabase;
    std::unique_ptr<GlobalSearchRepository> readRepository;
    GlobalSearchRepository* controllerRepository = writerRepository.get();

    const std::string filename = database.filename();
    if (!filename.empty())
    {
        auto candidateDatabase = std::make_unique<Database>();
        if (candidateDatabase->open(filename))
        {
            candidateDatabase->execute("PRAGMA busy_timeout=5000;");
            candidateDatabase->execute("PRAGMA query_only=ON;");
            auto candidateRepository = std::make_unique<GlobalSearchRepository>(*candidateDatabase);
            if (candidateRepository->ready())
            {
                controllerRepository = candidateRepository.get();
                readRepository = std::move(candidateRepository);
                readDatabase = std::move(candidateDatabase);
            }
        }
    }

    auto service = std::make_unique<GlobalSearchService>(*controllerRepository);
    auto controller = std::make_unique<GlobalSearchController>(
        *service,
        backendRegistryService);

    std::lock_guard<std::mutex> lock(mutex_);
    controller_.reset();
    service_.reset();
    readRepository_.reset();
    readDatabase_.reset();
    writerRepository_.reset();
    writerRepository_ = std::move(writerRepository);
    readDatabase_ = std::move(readDatabase);
    readRepository_ = std::move(readRepository);
    service_ = std::move(service);
    controller_ = std::move(controller);
    return true;
}

bool GlobalSearchApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    ApiResponse& response) const
{
    const std::string path = requestPath(requestTarget);
    if (path != "/api/search" && path != "/api/vdr/search") return false;

    GlobalSearchController* controller = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        controller = controller_.get();
    }
    if (controller == nullptr)
    {
        response = unavailableResponse();
        return true;
    }

    const RestQueryParameters query = RestQueryParameters::parse(requestQuery(requestTarget));
    response = controller->search(
        query.get("backend", query.get("backendId", "default")),
        query.get("query", query.get("text")),
        parseInt64(query.get("from"), -1),
        parseInt64(query.get("until"), -1),
        query.getInt("limit", 20),
        query.getInt("offset", 0));
    return true;
}

bool GlobalSearchApiRuntime::configured() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return writerRepository_ != nullptr && service_ != nullptr && controller_ != nullptr;
}

void GlobalSearchApiRuntime::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    controller_.reset();
    service_.reset();
    readRepository_.reset();
    readDatabase_.reset();
    writerRepository_.reset();
}

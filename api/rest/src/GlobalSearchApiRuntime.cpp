#include "GlobalSearchApiRuntime.h"

#include "BackendRegistryService.h"
#include "Database.h"
#include "GlobalSearchController.h"
#include "GlobalSearchRepository.h"
#include "GlobalSearchService.h"
#include "RestQueryParameters.h"

#include <cstdint>
#include <memory>
#include <sqlite3.h>
#include <string>
#include <utility>
#include <vector>

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

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string{}
        : std::string(reinterpret_cast<const char*>(value));
}

std::vector<GlobalSearchPersonPortrait> localPersonPortraits(
    Database& database,
    const std::string& backendId)
{
    std::vector<GlobalSearchPersonPortrait> portraits;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT p.display_name,r.role,"
        "COALESCE(NULLIF(c.backend_native_id,''),v.resource_key),"
        "r.ordinal,v.revision "
        "FROM suite_metadata_manual_assignment_values v "
        "JOIN suite_metadata_assignments a "
        "ON a.metadata_assignment_id=v.metadata_assignment_id "
        "JOIN suite_metadata_recording_person_relations r "
        "ON r.metadata_assignment_id=v.metadata_assignment_id "
        "JOIN suite_metadata_person_values p "
        "ON p.metadata_entity_id=r.person_entity_id "
        "JOIN suite_metadata_person_profiles profile "
        "ON profile.provider_id=p.provider_id "
        "AND profile.external_namespace=p.external_namespace "
        "AND profile.external_id=p.external_id "
        "LEFT JOIN vdr_recording_cache c "
        "ON c.backend_id=v.backend_id AND c.cache_key=v.resource_key "
        "WHERE v.backend_id=? "
        "AND a.assignment_state='selected' "
        "AND a.manual_assignment=1 "
        "AND a.relationship_locked=1 "
        "AND profile.local_path<>'' "
        "ORDER BY p.name_folded,r.role,v.revision DESC,r.ordinal,"
        "v.metadata_assignment_id;";
    if (sqlite3_prepare_v2(
            database.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return portraits;

    sqlite3_bind_text(
        statement,
        1,
        backendId.c_str(),
        static_cast<int>(backendId.size()),
        SQLITE_TRANSIENT);
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        GlobalSearchPersonPortrait portrait;
        portrait.name = columnText(statement, 0);
        portrait.role = columnText(statement, 1);
        portrait.backendNativeId = columnText(statement, 2);
        portrait.index = sqlite3_column_int(statement, 3);
        portrait.assignmentRevision = sqlite3_column_int(statement, 4);
        if (!portrait.name.empty() && !portrait.backendNativeId.empty() &&
            portrait.index >= 0 && portrait.assignmentRevision > 0)
            portraits.push_back(std::move(portrait));
    }
    sqlite3_finalize(statement);
    return portraits;
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

    GlobalSearchController::PersonPortraitLookup personPortraitLookup;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        personPortraitLookup = personPortraitLookup_;
    }
    Database* portraitDatabase = readDatabase
        ? readDatabase.get()
        : &database;
    if (!personPortraitLookup)
    {
        personPortraitLookup = [portraitDatabase](const std::string& backendId)
        {
            return localPersonPortraits(*portraitDatabase, backendId);
        };
    }

    auto service = std::make_unique<GlobalSearchService>(*controllerRepository);
    auto controller = std::make_unique<GlobalSearchController>(
        *service,
        backendRegistryService,
        std::move(personPortraitLookup));

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

void GlobalSearchApiRuntime::setPersonPortraitLookup(
    GlobalSearchController::PersonPortraitLookup personPortraitLookup)
{
    std::lock_guard<std::mutex> lock(mutex_);
    personPortraitLookup_ = std::move(personPortraitLookup);
    if (controller_)
        controller_->setPersonPortraitLookup(personPortraitLookup_);
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
    personPortraitLookup_ = {};
}
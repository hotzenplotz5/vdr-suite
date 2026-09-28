#include "SeriesArtworkBackendSettingsService.h"
#include "TmdbRecordingMetadataCandidateProvider.h"

#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <limits>
#include <sqlite3.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace
{
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string()
        : reinterpret_cast<const char*>(value);
}

bool tokenSyntaxValid(const std::string& token)
{
    return !token.empty() && token.size() <= 4096U &&
        std::all_of(
            token.begin(),
            token.end(),
            [](unsigned char character)
            {
                return character > 0x20U && character != 0x7fU;
            });
}

bool jsonContentType(const std::string& contentType)
{
    return contentType == "application/json" ||
        contentType.compare(0, 17U, "application/json;") == 0;
}

bool hasSeriesTmdbIdentity(const EpgScraperMetadata& metadata)
{
    return std::any_of(
        metadata.externalIds.begin(),
        metadata.externalIds.end(),
        [](const EpgScraperExternalId& identity)
        {
            return identity.provider == EpgScraperExternalIdProvider::Tmdb &&
                identity.scope == EpgScraperExternalIdScope::Series &&
                !identity.value.empty();
        });
}

EpgScraperMetadata qualifyTvScraperTmdbIdentity(
    const EpgScraperMetadata& metadata)
{
    EpgScraperMetadata qualified = metadata;
    if (metadata.provider != "tvscraper" ||
        metadata.mediaType != EpgScraperMediaType::Series ||
        metadata.providerId <= 0 ||
        hasSeriesTmdbIdentity(metadata))
    {
        return qualified;
    }

    EpgScraperExternalId identity;
    identity.provider = EpgScraperExternalIdProvider::Tmdb;
    identity.scope = EpgScraperExternalIdScope::Series;
    identity.value = std::to_string(metadata.providerId);
    qualified.externalIds.push_back(std::move(identity));
    return qualified;
}

bool invalidateUnresolvedSeriesMetadata(
    Database& database,
    const std::string& backendId)
{
    if (!database.tableExists("epg_scraper_metadata_cache"))
    {
        return true;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM epg_scraper_metadata_cache "
        "WHERE backend_id=? "
        "AND json_valid(public_json)=1 "
        "AND json_extract(public_json,'$.mediaType')='series' "
        "AND COALESCE("
        "json_extract(public_json,'$.preferredArtwork.available'),0)=0;";
    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool removed =
        bindText(statement, 1, backendId) &&
        sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return removed;
}


bool seriesArtworkOverrideColumnExists(
    Database& database,
    const std::string& column)
{
    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database.handle(),
            "PRAGMA table_info(backend_series_artwork_overrides);",
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    bool found = false;

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        if (columnText(statement, 1) == column)
        {
            found = true;
            break;
        }
    }

    sqlite3_finalize(statement);
    return found;
}

bool ensureSeriesArtworkOverrideColumn(
    Database& database,
    const std::string& column,
    const std::string& definition)
{
    if (seriesArtworkOverrideColumnExists(
            database,
            column))
    {
        return true;
    }

    return database.execute(
        "ALTER TABLE backend_series_artwork_overrides "
        "ADD COLUMN " + definition + ";");
}

bool digitsOnly(const std::string& value)
{
    return !value.empty() &&
        value.size() <= 16U &&
        std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return character >= '0' &&
                    character <= '9';
            });
}

bool validTmdbPosterReference(
    const std::string& value)
{
    return value.size() >= 6U &&
        value.size() <= 256U &&
        value.front() == '/' &&
        value.find('/', 1U) == std::string::npos &&
        value.find("..") == std::string::npos &&
        std::all_of(
            value.begin() + 1U,
            value.end(),
            [](unsigned char character)
            {
                return
                    (character >= 'A' &&
                     character <= 'Z') ||
                    (character >= 'a' &&
                     character <= 'z') ||
                    (character >= '0' &&
                     character <= '9') ||
                    character == '_' ||
                    character == '-' ||
                    character == '.';
            });
}

std::string percentEncodeSeriesArtwork(
    const std::string& value)
{
    static const char Hex[] =
        "0123456789ABCDEF";

    std::string output;

    for (const unsigned char character : value)
    {
        if ((character >= 'A' &&
             character <= 'Z') ||
            (character >= 'a' &&
             character <= 'z') ||
            (character >= '0' &&
             character <= '9') ||
            character == '-' ||
            character == '_' ||
            character == '.' ||
            character == '~')
        {
            output.push_back(
                static_cast<char>(character));
        }
        else
        {
            output.push_back('%');
            output.push_back(
                Hex[character >> 4U]);
            output.push_back(
                Hex[character & 0x0fU]);
        }
    }

    return output;
}

std::string seriesArtworkCoverImageUrl(
    const std::string& backendId,
    const std::string& seriesKey,
    int revision)
{
    return
        "/api/backends/" +
        percentEncodeSeriesArtwork(backendId) +
        "/settings/series-artwork/image"
        "?seriesKey=" +
        percentEncodeSeriesArtwork(seriesKey) +
        "&revision=" +
        std::to_string(revision);
}

bool localSeriesCoverPath(
    const std::string& path,
    const std::string& root)
{
    if (path.empty() || root.empty())
    {
        return false;
    }

    const std::filesystem::path normalizedPath =
        std::filesystem::path(path)
            .lexically_normal();

    const std::filesystem::path normalizedRoot =
        std::filesystem::path(root)
            .lexically_normal();

    if (!normalizedPath.is_absolute() ||
        !normalizedRoot.is_absolute() ||
        normalizedPath == normalizedPath.root_path() ||
        normalizedRoot == normalizedRoot.root_path())
    {
        return false;
    }

    std::string rootPrefix =
        normalizedRoot.string();

    if (!rootPrefix.empty() &&
        rootPrefix.back() != '/')
    {
        rootPrefix.push_back('/');
    }

    return normalizedPath.string().compare(
        0,
        rootPrefix.size(),
        rootPrefix) == 0;
}

std::string seriesArtworkImageContentType(
    const std::string& path)
{
    const std::string extension =
        std::filesystem::path(path)
            .extension()
            .string();

    if (extension == ".jpg" ||
        extension == ".jpeg")
    {
        return "image/jpeg";
    }

    if (extension == ".png")
    {
        return "image/png";
    }

    if (extension == ".webp")
    {
        return "image/webp";
    }

    return {};
}

SeriesArtworkImageResult readSeriesArtworkImage(
    const std::string& path,
    const std::string& root)
{
    SeriesArtworkImageResult result;

    if (!localSeriesCoverPath(path, root))
    {
        result.statusCode = 404;
        result.errorCode =
            "series_cover_image_not_found";
        result.message =
            "The Series cover image is unavailable";
        return result;
    }

    const std::string contentType =
        seriesArtworkImageContentType(path);

    if (contentType.empty())
    {
        result.statusCode = 415;
        result.errorCode =
            "invalid_series_cover_image_type";
        result.message =
            "The Series cover image type is unsupported";
        return result;
    }

    const int descriptor = ::open(
        path.c_str(),
        O_RDONLY |
        O_CLOEXEC |
        O_NOFOLLOW);

    if (descriptor < 0)
    {
        result.statusCode = 404;
        result.errorCode =
            "series_cover_image_not_found";
        result.message =
            "The Series cover image is unavailable";
        return result;
    }

    struct stat metadata{};

    if (::fstat(
            descriptor,
            &metadata) != 0 ||
        !S_ISREG(metadata.st_mode) ||
        metadata.st_size <= 0 ||
        metadata.st_size >
            32 * 1024 * 1024)
    {
        ::close(descriptor);

        result.statusCode = 404;
        result.errorCode =
            "series_cover_image_not_found";
        result.message =
            "The Series cover image is unavailable";
        return result;
    }

    std::string body(
        static_cast<std::size_t>(
            metadata.st_size),
        '\0');

    std::size_t offset = 0;

    while (offset < body.size())
    {
        const ssize_t count = ::read(
            descriptor,
            &body[offset],
            body.size() - offset);

        if (count < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            body.clear();
            break;
        }

        if (count == 0)
        {
            body.clear();
            break;
        }

        offset +=
            static_cast<std::size_t>(count);
    }

    ::close(descriptor);

    if (body.empty())
    {
        result.statusCode = 404;
        result.errorCode =
            "series_cover_image_not_found";
        result.message =
            "The Series cover image is unavailable";
        return result;
    }

    result.success = true;
    result.statusCode = 200;
    result.contentType = contentType;
    result.body = std::move(body);

    return result;
}

int openDirectoryNoFollow(const std::filesystem::path& path)
{
    const std::filesystem::path normalized = path.lexically_normal();
    if (!normalized.is_absolute() || normalized == normalized.root_path())
    {
        return -1;
    }

    int current = ::open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (current < 0)
    {
        return -1;
    }

    for (const auto& component : normalized.relative_path())
    {
        const std::string name = component.string();
        if (name.empty() || name == "." || name == "..")
        {
            ::close(current);
            return -1;
        }

        const int next = ::openat(
            current,
            name.c_str(),
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        ::close(current);
        if (next < 0)
        {
            return -1;
        }
        current = next;
    }

    struct stat metadata{};
    if (::fstat(current, &metadata) != 0 ||
        !S_ISDIR(metadata.st_mode) ||
        (metadata.st_mode & 0077) != 0)
    {
        ::close(current);
        return -1;
    }

    return current;
}

bool writeAll(int descriptor, const std::string& value)
{
    std::size_t offset = 0;
    while (offset < value.size())
    {
        const ssize_t written = ::write(
            descriptor,
            value.data() + offset,
            value.size() - offset);
        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return false;
        }
        if (written == 0)
        {
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

std::string tokenFilename(const std::string& backendId)
{
    return backendId + ".tmdb-token";
}
}

SeriesArtworkBackendSettingsService::SeriesArtworkBackendSettingsService(
    Database& database,
    IExternalArtworkHttpTransport& transport,
    ISeriesArtworkProviderCache& cache,
    SeriesArtworkBackendSettingsConfig config)
    : database_(database),
      transport_(transport),
      cache_(cache),
      config_(std::move(config))
{
    if (!validProvider(config_.defaultProvider))
    {
        config_.defaultProvider = "none";
    }
}

bool SeriesArtworkBackendSettingsService::validBackendId(
    const std::string& backendId)
{
    return !backendId.empty() && backendId.size() <= 128U &&
        std::all_of(
            backendId.begin(),
            backendId.end(),
            [](unsigned char character)
            {
                return (character >= 'a' && character <= 'z') ||
                    (character >= 'A' && character <= 'Z') ||
                    (character >= '0' && character <= '9') ||
                    character == '-' || character == '_' || character == '.';
            });
}

bool SeriesArtworkBackendSettingsService::validProvider(
    const std::string& provider)
{
    return provider == "none" ||
        provider == "tvmaze" ||
        provider == "tmdb";
}

bool SeriesArtworkBackendSettingsService::validSeriesKey(
    const std::string& seriesKey)
{
    return !seriesKey.empty() &&
        seriesKey.size() <= 1024U &&
        std::all_of(
            seriesKey.begin(),
            seriesKey.end(),
            [](unsigned char character)
            {
                return character >= 0x20U && character != 0x7fU;
            });
}

bool SeriesArtworkBackendSettingsService::validCoverPosterUrl(
    const std::string& posterUrl)
{
    if (posterUrl.empty() || posterUrl.size() > 2048U ||
        posterUrl.find("://") != std::string::npos ||
        posterUrl.find("..") != std::string::npos)
    {
        return false;
    }

    static const std::string primary =
        "/api/vdr/recordings/metadata/image?";
    static const std::string compatibility =
        "/api/recordings/metadata/image?";

    return posterUrl.compare(0, primary.size(), primary) == 0 ||
        posterUrl.compare(
            0,
            compatibility.size(),
            compatibility) == 0;
}

bool SeriesArtworkBackendSettingsService::ensureSchema()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return ensureSchemaLocked();
}

bool SeriesArtworkBackendSettingsService::ensureSchemaLocked() const
{
    if (!database_.execute(
            "CREATE TABLE IF NOT EXISTS backend_series_artwork_settings ("
            "backend_id TEXT PRIMARY KEY,"
            "provider TEXT NOT NULL,"
            "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "CHECK(provider IN ('none','tvmaze','tmdb'))"
            ");"
            "CREATE TABLE IF NOT EXISTS backend_series_artwork_overrides ("
            "backend_id TEXT NOT NULL,"
            "series_key TEXT NOT NULL,"
            "poster_url TEXT NOT NULL DEFAULT '',"
            "poster_path TEXT NOT NULL DEFAULT '',"
            "provider_id TEXT NOT NULL DEFAULT '',"
            "external_id TEXT NOT NULL DEFAULT '',"
            "poster_reference TEXT NOT NULL DEFAULT '',"
            "revision INTEGER NOT NULL DEFAULT 1,"
            "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "PRIMARY KEY(backend_id,series_key),"
            "CHECK(revision>0)"
            ");"))
    {
        return false;
    }

    return
        ensureSeriesArtworkOverrideColumn(
            database_,
            "poster_path",
            "poster_path TEXT NOT NULL DEFAULT ''") &&
        ensureSeriesArtworkOverrideColumn(
            database_,
            "provider_id",
            "provider_id TEXT NOT NULL DEFAULT ''") &&
        ensureSeriesArtworkOverrideColumn(
            database_,
            "external_id",
            "external_id TEXT NOT NULL DEFAULT ''") &&
        ensureSeriesArtworkOverrideColumn(
            database_,
            "poster_reference",
            "poster_reference TEXT NOT NULL DEFAULT ''");
}

bool SeriesArtworkBackendSettingsService::readManagedProviderLocked(
    const std::string& backendId,
    std::string& provider) const
{
    provider.clear();
    if (!ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT provider FROM backend_series_artwork_settings "
        "WHERE backend_id=? LIMIT 1;";
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound = bindText(statement, 1, backendId);
    const int step = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    if (step == SQLITE_ROW)
    {
        provider = columnText(statement, 0);
    }
    sqlite3_finalize(statement);
    return step == SQLITE_ROW || step == SQLITE_DONE;
}

bool SeriesArtworkBackendSettingsService::storeManagedProviderLocked(
    const std::string& backendId,
    const std::string& provider) const
{
    if (!ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO backend_series_artwork_settings "
        "(backend_id,provider,updated_at) VALUES (?,?,CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id) DO UPDATE SET "
        "provider=excluded.provider,updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool stored =
        bindText(statement, 1, backendId) &&
        bindText(statement, 2, provider) &&
        sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return stored;
}

std::vector<SeriesArtworkCoverOverride>
SeriesArtworkBackendSettingsService::loadCoverOverridesLocked(
    const std::string& backendId) const
{
    std::vector<SeriesArtworkCoverOverride> overrides;

    if (!ensureSchemaLocked())
    {
        return overrides;
    }

    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "SELECT series_key,poster_url,revision,"
        "poster_path,provider_id,external_id,poster_reference "
        "FROM backend_series_artwork_overrides "
        "WHERE backend_id=? ORDER BY series_key;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return overrides;
    }

    if (!bindText(
            statement,
            1,
            backendId))
    {
        sqlite3_finalize(statement);
        return overrides;
    }

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        SeriesArtworkCoverOverride value;

        value.seriesKey =
            columnText(statement, 0);

        const std::string legacyUrl =
            columnText(statement, 1);

        value.revision =
            sqlite3_column_int(
                statement,
                2);

        const std::string posterPath =
            columnText(statement, 3);

        const std::string providerId =
            columnText(statement, 4);

        const std::string externalId =
            columnText(statement, 5);

        const std::string posterReference =
            columnText(statement, 6);

        if (!validSeriesKey(value.seriesKey) ||
            value.revision <= 0)
        {
            continue;
        }

        if (providerId == "tmdb" &&
            digitsOnly(externalId) &&
            validTmdbPosterReference(
                posterReference) &&
            localSeriesCoverPath(
                posterPath,
                config_.seriesCoverCacheRoot))
        {
            value.posterUrl =
                seriesArtworkCoverImageUrl(
                    backendId,
                    value.seriesKey,
                    value.revision);

            overrides.push_back(
                std::move(value));

            continue;
        }

        if (validCoverPosterUrl(
                legacyUrl))
        {
            value.posterUrl = legacyUrl;

            overrides.push_back(
                std::move(value));
        }
    }

    sqlite3_finalize(statement);

    return overrides;
}

bool SeriesArtworkBackendSettingsService::storeCoverOverrideLocked(
    const std::string& backendId,
    const std::string& seriesKey,
    const std::string& posterUrl) const
{
    if (!ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO backend_series_artwork_overrides "
        "(backend_id,series_key,poster_url,revision,updated_at) "
        "VALUES(?,?,?,1,CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id,series_key) DO UPDATE SET "
        "poster_url=excluded.poster_url,"
        "revision=backend_series_artwork_overrides.revision+1,"
        "updated_at=CURRENT_TIMESTAMP;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool stored =
        bindText(statement, 1, backendId) &&
        bindText(statement, 2, seriesKey) &&
        bindText(statement, 3, posterUrl) &&
        sqlite3_step(statement) == SQLITE_DONE;

    sqlite3_finalize(statement);
    return stored;
}

bool SeriesArtworkBackendSettingsService::storeTmdbCoverOverrideLocked(
    const std::string& backendId,
    const std::string& seriesKey,
    const std::string& posterPath,
    const std::string& externalId,
    const std::string& posterReference) const
{
    if (!ensureSchemaLocked() ||
        !localSeriesCoverPath(
            posterPath,
            config_.seriesCoverCacheRoot))
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "INSERT INTO backend_series_artwork_overrides "
        "(backend_id,series_key,poster_url,poster_path,"
        "provider_id,external_id,poster_reference,"
        "revision,updated_at) "
        "VALUES(?,?,'',?,'tmdb',?,?,1,CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id,series_key) DO UPDATE SET "
        "poster_url='',"
        "poster_path=excluded.poster_path,"
        "provider_id='tmdb',"
        "external_id=excluded.external_id,"
        "poster_reference=excluded.poster_reference,"
        "revision=backend_series_artwork_overrides.revision+1,"
        "updated_at=CURRENT_TIMESTAMP;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool stored =
        bindText(statement, 1, backendId) &&
        bindText(statement, 2, seriesKey) &&
        bindText(statement, 3, posterPath) &&
        bindText(statement, 4, externalId) &&
        bindText(statement, 5, posterReference) &&
        sqlite3_step(statement) ==
            SQLITE_DONE;

    sqlite3_finalize(statement);

    return stored;
}

bool SeriesArtworkBackendSettingsService::removeCoverOverrideLocked(
    const std::string& backendId,
    const std::string& seriesKey) const
{
    if (!ensureSchemaLocked())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM backend_series_artwork_overrides "
        "WHERE backend_id=? AND series_key=?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool removed =
        bindText(statement, 1, backendId) &&
        bindText(statement, 2, seriesKey) &&
        sqlite3_step(statement) == SQLITE_DONE;

    sqlite3_finalize(statement);
    return removed;
}

std::string
SeriesArtworkBackendSettingsService::materializeTmdbSeriesPoster(
    const std::string& backendId,
    const std::string& externalId,
    const std::string& posterReference) const
{
    if (!validBackendId(backendId) ||
        !digitsOnly(externalId) ||
        !validTmdbPosterReference(
            posterReference))
    {
        return {};
    }

    std::string token;

    {
        std::lock_guard<std::mutex> lock(
            mutex_);

        snapshotLocked(
            backendId,
            &token);
    }

    if (token.empty())
    {
        return {};
    }

    TmdbRecordingMetadataCandidateProviderConfig config;

    config.readAccessToken = token;
    config.language = config_.tmdb.language;
    config.posterCacheRoot =
        config_.seriesCoverCacheRoot;
    config.connectTimeoutMs =
        config_.tmdb.connectTimeoutMs;
    config.totalTimeoutMs =
        config_.tmdb.totalTimeoutMs;
    config.maximumRetries =
        config_.tmdb.maximumRetries;
    config.retryBackoffMs =
        config_.tmdb.retryBackoffMs;
    config.maximumJsonBytes =
        config_.tmdb.maximumJsonBytes;
    config.maximumImageBytes =
        config_.tmdb.maximumImageBytes;

    TmdbRecordingMetadataCandidateProvider provider(
        transport_,
        std::move(config));

    return provider.materializePoster(
        "tv",
        externalId,
        posterReference);
}

SeriesArtworkImageResult
SeriesArtworkBackendSettingsService::tmdbCandidateImage(
    const std::string& backendId,
    const std::string& externalId,
    const std::string& posterReference) const
{
    const std::string path =
        materializeTmdbSeriesPoster(
            backendId,
            externalId,
            posterReference);

    if (path.empty())
    {
        SeriesArtworkImageResult result;
        result.statusCode = 502;
        result.errorCode =
            "tmdb_series_cover_unavailable";
        result.message =
            "The TMDB Series cover could not be prepared";
        return result;
    }

    return readSeriesArtworkImage(
        path,
        config_.seriesCoverCacheRoot);
}

SeriesArtworkImageResult
SeriesArtworkBackendSettingsService::coverImage(
    const std::string& backendId,
    const std::string& seriesKey) const
{
    SeriesArtworkImageResult missing;

    missing.statusCode = 404;
    missing.errorCode =
        "series_cover_image_not_found";
    missing.message =
        "The Series cover image is unavailable";

    if (!validBackendId(backendId) ||
        !validSeriesKey(seriesKey))
    {
        return missing;
    }

    std::string path;

    {
        std::lock_guard<std::mutex> lock(
            mutex_);

        if (!ensureSchemaLocked())
        {
            return missing;
        }

        sqlite3_stmt* statement = nullptr;

        const char* sql =
            "SELECT poster_path "
            "FROM backend_series_artwork_overrides "
            "WHERE backend_id=? AND series_key=? "
            "AND provider_id='tmdb' "
            "LIMIT 1;";

        if (sqlite3_prepare_v2(
                database_.handle(),
                sql,
                -1,
                &statement,
                nullptr) != SQLITE_OK)
        {
            return missing;
        }

        if (bindText(
                statement,
                1,
                backendId) &&
            bindText(
                statement,
                2,
                seriesKey) &&
            sqlite3_step(statement) ==
                SQLITE_ROW)
        {
            path = columnText(
                statement,
                0);
        }

        sqlite3_finalize(statement);
    }

    if (path.empty())
    {
        return missing;
    }

    return readSeriesArtworkImage(
        path,
        config_.seriesCoverCacheRoot);
}

std::string SeriesArtworkBackendSettingsService::readManagedTokenLocked(
    const std::string& backendId) const
{
    const int directory = openDirectoryNoFollow(config_.secretRoot);
    if (directory < 0)
    {
        return {};
    }

    const std::string filename = tokenFilename(backendId);
    const int descriptor = ::openat(
        directory,
        filename.c_str(),
        O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    ::close(directory);
    if (descriptor < 0)
    {
        return {};
    }

    struct stat metadata{};
    if (::fstat(descriptor, &metadata) != 0 ||
        !S_ISREG(metadata.st_mode) ||
        (metadata.st_mode & 0077) != 0 ||
        metadata.st_size <= 0 || metadata.st_size > 4096)
    {
        ::close(descriptor);
        return {};
    }

    std::string token(static_cast<std::size_t>(metadata.st_size), '\0');
    std::size_t offset = 0;
    while (offset < token.size())
    {
        const ssize_t count = ::read(
            descriptor,
            &token[offset],
            token.size() - offset);
        if (count < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            token.clear();
            break;
        }
        if (count == 0)
        {
            token.clear();
            break;
        }
        offset += static_cast<std::size_t>(count);
    }
    ::close(descriptor);

    return tokenSyntaxValid(token) ? token : std::string{};
}

bool SeriesArtworkBackendSettingsService::writeManagedTokenLocked(
    const std::string& backendId,
    const std::string& token) const
{
    if (!tokenSyntaxValid(token))
    {
        return false;
    }

    const int directory = openDirectoryNoFollow(config_.secretRoot);
    if (directory < 0)
    {
        return false;
    }

    const std::string filename = tokenFilename(backendId);
    const std::string temporary =
        "." + filename + "." + std::to_string(::getpid()) + ".tmp";
    const int descriptor = ::openat(
        directory,
        temporary.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
        S_IRUSR | S_IWUSR);
    if (descriptor < 0)
    {
        ::close(directory);
        return false;
    }

    bool written =
        ::fchmod(descriptor, S_IRUSR | S_IWUSR) == 0 &&
        writeAll(descriptor, token) &&
        ::fsync(descriptor) == 0;
    if (::close(descriptor) != 0)
    {
        written = false;
    }

    if (written)
    {
        written = ::renameat(
            directory,
            temporary.c_str(),
            directory,
            filename.c_str()) == 0;
    }
    if (written)
    {
        written = ::fsync(directory) == 0;
    }
    if (!written)
    {
        ::unlinkat(directory, temporary.c_str(), 0);
    }
    ::close(directory);
    return written;
}

bool SeriesArtworkBackendSettingsService::removeManagedTokenLocked(
    const std::string& backendId) const
{
    const int directory = openDirectoryNoFollow(config_.secretRoot);
    if (directory < 0)
    {
        return false;
    }

    const std::string filename = tokenFilename(backendId);
    const int removed = ::unlinkat(directory, filename.c_str(), 0);
    const bool success = removed == 0 || errno == ENOENT;
    const bool synced = !success || ::fsync(directory) == 0;
    ::close(directory);
    return success && synced;
}

SeriesArtworkBackendSettingsSnapshot
SeriesArtworkBackendSettingsService::snapshotLocked(
    const std::string& backendId,
    std::string* effectiveToken) const
{
    SeriesArtworkBackendSettingsSnapshot snapshot;
    snapshot.backendId = backendId;
    snapshot.coverOverrides = loadCoverOverridesLocked(backendId);

    std::string managedProvider;
    const bool providerRead =
        readManagedProviderLocked(backendId, managedProvider);
    if (providerRead && !managedProvider.empty() && validProvider(managedProvider))
    {
        snapshot.provider = managedProvider;
        snapshot.configurationSource = "managed";
    }
    else
    {
        snapshot.provider = config_.defaultProvider;
        snapshot.configurationSource = "environment";
    }

    const std::string managedToken = readManagedTokenLocked(backendId);
    if (!managedToken.empty())
    {
        snapshot.tmdbTokenConfigured = true;
        snapshot.tmdbTokenSource = "managed";
        if (effectiveToken != nullptr)
        {
            *effectiveToken = managedToken;
        }
    }
    else if (snapshot.configurationSource == "environment" &&
             tokenSyntaxValid(config_.environmentTmdbReadAccessToken))
    {
        snapshot.tmdbTokenConfigured = true;
        snapshot.tmdbTokenSource = "environment";
        if (effectiveToken != nullptr)
        {
            *effectiveToken = config_.environmentTmdbReadAccessToken;
        }
    }
    else if (effectiveToken != nullptr)
    {
        effectiveToken->clear();
    }

    return snapshot;
}

SeriesArtworkBackendSettingsSnapshot
SeriesArtworkBackendSettingsService::get(
    const std::string& backendId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!validBackendId(backendId))
    {
        return {};
    }
    return snapshotLocked(backendId);
}

SeriesArtworkBackendSettingsService::TokenValidation
SeriesArtworkBackendSettingsService::validateTmdbToken(
    const std::string& token) const
{
    if (!tokenSyntaxValid(token))
    {
        return TokenValidation::Invalid;
    }

    ExternalArtworkHttpRequest request;
    request.url = "https://api.themoviedb.org/3/configuration";
    request.bearerToken = token;
    request.accept = "application/json";
    request.connectTimeoutMs = config_.tmdb.connectTimeoutMs;
    request.totalTimeoutMs = config_.tmdb.totalTimeoutMs;
    request.maximumResponseBytes = config_.tmdb.maximumJsonBytes;

    const ExternalArtworkHttpResponse response = transport_.perform(request);
    if (response.transportError || response.statusCode == 429L ||
        response.statusCode >= 500L)
    {
        return TokenValidation::Unavailable;
    }
    if (response.statusCode != 200L ||
        !jsonContentType(response.contentType) ||
        response.body.empty() || response.body.front() != '{')
    {
        return TokenValidation::Invalid;
    }
    return TokenValidation::Valid;
}

SeriesArtworkBackendSettingsUpdateResult
SeriesArtworkBackendSettingsService::update(
    const SeriesArtworkBackendSettingsUpdate& request)
{
    SeriesArtworkBackendSettingsUpdateResult result;
    if (!validBackendId(request.backendId))
    {
        result.statusCode = 400;
        result.errorCode = "invalid_backend_id";
        result.message = "The backend ID is invalid";
        return result;
    }
    if (request.operation == "set-series-cover-tmdb")
    {
        if (!validSeriesKey(
                request.seriesKey))
        {
            result.statusCode = 400;
            result.errorCode =
                "invalid_series_key";
            result.message =
                "The Series key is invalid";
            return result;
        }

        if (request.providerId != "tmdb" ||
            request.externalNamespace != "tv" ||
            !digitsOnly(
                request.externalId) ||
            !validTmdbPosterReference(
                request.posterReference))
        {
            result.statusCode = 400;
            result.errorCode =
                "invalid_tmdb_series_cover";
            result.message =
                "The TMDB Series cover candidate is invalid";
            return result;
        }

        const std::string posterPath =
            materializeTmdbSeriesPoster(
                request.backendId,
                request.externalId,
                request.posterReference);

        if (posterPath.empty())
        {
            result.statusCode = 502;
            result.errorCode =
                "tmdb_series_cover_materialization_failed";
            result.message =
                "The TMDB Series cover could not be materialized";
            return result;
        }

        {
            std::lock_guard<std::mutex> lock(
                mutex_);

            if (!storeTmdbCoverOverrideLocked(
                    request.backendId,
                    request.seriesKey,
                    posterPath,
                    request.externalId,
                    request.posterReference))
            {
                result.statusCode = 503;
                result.errorCode =
                    "series_cover_persistence_unavailable";
                result.message =
                    "The Series cover override could not be persisted";
                return result;
            }

            result.success = true;
            result.statusCode = 200;
            result.settings =
                snapshotLocked(
                    request.backendId);
        }

        return result;
    }

    if (!request.operation.empty())
    {
        if (request.operation != "set-series-cover" &&
            request.operation != "clear-series-cover")
        {
            result.statusCode = 400;
            result.errorCode = "invalid_series_artwork_operation";
            result.message = "The Series artwork operation is invalid";
            return result;
        }

        if (!validSeriesKey(request.seriesKey))
        {
            result.statusCode = 400;
            result.errorCode = "invalid_series_key";
            result.message = "The Series key is invalid";
            return result;
        }

        if (request.operation == "set-series-cover" &&
            !validCoverPosterUrl(request.posterUrl))
        {
            result.statusCode = 400;
            result.errorCode = "invalid_series_cover_url";
            result.message =
                "The Series cover must reference an internal recording metadata image";
            return result;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (!ensureSchemaLocked())
        {
            result.statusCode = 503;
            result.errorCode = "settings_persistence_unavailable";
            result.message = "Backend settings persistence is unavailable";
            return result;
        }

        const bool stored =
            request.operation == "set-series-cover"
                ? storeCoverOverrideLocked(
                    request.backendId,
                    request.seriesKey,
                    request.posterUrl)
                : removeCoverOverrideLocked(
                    request.backendId,
                    request.seriesKey);

        if (!stored)
        {
            result.statusCode = 503;
            result.errorCode = "series_cover_persistence_unavailable";
            result.message =
                "The Series cover override could not be persisted";
            return result;
        }

        result.success = true;
        result.statusCode = 200;
        result.settings = snapshotLocked(request.backendId);
        return result;
    }

    if (!validProvider(request.provider))
    {
        result.statusCode = 400;
        result.errorCode = "invalid_artwork_provider";
        result.message = "The artwork provider must be none, tvmaze or tmdb";
        return result;
    }
    if (request.clearTmdbReadAccessToken &&
        !request.tmdbReadAccessToken.empty())
    {
        result.statusCode = 400;
        result.errorCode = "conflicting_token_update";
        result.message = "A token cannot be replaced and removed together";
        return result;
    }

    if (!request.tmdbReadAccessToken.empty())
    {
        const TokenValidation validation =
            validateTmdbToken(request.tmdbReadAccessToken);
        if (validation == TokenValidation::Unavailable)
        {
            result.statusCode = 503;
            result.errorCode = "tmdb_validation_unavailable";
            result.message = "TMDB token validation is temporarily unavailable";
            return result;
        }
        if (validation == TokenValidation::Invalid)
        {
            result.statusCode = 400;
            result.errorCode = "invalid_tmdb_read_access_token";
            result.message = "TMDB rejected the API Read Access Token";
            return result;
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (!ensureSchemaLocked())
    {
        result.statusCode = 503;
        result.errorCode = "settings_persistence_unavailable";
        result.message = "Backend settings persistence is unavailable";
        return result;
    }

    std::string effectiveToken;
    SeriesArtworkBackendSettingsSnapshot current =
        snapshotLocked(request.backendId, &effectiveToken);

    if (request.clearTmdbReadAccessToken)
    {
        if (request.provider == "tmdb")
        {
            result.statusCode = 400;
            result.errorCode = "tmdb_token_required";
            result.message = "TMDB requires an API Read Access Token";
            return result;
        }
        if (!removeManagedTokenLocked(request.backendId))
        {
            result.statusCode = 503;
            result.errorCode = "secret_persistence_unavailable";
            result.message = "The TMDB token could not be removed safely";
            return result;
        }
        effectiveToken.clear();
    }
    else if (!request.tmdbReadAccessToken.empty())
    {
        if (!writeManagedTokenLocked(
                request.backendId,
                request.tmdbReadAccessToken))
        {
            result.statusCode = 503;
            result.errorCode = "secret_persistence_unavailable";
            result.message = "The TMDB token could not be stored safely";
            return result;
        }
        effectiveToken = request.tmdbReadAccessToken;
    }
    else if (current.configurationSource == "environment" &&
             current.tmdbTokenSource == "environment" &&
             request.provider == "tmdb")
    {
        if (!writeManagedTokenLocked(request.backendId, effectiveToken))
        {
            result.statusCode = 503;
            result.errorCode = "secret_persistence_unavailable";
            result.message = "The existing TMDB token could not be adopted safely";
            return result;
        }
    }

    if (request.provider == "tmdb" && effectiveToken.empty())
    {
        result.statusCode = 400;
        result.errorCode = "tmdb_token_required";
        result.message = "TMDB requires an API Read Access Token";
        return result;
    }

    if (!invalidateUnresolvedSeriesMetadata(database_, request.backendId))
    {
        result.statusCode = 503;
        result.errorCode = "metadata_cache_invalidation_failed";
        result.message = "Cached series metadata could not be invalidated";
        return result;
    }

    if (!storeManagedProviderLocked(request.backendId, request.provider))
    {
        result.statusCode = 503;
        result.errorCode = "settings_persistence_unavailable";
        result.message = "Backend settings could not be stored";
        return result;
    }

    result.success = true;
    result.statusCode = 200;
    result.settings = snapshotLocked(request.backendId);
    return result;
}

SeriesArtworkFallbackResolution
SeriesArtworkBackendSettingsService::resolve(
    const std::string& backendId,
    const VdrEvent& event,
    const EpgScraperMetadata& metadata)
{
    std::string token;
    SeriesArtworkBackendSettingsSnapshot settings;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!validBackendId(backendId))
        {
            return {};
        }
        settings = snapshotLocked(backendId, &token);
    }

    if (settings.provider == "tvmaze")
    {
        TvmazeSeriesArtworkProvider provider(
            transport_,
            cache_,
            config_.tvmaze);
        return provider.resolve(backendId, event, metadata);
    }

    if (settings.provider == "tmdb" && !token.empty())
    {
        TmdbSeriesArtworkProviderConfig providerConfig = config_.tmdb;
        providerConfig.readAccessToken = token;
        TmdbSeriesArtworkProvider provider(
            transport_,
            cache_,
            std::move(providerConfig));
        const EpgScraperMetadata qualified =
            qualifyTvScraperTmdbIdentity(metadata);
        return provider.resolve(backendId, event, qualified);
    }

    return {};
}
